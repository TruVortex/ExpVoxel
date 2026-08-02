#include "concurrency/work_stealing_pool.hpp"
#include <atomic>
#include <cassert>
#include <iostream>

void test_scheduler_stress() {
    constexpr size_t NUM_TASKS = 100000;
    std::atomic<size_t> executed_count{0};
    {
        WorkStealingPool pool(16);
        for (size_t i = 0; i < NUM_TASKS; ++i) {
            pool.enqueue([&executed_count] {
                executed_count.fetch_add(1, std::memory_order_relaxed);
            });
        }
        pool.wait_all();
    }
    assert(executed_count.load() == NUM_TASKS);
    std::cout << "PASSED: test_scheduler_stress\n";
}

int main() {
    std::cout << "Running lock-free tests...\n";
    test_scheduler_stress();
    std::cout << "All lock-free tests passed!\n";
    return 0;
}