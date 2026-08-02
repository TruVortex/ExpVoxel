#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdlib>

class Arena {
  public:
    explicit Arena(size_t capacity_bytes)
        : m_capacity(capacity_bytes), m_offset(0) {
#if defined(_MSC_VER)
        m_buffer = static_cast<uint8_t *>(_aligned_malloc(capacity_bytes, 64));
#else
        m_buffer =
            static_cast<uint8_t *>(std::aligned_alloc(64, capacity_bytes));
#endif
    }

    ~Arena() {
#if defined(_MSC_VER)
        _aligned_free(m_buffer);
#else
        std::free(m_buffer);
#endif
    }

    Arena(const Arena &) = delete;
    Arena &operator=(const Arena &) = delete;

    void *alloc(size_t bytes, size_t alignment = 64) {
        size_t current = reinterpret_cast<size_t>(m_buffer + m_offset);
        size_t aligned = (current + alignment - 1) & ~(alignment - 1);
        size_t padding = aligned - current;
        if (m_offset + padding + bytes > m_capacity) {
            return nullptr;
        }
        m_offset += padding + bytes;
        return reinterpret_cast<void *>(aligned);
    }

    void reset() { m_offset = 0; }
    size_t used() const { return m_offset; }
    size_t capacity() const { return m_capacity; }

  private:
    uint8_t *m_buffer{nullptr};
    size_t m_capacity{0};
    size_t m_offset{0};
};