#pragma once
#include <cmath>
#include <cstdint>

struct Vec3 {
    float x{0.0f}, y{0.0f}, z{0.0f};

    Vec3 operator+(const Vec3 &v) const { return {x + v.x, y + v.y, z + v.z}; }
    Vec3 operator-(const Vec3 &v) const { return {x - v.x, y - v.y, z - v.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }

    float length() const { return std::sqrt(x * x + y * y + z * z); }
    Vec3 normalized() const {
        float len = length();
        return len > 0.0f ? *this * (1.0f / len) : Vec3{0, 0, 0};
    }
};

struct Ray {
    Vec3 origin;
    Vec3 dir;
};

struct alignas(32) RayPacket8 {
    float orig_x[8], orig_y[8], orig_z[8];
    float dir_x[8], dir_y[8], dir_z[8];
    uint32_t active_mask{0xFF};
};

struct HitResult {
    bool hit{false};
    int voxel_x{-1}, voxel_y{-1}, voxel_z{-1};
    float t_hit{0.0f};
    uint8_t colour_r{0}, colour_g{0}, colour_b{0};
};