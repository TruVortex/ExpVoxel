#include "rendering/simd_dda.hpp"
#include <algorithm>
#include <cmath>
#include <immintrin.h>
#include <limits>

inline bool intersect_aabb(float ox, float oy, float oz, float dx, float dy,
                           float dz, float max_x, float max_y, float max_z,
                           float &t_near, float &t_far) {
    float inv_dx = (std::abs(dx) < 1e-6f) ? 1e6f : 1.0f / dx;
    float inv_dy = (std::abs(dy) < 1e-6f) ? 1e6f : 1.0f / dy;
    float inv_dz = (std::abs(dz) < 1e-6f) ? 1e6f : 1.0f / dz;
    float t1 = (0.0f - ox) * inv_dx;
    float t2 = (max_x - ox) * inv_dx;
    float t3 = (0.0f - oy) * inv_dy;
    float t4 = (max_y - oy) * inv_dy;
    float t5 = (0.0f - oz) * inv_dz;
    float t6 = (max_z - oz) * inv_dz;
    float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)),
                          std::min(t5, t6));
    float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)),
                          std::max(t5, t6));
    if (tmax < 0.0f || tmin > tmax) {
        return false;
    }
    t_near = (tmin < 0.0f) ? 0.0f : tmin;
    t_far = tmax;
    return true;
}

std::array<HitResult, 8> trace_dda_simd_packet(const RayPacket8 &packet,
                                               const Grid &grid) {
    std::array<HitResult, 8> results{};
    float grid_w = static_cast<float>(grid.width());
    float grid_h = static_cast<float>(grid.height());
    float grid_d = static_cast<float>(grid.depth());
    for (int i = 0; i < 8; ++i) {
        if (!(packet.active_mask & (1 << i))) {
            continue;
        }
        float ox = packet.orig_x[i];
        float oy = packet.orig_y[i];
        float oz = packet.orig_z[i];
        float dx = packet.dir_x[i];
        float dy = packet.dir_y[i];
        float dz = packet.dir_z[i];
        float t_near = 0.0f, t_far = 0.0f;
        if (!intersect_aabb(ox, oy, oz, dx, dy, dz, grid_w, grid_h, grid_d,
                            t_near, t_far)) {
            continue;
        }
        float entry_x = ox + (t_near + 1e-3f) * dx;
        float entry_y = oy + (t_near + 1e-3f) * dy;
        float entry_z = oz + (t_near + 1e-3f) * dz;
        int map_x = static_cast<int>(std::floor(entry_x));
        int map_y = static_cast<int>(std::floor(entry_y));
        int map_z = static_cast<int>(std::floor(entry_z));
        int step_x = (dx >= 0.0f) ? 1 : -1;
        int step_y = (dy >= 0.0f) ? 1 : -1;
        int step_z = (dz >= 0.0f) ? 1 : -1;
        float t_delta_x = (std::abs(dx) < 1e-6f)
                              ? std::numeric_limits<float>::infinity()
                              : std::abs(1.0f / dx);
        float t_delta_y = (std::abs(dy) < 1e-6f)
                              ? std::numeric_limits<float>::infinity()
                              : std::abs(1.0f / dy);
        float t_delta_z = (std::abs(dz) < 1e-6f)
                              ? std::numeric_limits<float>::infinity()
                              : std::abs(1.0f / dz);
        float t_max_x = (dx > 0.0f)   ? (map_x + 1.0f - entry_x) * t_delta_x
                        : (dx < 0.0f) ? (entry_x - map_x) * t_delta_x
                                      : std::numeric_limits<float>::infinity();
        float t_max_y = (dy > 0.0f)   ? (map_y + 1.0f - entry_y) * t_delta_y
                        : (dy < 0.0f) ? (entry_y - map_y) * t_delta_y
                                      : std::numeric_limits<float>::infinity();
        float t_max_z = (dz > 0.0f) ? (map_z + 1.0f - entry_z) * t_delta_z
                                    : (entry_z - map_z) * t_delta_z;
        constexpr int MAX_STEPS = 128;
        int steps = 0;
        while (steps < MAX_STEPS && grid.in_bounds(map_x, map_y, map_z)) {
            int bx = map_x >> 3;
            int by = map_y >> 3;
            int bz = map_z >> 3;
            const Brick *brick = grid.get_brick_ptr(bx, by, bz);
            if (!brick) {
                int nx = (step_x > 0) ? (((bx + 1) << 3) - map_x)
                                      : (map_x - ((bx << 3) - 1));
                int ny = (step_y > 0) ? (((by + 1) << 3) - map_y)
                                      : (map_y - ((by << 3) - 1));
                int nz = (step_z > 0) ? (((bz + 1) << 3) - map_z)
                                      : (map_z - ((bz << 3) - 1));
                float t_exit_x = t_max_x + (nx - 1) * t_delta_x;
                float t_exit_y = t_max_y + (ny - 1) * t_delta_y;
                float t_exit_z = t_max_z + (nz - 1) * t_delta_z;
                if (t_exit_x < t_max_y && t_exit_x < t_max_z) {
                    map_x += nx * step_x;
                    t_max_x += nx * t_delta_x;
                } else if (t_exit_y < t_max_x && t_exit_y < t_max_z) {
                    map_y += ny * step_y;
                    t_max_y += ny * t_delta_y;
                } else if (t_exit_z < t_max_x && t_exit_z < t_max_y) {
                    map_z += nz * step_z;
                    t_max_z += nz * t_delta_z;
                } else {
                    if (t_max_x < t_max_y) {
                        if (t_max_x < t_max_z) {
                            map_x += step_x;
                            t_max_x += t_delta_x;
                        } else {
                            map_z += step_z;
                            t_max_z += t_delta_z;
                        }
                    } else {
                        if (t_max_y < t_max_z) {
                            map_y += step_y;
                            t_max_y += t_delta_y;
                        } else {
                            map_z += step_z;
                            t_max_z += t_delta_z;
                        }
                    }
                }
                steps++;
                continue;
            }
            if (grid.is_solid(map_x, map_y, map_z)) {
                results[i].hit = true;
                results[i].voxel_x = map_x;
                results[i].voxel_y = map_y;
                results[i].voxel_z = map_z;
                grid.get_colour(map_x, map_y, map_z, results[i].colour_r,
                               results[i].colour_g, results[i].colour_b);
                break;
            }
            if (t_max_x < t_max_y) {
                if (t_max_x < t_max_z) {
                    map_x += step_x;
                    t_max_x += t_delta_x;
                } else {
                    map_z += step_z;
                    t_max_z += t_delta_z;
                }
            } else {
                if (t_max_y < t_max_z) {
                    map_y += step_y;
                    t_max_y += t_delta_y;
                } else {
                    map_z += step_z;
                    t_max_z += t_delta_z;
                }
            }
            steps++;
        }
    }
    return results;
}