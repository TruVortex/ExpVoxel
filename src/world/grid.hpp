#pragma once
#include "world/brick.hpp"
#include <atomic>
#include <memory>
#include <vector>

class Grid {
  public:
    Grid(int brick_count_x, int brick_count_y, int brick_count_z)
        : m_bx(brick_count_x), m_by(brick_count_y), m_bz(brick_count_z),
          m_width(m_bx * BRICK_SIZE), m_height(m_by * BRICK_SIZE),
          m_depth(m_bz * BRICK_SIZE),
          m_bricks(static_cast<size_t>(m_bx * m_by * m_bz)),
          m_raw_bricks(static_cast<size_t>(m_bx * m_by * m_bz), nullptr) {}

    void set_voxel(int x, int y, int z, uint8_t r, uint8_t g, uint8_t b) {
        if (!in_bounds(x, y, z)) {
            return;
        }
        int bx = x / 8, by = y / 8, bz = z / 8;
        int lx = x & 7, ly = y & 7, lz = z & 7;
        size_t b_idx = brick_index(bx, by, bz);
        if (!m_bricks[b_idx]) {
            m_bricks[b_idx] = std::make_unique<Brick>();
            m_raw_bricks[b_idx] = m_bricks[b_idx].get();
        }
        size_t v_idx = static_cast<size_t>(lx + (ly * 8) + (lz * 64));
        m_bricks[b_idx]->voxels[v_idx] = {r, g, b};
        m_bricks[b_idx]->occupancy[lz].fetch_or(1ULL << (v_idx & 63),
                                                std::memory_order_release);
    }

    void clear_voxel_atomic(int x, int y, int z) {
        if (!in_bounds(x, y, z)) {
            return;
        }
        int bx = x / 8, by = y / 8, bz = z / 8;
        size_t b_idx = brick_index(bx, by, bz);
        Brick *brick = m_raw_bricks[b_idx];
        if (!brick) {
            return;
        }
        int lx = x & 7, ly = y & 7, lz = z & 7;
        brick->occupancy[lz].fetch_and(~(1ULL << (lx + (ly * 8))),
                                       std::memory_order_release);
    }

    inline bool is_solid(int x, int y, int z) const {
        if (!in_bounds(x, y, z)) {
            return false;
        }
        int bx = x / 8, by = y / 8, bz = z / 8;
        size_t b_idx = static_cast<size_t>(bx + by * m_bx + bz * m_bx * m_by);
        const Brick *brick = m_raw_bricks[b_idx];
        if (!brick) {
            return false;
        }
        int lx = x & 7, ly = y & 7, lz = z & 7;
        uint64_t mask = brick->occupancy[lz].load(std::memory_order_relaxed);
        return mask & (1ULL << ((lx + (ly * 8)) & 63));
    }

    inline bool get_colour(int x, int y, int z, uint8_t &r, uint8_t &g,
                          uint8_t &b_out) const {
        if (!in_bounds(x, y, z)) {
            return false;
        }
        int bx = x / 8, by = y / 8, bz = z / 8;
        size_t b_idx = static_cast<size_t>(bx + by * m_bx + bz * m_bx * m_by);
        const Brick *brick = m_raw_bricks[b_idx];
        if (!brick) {
            return false;
        }
        int lx = x & 7, ly = y & 7, lz = z & 7;
        size_t v_idx = static_cast<size_t>(lx + (ly * 8) + (lz * 64));
        uint64_t mask = brick->occupancy[lz].load(std::memory_order_relaxed);
        if ((mask & (1ULL << (v_idx & 63))) == 0) {
            return false;
        }
        const auto &v = brick->voxels[v_idx];
        r = v.r;
        g = v.g;
        b_out = v.b;
        return true;
    }

    inline bool in_bounds(int x, int y, int z) const {
        return static_cast<unsigned>(x) < static_cast<unsigned>(m_width) &&
               static_cast<unsigned>(y) < static_cast<unsigned>(m_height) &&
               static_cast<unsigned>(z) < static_cast<unsigned>(m_depth);
    }

    inline const Brick *get_brick_ptr(int bx, int by, int bz) const {
        if (static_cast<unsigned>(bx) >= static_cast<unsigned>(m_bx) ||
            static_cast<unsigned>(by) >= static_cast<unsigned>(m_by) ||
            static_cast<unsigned>(bz) >= static_cast<unsigned>(m_bz)) {
            return nullptr;
        }
        return m_raw_bricks[static_cast<size_t>(bx + by * m_bx +
                                                bz * m_bx * m_by)];
    }

    int width() const { return m_width; }
    int height() const { return m_height; }
    int depth() const { return m_depth; }

  private:
    int m_bx, m_by, m_bz;
    int m_width, m_height, m_depth;
    std::vector<std::unique_ptr<Brick>> m_bricks;
    std::vector<Brick *> m_raw_bricks;

    inline size_t brick_index(int bx, int by, int bz) const {
        return static_cast<size_t>(bx + by * m_bx + bz * m_bx * m_by);
    }
};