#include "concurrency/thread_pool.hpp"

ThreadPool::ThreadPool(size_t threads) {
    for (size_t i = 0; i < threads; ++i) {
        m_workers.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(m_mutex);
                    m_cv_task.wait(
                        lock, [this] { return m_stop || !m_tasks.empty(); });
                    if (m_stop && m_tasks.empty()) {
                        return;
                    }
                    task = std::move(m_tasks.front());
                    m_tasks.pop();
                    m_active_tasks++;
                }
                task();
                {
                    std::unique_lock<std::mutex> lock(m_mutex);
                    m_active_tasks--;
                    if (m_tasks.empty() && m_active_tasks == 0) {
                        m_cv_done.notify_all();
                    }
                }
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_stop = true;
    }
    m_cv_task.notify_all();
    for (std::thread &worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_tasks.push(std::move(task));
    }
    m_cv_task.notify_one();
}

void ThreadPool::wait_all() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv_done.wait(lock,
                   [this] { return m_tasks.empty() && m_active_tasks == 0; });
}