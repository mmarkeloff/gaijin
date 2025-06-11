#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <thread>
#include <string_view>

#include "concurrentqueue/concurrentqueue.h"

#include "async_tcp_srv.h"
#include "common/proto.h"
#include "cache.h"
#include "metrics.h"
#include "common/logger.h"

class CacheService
{
public:
    struct Observer : AsyncTcpServer::Observer
    {
        Observer(CacheService *svc)
            : m_svc{svc}
        { }
        void onConnectionAccepted(int cid) { logger().log(LogSeverity::DEBUG, {"new client", {"id", cid}}); }
        void onConnectionClosed(int cid) { logger().log(LogSeverity::DEBUG, {"client disconnected", {"id", cid}}); }
        void onReceived(int cid, const char *data, size_t size)
        {
            m_svc->newCommandsData(cid, std::string_view{data, size});
        }

    private:
        CacheService *m_svc;
    };

    struct DataTask
    {
        int cid;
        std::string data;
    };

public:
    CacheService(boost::asio::io_context &ioContext,
                 uint16_t port = 1234,
                 uint32_t consumersCount = 2,
                 uint32_t producersCount = 1);
    ~CacheService() { stop(); }

    void newCommandsData(int cid, std::string_view data) noexcept;

private:
    void stop() noexcept;

    // parse transport data to commands and make tasks for executing these commands
    void produceCommands() noexcept;

    // execute commands
    void consumeCommands() noexcept;

private:
    Observer m_observer;
    AsyncTcpServer m_srv;

    std::atomic_bool m_stop;

    std::vector<std::unique_ptr<std::thread>> m_commandsConsumers;
    moodycamel::ConcurrentQueue<Command> m_commands;

    std::vector<std::unique_ptr<std::thread>> m_commandsProducers;
    moodycamel::ConcurrentQueue<DataTask> m_commandsData;

    std::unique_ptr<Cache> m_cache;
    std::unique_ptr<Metrics> m_metrics;
};