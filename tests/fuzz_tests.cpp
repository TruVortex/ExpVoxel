#include "rendering/simd_dda.hpp"
#include "world/grid.hpp"
#include <atomic>
#include <cassert>
#include <iostream>
#include <random>
#include <thread>

void test_fuzz_mutation() {
    Grid grid(4, 4, 4);
    std::atomic<bool> running{true};
    std::thread mutator([&grid, &running] {
        std::mt19937 rng(1337);
        std::uniform_int_distribution<int> dist(0, 31);
        while (running.load(std::memory_order_relaxed)) {
            int x = dist(rng), y = dist(rng), z = dist(rng);
            if (grid.is_solid(x, y, z)) {
                grid.clear_voxel_atomic(x, y, z);
            } else {
                grid.set_voxel(x, y, z, 255, 0, 0);
            }
        }
    });
    std::vector<std::thread> readers;
    for (int t = 0; t < 8; ++t) {
        readers.emplace_back([&grid, t] {
            std::mt19937 rng(t);
            std::uniform_real_distribution<float> orig_dist(0.0f, 31.0f);
            std::uniform_real_distribution<float> dir_dist(-1.0f, 1.0f);
            alignas(32) RayPacket8 packet{};
            packet.active_mask = 0xFF;
            for (int i = 0; i < 10000; ++i) {
                for (int r = 0; r < 8; ++r) {
                    packet.orig_x[r] = orig_dist(rng);
                    packet.orig_y[r] = orig_dist(rng);
                    packet.orig_z[r] = orig_dist(rng);
                    packet.dir_x[r] = dir_dist(rng);
                    packet.dir_y[r] = dir_dist(rng);
                    packet.dir_z[r] = dir_dist(rng);
                }
                auto results = trace_dda_simd_packet(packet, grid);
                (void)results;
            }
        });
    }
    for (auto &r : readers) {
        r.join();
    }
    running.store(false);
    mutator.join();
    std::cout << "PASSED: test_fuzz_mutation\n";
}

int main() {
    std::cout << "Running mutation tests...\n";
    test_fuzz_mutation();
    std::cout << "All mutation tests passed!\n";
    return 0;
}