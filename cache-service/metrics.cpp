#include "metrics.h"
#include "common/logger.h"

Metrics::Metrics(int64_t statDelta)
    : m_statDelta{statDelta}
{
    m_stop.store(false);
    m_metricProducer = std::make_unique<std::thread>(&Metrics::produceMetrics, this);
    m_statist = std::make_unique<std::thread>(&Metrics::stat, this);
}
Metrics::~Metrics()
{
    m_stop.store(true);
    m_statist->join();
    m_metricProducer->join();
}

void Metrics::add(const Key &key, Action action) noexcept { m_metricTasks.enqueue({key, action}); }

int64_t Metrics::getReads(const Key &key) noexcept
{
    std::shared_lock guard{m_guard};
    auto it = m_keyMetrics.find(key);
    return it != m_keyMetrics.end() ? it->second.reads.load(std::memory_order_relaxed) : 0;
}
int64_t Metrics::getWrites(const Key &key) noexcept
{
    std::shared_lock guard{m_guard};
    auto it = m_keyMetrics.find(key);
    return it != m_keyMetrics.end() ? it->second.writes.load(std::memory_order_relaxed) : 0;
}

void Metrics::produceMetrics() noexcept
{
    while (!m_stop.load())
    {
        Task task;
        if (!m_metricTasks.try_dequeue(task)) // try_dequeue_bulk() for optimization
        {
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
            continue;
        }

        std::shared_lock readGuard{m_guard};

        auto it = m_keyMetrics.find(task.key);
        if (it == m_keyMetrics.end())
        {
            // because value is atomic, capture write lock only when hash table increasing its size
            readGuard.unlock();
            {
                std::lock_guard writeGuard{m_guard};
                m_keyMetrics[task.key];
            }
            readGuard.lock();
            it = m_keyMetrics.find(task.key);
        }

        if (task.action == Action::GET)
        {
            it->second.reads++;
            m_totalMetrics.reads++;
        }
        else
        {
            it->second.writes++;
            m_totalMetrics.writes++;
        }
    }
}

void Metrics::stat() noexcept
{
    auto prev = std::chrono::system_clock::now();
    auto last = m_totalMetrics.reads.load() + m_totalMetrics.writes.load();
    while (!m_stop.load())
    {
        auto now = std::chrono::system_clock::now();
        auto delta = std::chrono::duration_cast<std::chrono::seconds>(now - prev);
        if (delta.count() < m_statDelta)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
        }
        else
        {
            auto reads = m_totalMetrics.reads.load();
            auto writes = m_totalMetrics.writes.load();
            auto total = reads + writes;
            logger().log(LogSeverity::INFO,
                         {"stat",
                          {"read requests", reads},
                          {"write requests", writes},
                          {"processed in last " + std::to_string(m_statDelta) + " seconds", total - last}});
            prev = now;
            last = total;
        }
    }
}