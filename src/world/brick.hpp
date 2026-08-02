#pragma once
#include <atomic>
#include <cstdint>

constexpr int BRICK_SIZE = 8;
constexpr int BRICK_VOXELS = BRICK_SIZE * BRICK_SIZE * BRICK_SIZE;

struct Voxel {
    uint8_t r{0}, g{0}, b{0};
};

struct alignas(64) Brick {
    Voxel voxels[BRICK_VOXELS];
    std::atomic<uint64_t> occupancy[8]{0, 0, 0, 0, 0, 0, 0, 0};
};