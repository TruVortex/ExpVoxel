#include "rendering/dda.hpp"
#include "rendering/simd_dda.hpp"
#include <cassert>
#include <iostream>
#include <random>

void test_simd_vs_scalar_equivalence() {
    Grid grid(4, 4, 4);
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> pos_dist(0, 31);
    for (int i = 0; i < 200; ++i) {
        grid.set_voxel(pos_dist(rng), pos_dist(rng), pos_dist(rng), 255, 128,
                       64);
    }
    std::uniform_real_distribution<float> orig_dist(0.0f, 31.0f);
    std::uniform_real_distribution<float> dir_dist(-1.0f, 1.0f);
    constexpr int NUM_PACKETS = 1000;
    for (int p = 0; p < NUM_PACKETS; ++p) {
        alignas(32) RayPacket8 packet{};
        packet.active_mask = 0xFF;
        Ray scalar_rays[8];
        for (int i = 0; i < 8; ++i) {
            packet.orig_x[i] = orig_dist(rng);
            packet.orig_y[i] = orig_dist(rng);
            packet.orig_z[i] = orig_dist(rng);
            Vec3 d{dir_dist(rng), dir_dist(rng), dir_dist(rng)};
            d = d.normalized();
            if (d.length() == 0.0f) {
                d = {0.0f, 0.0f, 1.0f};
            }
            packet.dir_x[i] = d.x;
            packet.dir_y[i] = d.y;
            packet.dir_z[i] = d.z;
            scalar_rays[i] =
                Ray{{packet.orig_x[i], packet.orig_y[i], packet.orig_z[i]}, d};
        }
        auto simd_results = trace_dda_simd_packet(packet, grid);
        for (int i = 0; i < 8; ++i) {
            HitResult scalar_res = trace_dda_scalar(scalar_rays[i], grid);
            assert(simd_results[i].hit == scalar_res.hit);
            if (simd_results[i].hit) {
                assert(simd_results[i].voxel_x == scalar_res.voxel_x);
                assert(simd_results[i].voxel_y == scalar_res.voxel_y);
                assert(simd_results[i].voxel_z == scalar_res.voxel_z);
            }
        }
    }
    std::cout << "PASSED: test_simd_vs_scalar_equivalence\n";
}

int main() {
    std::cout << "Running SIMD tests...\n";
    test_simd_vs_scalar_equivalence();
    std::cout << "All SIMD tests passed!\n";
    return 0;
}