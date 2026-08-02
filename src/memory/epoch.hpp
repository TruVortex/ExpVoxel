#pragma once
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <limits>
#include <mutex>
#include <vector>

constexpr uint64_t INACTIVE_EPOCH = std::numeric_limits<uint64_t>::max();

class EpochManager {
  public:
    static constexpr size_t MAX_THREADS = 64;

    EpochManager() {
        for (size_t i = 0; i < MAX_THREADS; ++i) {
            m_active_epochs[i].store(INACTIVE_EPOCH, std::memory_order_relaxed);
        }
    }

    size_t register_thread() {
        static std::atomic<size_t> next_id{0};
        size_t id = next_id.fetch_add(1, std::memory_order_relaxed);
        return id % MAX_THREADS;
    }

    void enter_epoch(size_t thread_id) {
        uint64_t current = m_global_epoch.load(std::memory_order_relaxed);
        m_active_epochs[thread_id].store(current, std::memory_order_release);
    }

    void leave_epoch(size_t thread_id) {
        m_active_epochs[thread_id].store(INACTIVE_EPOCH,
                                         std::memory_order_release);
    }

    template <typename T> void defer_delete(T *ptr) {
        uint64_t current = m_global_epoch.load(std::memory_order_relaxed);
        std::lock_guard<std::mutex> lock(m_retire_mutex);
        m_retired.push_back(
            {ptr, [](void *p) { delete static_cast<T *>(p); }, current});
    }

    void gc() {
        uint64_t min_epoch =
            m_global_epoch.fetch_add(1, std::memory_order_acq_rel);
        for (size_t i = 0; i < MAX_THREADS; ++i) {
            uint64_t ep = m_active_epochs[i].load(std::memory_order_acquire);
            if (ep != INACTIVE_EPOCH) {
                min_epoch = std::min(min_epoch, ep);
            }
        }
        std::vector<RetiredNode> to_keep;
        std::vector<RetiredNode> to_delete;
        {
            std::lock_guard<std::mutex> lock(m_retire_mutex);
            for (const auto &node : m_retired) {
                if (node.epoch < min_epoch) {
                    to_delete.push_back(node);
                } else {
                    to_keep.push_back(node);
                }
            }
            m_retired = std::move(to_keep);
        }
        for (const auto &node : to_delete) {
            node.deleter(node.ptr);
        }
    }

    size_t pending_deletions() {
        std::lock_guard<std::mutex> lock(m_retire_mutex);
        return m_retired.size();
    }

  private:
    struct RetiredNode {
        void *ptr;
        void (*deleter)(void *);
        uint64_t epoch;
    };

    std::atomic<uint64_t> m_global_epoch{0};
    alignas(64) std::atomic<uint64_t> m_active_epochs[MAX_THREADS];

    std::vector<RetiredNode> m_retired;
    std::mutex m_retire_mutex;
};