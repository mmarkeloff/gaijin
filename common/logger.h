#pragma once

#include "concurrentqueue/concurrentqueue.h"
#include "json/include/nlohmann/json.hpp"

enum LogSeverity : uint32_t
{
    DEBUG,
    INFO,
    ERR
};
using LogMessage = nlohmann::json;

class Logger
{
private:
    struct MsgTask
    {
        LogSeverity lvl;
        LogMessage msg;
    };

public:
    Logger(LogSeverity lvl);
    ~Logger();

    void log(LogSeverity msgLvl, LogMessage &&msg) noexcept;

    void work() noexcept;

private:
    LogSeverity m_lvl;

    std::atomic_bool m_stop;
    std::unique_ptr<std::thread> m_worker;
    moodycamel::ConcurrentQueue<MsgTask> m_msgs;
};

inline Logger &logger() { 
    static Logger instance{LogSeverity::INFO}; 
    return instance;
}