#pragma once
#include <atomic>
#include <cstdint>
#include <vector>

template <typename T> class ChaseLevDeque {
  private:
    struct Array {
        int64_t capacity;
        int64_t mask;
        std::atomic<T> *buffer;

        explicit Array(int64_t cap) : capacity(cap), mask(cap - 1) {
            buffer = new std::atomic<T>[cap];
        }
        ~Array() { delete[] buffer; }

        void store(int64_t i, T val) {
            buffer[i & mask].store(val, std::memory_order_relaxed);
        }

        T load(int64_t i) {
            return buffer[i & mask].load(std::memory_order_relaxed);
        }
    };

    std::atomic<int64_t> m_top{0};
    std::atomic<int64_t> m_bottom{0};
    std::atomic<Array *> m_array;
    std::vector<Array *> m_garbage;

  public:
    explicit ChaseLevDeque(int64_t initial_capacity = 65536) {
        Array *a = new Array(initial_capacity);
        m_array.store(a, std::memory_order_relaxed);
        m_garbage.push_back(a);
    }

    ~ChaseLevDeque() {
        for (Array *a : m_garbage) {
            delete a;
        }
    }

    void push(T val) {
        int64_t b = m_bottom.load(std::memory_order_relaxed);
        int64_t t = m_top.load(std::memory_order_acquire);
        Array *a = m_array.load(std::memory_order_relaxed);
        if (b - t >= a->capacity - 1) {
            int64_t new_cap = a->capacity * 2;
            Array *new_a = new Array(new_cap);
            for (int64_t i = t; i < b; ++i) {
                new_a->store(i, a->load(i));
            }
            m_array.store(new_a, std::memory_order_release);
            m_garbage.push_back(new_a);
            a = new_a;
        }
        a->store(b, val);
        std::atomic_thread_fence(std::memory_order_release);
        m_bottom.store(b + 1, std::memory_order_relaxed);
    }

    T pop(T empty_val) {
        int64_t b = m_bottom.load(std::memory_order_relaxed) - 1;
        Array *a = m_array.load(std::memory_order_relaxed);
        m_bottom.store(b, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        int64_t t = m_top.load(std::memory_order_relaxed);
        if (t <= b) {
            T val = a->load(b);
            if (t == b) {
                if (!m_top.compare_exchange_strong(t, t + 1,
                                                   std::memory_order_seq_cst,
                                                   std::memory_order_relaxed)) {
                    val = empty_val;
                }
                m_bottom.store(b + 1, std::memory_order_relaxed);
            }
            return val;
        } else {
            m_bottom.store(b + 1, std::memory_order_relaxed);
            return empty_val;
        }
    }

    T steal(T empty_val) {
        int64_t t = m_top.load(std::memory_order_acquire);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        int64_t b = m_bottom.load(std::memory_order_acquire);
        if (t < b) {
            Array *a = m_array.load(std::memory_order_acquire);
            T val = a->load(t);
            if (!m_top.compare_exchange_strong(t, t + 1,
                                               std::memory_order_seq_cst,
                                               std::memory_order_relaxed)) {
                return empty_val;
            }
            return val;
        }
        return empty_val;
    }
};