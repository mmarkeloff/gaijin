#pragma once

#include <atomic>
#include <unordered_map>
#include <thread>
#include <memory>
#include <shared_mutex>

#include "concurrentqueue/concurrentqueue.h"

#include "common/proto.h"

class Metrics
{
private:
    struct Metric
    {
        std::atomic<int64_t> reads;
        std::atomic<int64_t> writes;
    };

    struct Task
    {
        Key key;
        Action action;
    };

public:
    Metrics(int64_t statDelta = 5 /* seconds */);
    ~Metrics();

    void add(const Key &key, Action action) noexcept;

    int64_t getReads(const Key &key) noexcept;
    int64_t getWrites(const Key &key) noexcept;

private:
    void produceMetrics() noexcept;

    void stat() noexcept;

private:
    std::shared_mutex m_guard;
    std::unordered_map<Key, Metric> m_keyMetrics;
    Metric m_totalMetrics;

    std::atomic_bool m_stop;
    std::unique_ptr<std::thread> m_statist, m_metricProducer;

    moodycamel::ConcurrentQueue<Task> m_metricTasks;

    int64_t m_statDelta;
};