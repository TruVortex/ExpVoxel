#pragma once
#include "concurrency/work_stealing_pool.hpp"
#include "core/math.hpp"
#include "world/grid.hpp"
#include <cstdint>

void render_tiles_multithreaded(WorkStealingPool &pool, const Grid &grid,
                                uint32_t *pixels, int width, int height,
                                Vec3 cam_pos, float fov,
                                Vec3 cam_forward = {0.0f, 0.0f, 1.0f},
                                Vec3 cam_right = {1.0f, 0.0f, 0.0f},
                                Vec3 cam_up = {0.0f, 1.0f, 0.0f},
                                int tile_size = 16);