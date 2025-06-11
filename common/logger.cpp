#include <iostream>
#include <chrono>
#include <array>

#include "common/logger.h"

static const char *severityToString(LogSeverity lvl) noexcept
{
    static constexpr std::array<const char *, 3> tab{"debug", "info", "err"};
    if (lvl > LogSeverity::ERR)
    {
        lvl = LogSeverity::DEBUG;
    }
    return tab[lvl];
}
static std::ostream &pickStream(LogSeverity lvl, std::ostream &cout, std::ostream &cerr) noexcept
{
    return lvl >= LogSeverity::ERR ? cerr : cout;
}

Logger::Logger(LogSeverity lvl)
    : m_lvl{lvl}
{
    m_stop.store(false, std::memory_order_release);
    m_worker = std::make_unique<std::thread>(&Logger::work, this);
}
Logger::~Logger()
{
    m_stop.store(true, std::memory_order_release);
    m_worker->join();
}

void Logger::log(LogSeverity msgLvl, LogMessage &&msg) noexcept { m_msgs.enqueue({msgLvl, std::move(msg)}); }

void Logger::work() noexcept
{
    // TODO: if logger is not hot, use std::condition_variable{} for optimization
    while (!m_stop.load(std::memory_order_acquire))
    {
        MsgTask task;
        if (m_msgs.try_dequeue(task))
        {
            if (task.lvl >= m_lvl)
            {
                LogMessage msg = {std::chrono::system_clock::now().time_since_epoch().count(),
                                  severityToString(task.lvl),
                                  std::move(task.msg)};

                pickStream(task.lvl, std::cout, std::cerr) << std::move(msg) << std::endl;
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
    }
}