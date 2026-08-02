#pragma once
#include "concurrency/chase_lev_deque.hpp"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class WorkStealingPool {
  public:
    using Task = std::function<void()>;

    explicit WorkStealingPool(
        size_t threads = std::thread::hardware_concurrency());
    ~WorkStealingPool();

    void enqueue(Task task);
    void wait_all();
    size_t thread_count() const { return m_threads.size(); }

  private:
    std::vector<std::thread> m_threads;
    std::vector<std::unique_ptr<ChaseLevDeque<Task *>>> m_deques;

    std::queue<Task *> m_global_queue;
    std::mutex m_global_mutex;

    std::atomic<bool> m_stop{false};
    std::atomic<size_t> m_active_tasks{0};

    static thread_local int t_thread_id;
};