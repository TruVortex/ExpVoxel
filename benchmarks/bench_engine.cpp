#include "concurrency/work_stealing_pool.hpp"
#include "rendering/tile_renderer.hpp"
#include "world/grid.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>

int main() {
    std::cout << "Running ExpVoxel Benchmarks..." << std::endl;
    constexpr int WIDTH = 1280;
    constexpr int HEIGHT = 720;
    constexpr int FRAMES = 10;
    Grid grid(8, 8, 8);
    for (int x = 0; x < 64; ++x) {
        for (int y = 0; y < 64; ++y) {
            grid.set_voxel(x, y, 0, 200, 50, 50);
            grid.set_voxel(x, y, 63, 50, 50, 200);
        }
    }
    Vec3 cam_pos{32.0f, 32.0f, -40.0f};
    float fov = 60.0f * (3.14159f / 180.0f);
    std::vector<uint32_t> pixels(WIDTH * HEIGHT, 0);
    std::ofstream csv("benchmark_results.csv");
    csv << "threads,avg_frame_ms,mrays_per_sec\n";
    std::vector<int> thread_counts = {1, 2, 4, 8, 16};
    for (int num_threads : thread_counts) {
        std::cout << "Testing " << num_threads << " thread(s)... "
                  << std::flush;
        WorkStealingPool pool(num_threads);
        render_tiles_multithreaded(pool, grid, pixels.data(), WIDTH, HEIGHT,
                                   cam_pos, fov);
        auto start = std::chrono::high_resolution_clock::now();
        for (int f = 0; f < FRAMES; ++f) {
            render_tiles_multithreaded(pool, grid, pixels.data(), WIDTH, HEIGHT,
                                       cam_pos, fov);
        }
        auto end = std::chrono::high_resolution_clock::now();
        double total_ms =
            std::chrono::duration<double, std::milli>(end - start).count();
        double avg_frame_ms = total_ms / FRAMES;
        double mrays_sec = (WIDTH * HEIGHT) / (avg_frame_ms * 1000.0);
        std::cout << "Done! Avg Frame: " << avg_frame_ms
                  << " ms | Throughput: " << mrays_sec << " MRays/s\n";
        csv << num_threads << "," << avg_frame_ms << "," << mrays_sec << "\n";
    }
    std::cout << "\nResults saved to benchmark_results.csv" << std::endl;
    return 0;
}