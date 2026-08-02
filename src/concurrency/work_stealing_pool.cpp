#include "concurrency/work_stealing_pool.hpp"

thread_local int WorkStealingPool::t_thread_id = -1;

WorkStealingPool::WorkStealingPool(size_t threads) {
    m_deques.reserve(threads);
    for (size_t i = 0; i < threads; ++i) {
        m_deques.push_back(std::make_unique<ChaseLevDeque<Task *>>());
    }
    for (size_t i = 0; i < threads; ++i) {
        m_threads.emplace_back([this, i] {
            t_thread_id = static_cast<int>(i);
            while (!m_stop.load(std::memory_order_relaxed)) {
                Task *task = m_deques[i]->pop(nullptr);
                if (!task) {
                    std::lock_guard<std::mutex> lock(m_global_mutex);
                    if (!m_global_queue.empty()) {
                        task = m_global_queue.front();
                        m_global_queue.pop();
                    }
                }
                if (!task) {
                    for (size_t offset = 1; offset < m_deques.size();
                         ++offset) {
                        size_t target = (i + offset) % m_deques.size();
                        task = m_deques[target]->steal(nullptr);
                        if (task) {
                            break;
                        }
                    }
                }
                if (task) {
                    (*task)();
                    delete task;
                    m_active_tasks.fetch_sub(1, std::memory_order_release);
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }
}

WorkStealingPool::~WorkStealingPool() {
    m_stop.store(true, std::memory_order_relaxed);
    for (auto &thread : m_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void WorkStealingPool::enqueue(Task task) {
    m_active_tasks.fetch_add(1, std::memory_order_relaxed);
    Task *task_ptr = new Task(std::move(task));

    if (t_thread_id >= 0 &&
        static_cast<size_t>(t_thread_id) < m_deques.size()) {
        m_deques[t_thread_id]->push(task_ptr);
    } else {
        std::lock_guard<std::mutex> lock(m_global_mutex);
        m_global_queue.push(task_ptr);
    }
}

void WorkStealingPool::wait_all() {
    while (m_active_tasks.load(std::memory_order_acquire) > 0) {
        std::this_thread::yield();
    }
}