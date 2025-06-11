#include "cache_svc.h"

CacheService::CacheService(boost::asio::io_context &ioContext,
                           uint16_t port,
                           uint32_t consumersCount,
                           uint32_t producersCount)
    : m_observer{this}
    , m_srv{ioContext, m_observer}
    , m_cache{std::make_unique<Cache>()}
    , m_metrics{std::make_unique<Metrics>()}
{
    m_stop.store(false, std::memory_order_release);

    auto createWorkers = [this](auto &workers, uint32_t count, auto &&f) {
        workers.reserve(count);
        for (decltype(count) i = 0; i < count; ++i)
        {
            workers.push_back(std::make_unique<std::thread>(f, this));
        }
    };

    createWorkers(m_commandsConsumers, consumersCount, &CacheService::consumeCommands);
    createWorkers(m_commandsProducers, producersCount, &CacheService::produceCommands);

    m_srv.listen(boost::asio::ip::tcp::v4(), port);
    m_srv.startAcceptingConnections();

    logger().log(LogSeverity::INFO, {"cache-service started", {"port", port}});
}

void CacheService::stop() noexcept
{
    m_stop.store(true, std::memory_order_release);
    m_srv.close();
    for (auto &worker : m_commandsProducers)
    {
        worker->join();
    }
    for (auto &worker : m_commandsConsumers)
    {
        worker->join();
    }
}

void CacheService::newCommandsData(int cid, std::string_view data) noexcept
{
    m_commandsData.enqueue({cid, std::string{data}});
}

void CacheService::produceCommands() noexcept
{
    while (!m_stop.load(std::memory_order_acquire) || m_commandsData.size_approx() != 0)
    {
        DataTask task;
        if (m_commandsData.try_dequeue(task))
        {
            auto commands = parseCommands(task.data, task.cid);
            if (!commands.empty())
            {
                m_commands.enqueue_bulk(commands.data(), commands.size());
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
    }
}
void CacheService::consumeCommands() noexcept
{
    while (!m_stop.load(std::memory_order_acquire) || m_commands.size_approx() != 0)
    {
        Command comm;
        if (!m_commands.try_dequeue(comm)) // try_dequeue_bulk() for optimization
        {
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
            continue;
        }

        Value val;
        if (comm.action == Action::GET)
        {
            val = m_cache->get(comm.key).value_or("none");
        }
        else
        {
            m_cache->set(comm.key, comm.val.value());
            val = comm.val.value();
        }

        m_metrics->add(comm.key, comm.action);
        m_srv.send(comm.cid,
                   RESPONSE_DELIM + comm.key + "=" + val + " (reads=" + std::to_string(m_metrics->getReads(comm.key)) +
                       " writes=" + std::to_string(m_metrics->getWrites(comm.key)) + ")");
    }
}