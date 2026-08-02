#include "rendering/dda.hpp"
#include "world/grid.hpp"
#include <cassert>
#include <iostream>

void test_direct_hit() {
    Grid grid(2, 2, 2);
    grid.set_voxel(5, 5, 5, 255, 0, 0);
    Ray ray{{5.5f, 5.5f, -2.0f}, {0.0f, 0.0f, 1.0f}};
    HitResult result = trace_dda_scalar(ray, grid);
    assert(result.hit == true);
    assert(result.voxel_x == 5 && result.voxel_y == 5 && result.voxel_z == 5);
    assert(result.colour_r == 255);
    std::cout << "PASSED: test_direct_hit\n";
}

void test_miss() {
    Grid grid(2, 2, 2);
    grid.set_voxel(5, 5, 5, 255, 0, 0);
    Ray ray{{0.5f, 0.5f, -2.0f}, {0.0f, 1.0f, 0.0f}};
    HitResult result = trace_dda_scalar(ray, grid);
    assert(result.hit == false);
    std::cout << "PASSED: test_miss\n";
}

int main() {
    std::cout << "Running DDA tests...\n";
    test_direct_hit();
    test_miss();
    std::cout << "All DDA tests passed successfully!\n";
    return 0;
}