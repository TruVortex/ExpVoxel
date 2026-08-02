#include "memory/arena.hpp"
#include "world/brick.hpp"
#include <cassert>
#include <iostream>

void test_arena_alignment() {
    Arena arena(1024 * 1024);
    void *p1 = arena.alloc(10, 64);
    void *p2 = arena.alloc(100, 64);
    assert(reinterpret_cast<uintptr_t>(p1) % 64 == 0);
    assert(reinterpret_cast<uintptr_t>(p2) % 64 == 0);
    assert(arena.used() > 0);
    arena.reset();
    assert(arena.used() == 0);
    std::cout << "PASSED: test_arena_alignment\n";
}

void test_brick_alignment() {
    assert(sizeof(Brick) == 1600);
    assert(sizeof(Brick) % 64 == 0);
    assert(alignof(Brick) == 64);
    std::cout << "PASSED: test_brick_alignment\n";
}

int main() {
    std::cout << "Running memory tests...\n";
    test_arena_alignment();
    test_brick_alignment();
    std::cout << "All memory tests passed!\n";
    return 0;
}