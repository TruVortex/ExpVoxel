#include "rendering/dda.hpp"
#include <cmath>
#include <limits>

HitResult trace_dda_scalar(const Ray &ray, const Grid &grid) {
    HitResult result{};
    int map_x = static_cast<int>(std::floor(ray.origin.x));
    int map_y = static_cast<int>(std::floor(ray.origin.y));
    int map_z = static_cast<int>(std::floor(ray.origin.z));
    int step_x = (ray.dir.x >= 0) ? 1 : -1;
    int step_y = (ray.dir.y >= 0) ? 1 : -1;
    int step_z = (ray.dir.z >= 0) ? 1 : -1;
    float t_delta_x = (std::abs(ray.dir.x) < 1e-6f)
                          ? std::numeric_limits<float>::infinity()
                          : std::abs(1.0f / ray.dir.x);
    float t_delta_y = (std::abs(ray.dir.y) < 1e-6f)
                          ? std::numeric_limits<float>::infinity()
                          : std::abs(1.0f / ray.dir.y);
    float t_delta_z = (std::abs(ray.dir.z) < 1e-6f)
                          ? std::numeric_limits<float>::infinity()
                          : std::abs(1.0f / ray.dir.z);
    float t_max_x =
        (ray.dir.x > 0.0f)   ? (map_x + 1.0f - ray.origin.x) * t_delta_x
        : (ray.dir.x < 0.0f) ? (ray.origin.x - map_x) * t_delta_x
                             : std::numeric_limits<float>::infinity();
    float t_max_y =
        (ray.dir.y > 0.0f)   ? (map_y + 1.0f - ray.origin.y) * t_delta_y
        : (ray.dir.y < 0.0f) ? (ray.origin.y - map_y) * t_delta_y
                             : std::numeric_limits<float>::infinity();
    float t_max_z =
        (ray.dir.z > 0.0f)   ? (map_z + 1.0f - ray.origin.z) * t_delta_z
        : (ray.dir.z < 0.0f) ? (ray.origin.z - map_z) * t_delta_z
                             : std::numeric_limits<float>::infinity();
    constexpr int MAX_STEPS = 200;
    int steps = 0;
    while (steps < MAX_STEPS) {
        if (grid.in_bounds(map_x, map_y, map_z) &&
            grid.is_solid(map_x, map_y, map_z)) {
            result.hit = true;
            result.voxel_x = map_x;
            result.voxel_y = map_y;
            result.voxel_z = map_z;
            grid.get_colour(map_x, map_y, map_z, result.colour_r, result.colour_g,
                           result.colour_b);
            return result;
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
    return result;
}