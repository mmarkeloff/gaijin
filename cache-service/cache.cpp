#include <fstream>

#include "nlohmann/json.hpp"

#include "cache.h"
#include "common/logger.h"

Cache::Cache(int64_t flushDelta)
    : m_flushDelta{flushDelta}
{ 
    load();
    m_stop.store(false);
    m_flusher = std::make_unique<std::thread>(&Cache::flush, this); 
}
Cache::~Cache() { 
    m_stop.store(true);
    m_flusher->join(); 
}

void Cache::set(const Key &key, const Value &value) noexcept
{
    // if $set is hot, make lockfree queue with tasks to update table in separate thread
    std::lock_guard guard{m_guard};
    m_tab[key] = value;
}

std::optional<Value> Cache::get(const Key &key) const noexcept
{
    std::shared_lock guard{m_guard};
    auto it = m_tab.find(key);
    return it != m_tab.end() ? it->second : std::optional<Value>{};
}

void Cache::load() noexcept {
    std::lock_guard guard{m_guard};

    std::ifstream f{path};
    Key key;
    Value value;
    while (f >> key >> value)
    {
        m_tab[key] = value;
    }
    f.close();
}

void Cache::flush() const noexcept
{
    auto prev = std::chrono::system_clock::now();
    while (!m_stop.load())
    {
        auto now = std::chrono::system_clock::now();
        auto delta = std::chrono::duration_cast<std::chrono::minutes>(now - prev);
        if (delta.count() < m_flushDelta)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
        }
        else
        {
            prev = now;
            doFlush();
        }
    }
}

void Cache::doFlush() const noexcept {
    std::shared_lock guard{m_guard};
    logger().log(LogSeverity::INFO, {"flushing cache", {"to", path}, {"size", m_tab.size()}});
    std::ofstream f(path);
    if (!f.is_open())
    {
        logger().log(LogSeverity::ERR, {"unable to open file", {"file", path}});
        return;
    }
    for (const auto& [key, value] : m_tab)
    {
        f << key << " " << value << std::endl;
    }
    f.close();
}
