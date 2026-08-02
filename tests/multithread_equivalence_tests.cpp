#include "concurrency/work_stealing_pool.hpp"
#include "rendering/tile_renderer.hpp"
#include "world/grid.hpp"
#include <cassert>
#include <iostream>
#include <vector>

void test_multithread_equivalence() {
    constexpr int WIDTH = 320;
    constexpr int HEIGHT = 240;
    Grid grid(4, 4, 4);
    for (int x = 0; x < 32; ++x) {
        for (int y = 0; y < 32; ++y) {
            grid.set_voxel(x, y, 0, 200, 100, 50);
            grid.set_voxel(x, y, 31, 50, 100, 200);
        }
    }
    Vec3 cam_pos{16.0f, 16.0f, -20.0f};
    float fov = 60.0f * (3.14159f / 180.0f);
    std::vector<uint32_t> buf_single(WIDTH * HEIGHT, 0);
    std::vector<uint32_t> buf_multi(WIDTH * HEIGHT, 0);
    WorkStealingPool pool_single(1);
    render_tiles_multithreaded(pool_single, grid, buf_single.data(), WIDTH,
                               HEIGHT, cam_pos, fov);
    WorkStealingPool pool_multi(8);
    render_tiles_multithreaded(pool_multi, grid, buf_multi.data(), WIDTH,
                               HEIGHT, cam_pos, fov);
    for (size_t i = 0; i < WIDTH * HEIGHT; ++i) {
        assert(buf_single[i] == buf_multi[i]);
    }
    std::cout << "PASSED: test_multithread_equivalence\n";
}

int main() {
    std::cout << "Running multithread equivalence tests...\n";
    test_multithread_equivalence();
    std::cout << "All multithread equivalence tests passed!\n";
    return 0;
}