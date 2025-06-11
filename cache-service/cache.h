#pragma once

#include <shared_mutex>
#include <unordered_map>
#include <atomic>
#include <thread>

#include "common/proto.h"

class Cache
{
public:
    static constexpr const char *path = "config.txt";

public:
    Cache(int64_t flushDelta = 1 /* minutes */);
    ~Cache();

    void set(const Key &key, const Value &value) noexcept;
    std::optional<Value> get(const Key &key) const noexcept;

private:
    void load() noexcept;

    void flush() const noexcept;
    void doFlush() const noexcept;

private:
    mutable std::shared_mutex m_guard;
    std::unordered_map<Key, Value> m_tab;

    int64_t m_flushDelta;

    std::atomic_bool m_stop;
    std::unique_ptr<std::thread> m_flusher;
};