#include "memory/epoch.hpp"
#include <cassert>
#include <iostream>

struct DummyNode {
    int value{42};
};

void test_epoch_reclamation() {
    EpochManager epoch_mgr;
    size_t tid1 = epoch_mgr.register_thread();
    DummyNode *node = new DummyNode();
    epoch_mgr.defer_delete(node);
    assert(epoch_mgr.pending_deletions() == 1);
    epoch_mgr.enter_epoch(tid1);
    epoch_mgr.gc();
    assert(epoch_mgr.pending_deletions() == 1);
    epoch_mgr.leave_epoch(tid1);
    epoch_mgr.gc();
    assert(epoch_mgr.pending_deletions() == 0);
    std::cout << "PASSED: test_epoch_reclamation\n";
}

int main() {
    std::cout << "Running EBMR tests...\n";
    test_epoch_reclamation();
    std::cout << "All EMBR tests passed!\n";
    return 0;
}