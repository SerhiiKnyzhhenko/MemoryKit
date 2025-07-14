#include "tests.h"

// Helper function to print test headers
void print_test_header(const std::string& test_name) {
    std::cout << "\n--- " << test_name << " ---\n";
}


//-------------------------- TESTS FOR StackAllocator --------------------------//

/**
 * @brief Tests that basic allocation works and that clear() resets the allocator.
 * @details It allocates some memory, clears the allocator, and then allocates again.
 * The pointer from the first and third allocations should be identical.
 */
void test_stack_alloc_and_clear() {
    print_test_header("Test 1: Basic Allocation & clear()");
    StackAllocator alloc(1024);

    // Arrange & Act
    void* p1 = alloc.allocate(100);
    alloc.allocate(200);
    alloc.clear();
    void* p2 = alloc.allocate(100);

    // Assert
    assert(p1 == p2 && "clear() did not reset the stack pointer!");
    std::cout << "  PASS" << std::endl;
}

/**
 * @brief Tests the LIFO (Last-In, First-Out) deallocation via pop().
 * @details It allocates two blocks, pops the second one, and then re-allocates
 * a block of the same size. The address should be the same.
 */
void test_stack_alloc_pop() {
    print_test_header("Test 2: LIFO pop()");
    StackAllocator alloc(1024);

    // Arrange & Act
    alloc.allocate(100); // First allocation
    void* p2_before = alloc.allocate(200); // The allocation we will pop
    alloc.pop(); // Pop the last allocation
    void* p2_after = alloc.allocate(200); // Re-allocate with the same size

    // Assert
    assert(p2_before == p2_after && "pop() did not roll back the stack correctly!");
    std::cout << "  PASS" << std::endl;
}

/**
 * @brief Tests the out-of-memory behavior.
 * @details Ensures the allocator returns nullptr when a request cannot be satisfied,
 * without crashing.
 */
void test_stack_out_of_memory() {
    print_test_header("Test 3: Out of Memory");
    StackAllocator alloc(256);

    // Arrange & Act
    void* p1 = alloc.allocate(200);
    void* p2 = alloc.allocate(100); // This allocation should fail

    // Assert
    assert(p1 != nullptr && "Initial allocation failed!");
    assert(p2 == nullptr && "Allocator did not return nullptr when out of memory!");
    std::cout << "  PASS" << std::endl;
}


// --- Main test runner function ---
void run_stack_allocator_tests() {
    print_test_header("Testing StackAllocator");
    test_stack_alloc_and_clear();
    test_stack_alloc_pop();
    test_stack_out_of_memory();
}



//-------------------------- TESTS FOR GeneralPurposeAllocator --------------------------//

/**
 * @brief Tests a simple cycle of allocations and deallocations.
 * @details This test ensures that after all blocks are freed, they coalesce back into a single large block,
 * allowing a subsequent large allocation to succeed.
 */
void test_simple_cycle() {
    print_test_header("Test 1: Simple Allocate/Free Cycle & Coalescing");
    GeneralPurposeAllocator alloc(1024);

    // Arrange
    void* p1 = alloc.allocate(100);
    void* p2 = alloc.allocate(200);
    void* p3 = alloc.allocate(300);

    // Act
    alloc.deallocate(p1);
    alloc.deallocate(p2);
    alloc.deallocate(p3);

    // Assert
    // If coalescing worked, we should have a single large free block.
    // A new allocation almost the size of the total arena should succeed.
    void* p4 = alloc.allocate(1000);
    assert(p4 != nullptr && "Coalescing failed, not enough contiguous space.");

    std::cout << "PASS" << std::endl;
}

/**
 * @brief Tests if the allocator reuses a freed memory block for a new allocation of the same size.
 */
void test_reuse() {
    print_test_header("Test 2: Block Reuse");
    GeneralPurposeAllocator alloc(1024);

    // Arrange
    void* p1 = alloc.allocate(128);
    alloc.allocate(128); // Allocate another block to prevent merging with the end of the arena

    // Act
    alloc.deallocate(p1);
    void* p3 = alloc.allocate(128); // Request a block of the same size

    // Assert
    assert(p1 == p3 && "Allocator did not reuse the freed block.");

    std::cout << "PASS" << std::endl;
}

/**
 * @brief Tests if a freed block correctly merges with its right neighbor.
 */
void test_coalesce_right() {
    print_test_header("Test 3: Coalesce with Right Neighbor");
    GeneralPurposeAllocator alloc(1024);

    // Arrange
    void* p1 = alloc.allocate(100);
    void* p2 = alloc.allocate(100);
    alloc.allocate(100); // Sentinel block

    // Act
    alloc.deallocate(p1);
    alloc.deallocate(p2); // Deallocating p2 should trigger a coalesce with p1 (its left neighbor)

    // Assert
    // If p1 and p2 merged, a new allocation of 200 should fit perfectly.
    void* p4 = alloc.allocate(200);
    assert(p4 != nullptr && "Right-side coalescing failed.");

    std::cout << "PASS" << std::endl;
}

/**
 * @brief Tests if a freed block correctly merges with its left neighbor.
 */
void test_coalesce_left() {
    print_test_header("Test 4: Coalesce with Left Neighbor");
    GeneralPurposeAllocator alloc(1024);

    // Arrange
    void* p1 = alloc.allocate(100);
    void* p2 = alloc.allocate(100);
    alloc.allocate(100); // Sentinel block

    // Act
    alloc.deallocate(p2);
    alloc.deallocate(p1); // Deallocating p1 should trigger a coalesce with p2 (its right neighbor)

    // Assert
    // If p1 and p2 merged, a new allocation of 200 should fit perfectly.
    void* p4 = alloc.allocate(200);
    assert(p4 != nullptr && "Left-side coalescing failed.");

    std::cout << "PASS" << std::endl;
}

/**
 * @brief Tests if a block merges with free neighbors on both sides simultaneously.
 */
void test_sandwich_coalesce() {
    print_test_header("Test 5: Sandwich Coalesce (Both Sides)");
    GeneralPurposeAllocator alloc(1024);

    // Arrange
    alloc.allocate(100); // p1, will remain allocated
    void* p2 = alloc.allocate(100);
    void* p3 = alloc.allocate(100);
    void* p4 = alloc.allocate(100);

    // Act
    alloc.deallocate(p2);
    alloc.deallocate(p4);
    alloc.deallocate(p3); // Freeing p3 should cause it to merge with p2 and p4.

    // Assert
    // A new block of 300 should now fit into the merged space.
    void* p5 = alloc.allocate(300);
    assert(p5 != nullptr && "Sandwich coalescing failed.");

    std::cout << "PASS" << std::endl;
}

/**
 * @brief Tests the separate allocation path for large blocks (>128 KB).
 */
void test_large_block_allocation() {
    print_test_header("Test 6: Large Block Allocation (>128KB)");
    GeneralPurposeAllocator alloc(1024); // Main arena size is small

    // Arrange & Act
    // This allocation should bypass the main arena and use VirtualAlloc/mmap directly.
    const size_t large_size = 256 * 1024;
    void* p_large = alloc.allocate(large_size);

    // Assert
    assert(p_large != nullptr && "Large block allocation failed.");

    // Act
    alloc.deallocate(p_large); // Should call VirtualFree/munmap.

    std::cout << "PASS" << std::endl;
}


// --- Main test runner function ---
void run_general_purpose_allocator_tests() {
    print_test_header("Testing GeneralPurposeAllocator");
    test_simple_cycle();
    test_reuse();
    test_coalesce_right();
    test_coalesce_left();
    test_sandwich_coalesce();
    test_large_block_allocation();
}




//-------------------------- TESTS FOR PoolAllocator --------------------------//

/**
 * @brief Tests the out-of-memory condition.
 * @details Allocates all available chunks from the pool and then asserts
 * that the next allocation request returns nullptr.
 */
void test_pool_out_of_memory() {
    print_test_header("Test 1: Out of Memory");
    PoolAllocator alloc(64, 10);

    // Arrange & Act: Exhaust the pool
    for (size_t i = 0; i < 10; i++) {
        alloc.allocate(1);
    }

    // Act: Request one more chunk
    void* p = alloc.allocate(1);

    // Assert: The pointer must be null
    assert(p == nullptr && "Allocator did not return nullptr when pool was empty!");
    std::cout << "  PASS" << std::endl;
}

/**
 * @brief Tests the basic allocate-deallocate-allocate cycle.
 * @details Verifies that a deallocated chunk is correctly returned to the
 * front of the free list and is reused on the next allocation.
 */
void test_allocate_deallocate_reuse() {
    print_test_header("Test 2: Reuse Freed Chunk");
    PoolAllocator alloc(64, 10);

    // Arrange
    void* p1 = alloc.allocate(1);

    // Act
    alloc.deallocate(p1);
    void* p2 = alloc.allocate(1);

    // Assert: The same memory address should be returned
    assert(p1 == p2 && "Allocator did not reuse the chunk!");
    std::cout << "  PASS" << std::endl;
}

/**
 * @brief Tests a full lifecycle of the pool.
 * @details Allocates all chunks, deallocates all of them, and then
 * re-allocates them all again to ensure the allocator state is not corrupted.
 */
void test_full_cycle() {
    print_test_header("Test 3: Full Lifecycle");
    PoolAllocator alloc(64, 10);
    std::vector<void*> allocated_pointers;
    allocated_pointers.reserve(10);

    // Arrange: Allocate all chunks
    for (size_t i = 0; i < 10; i++) {
        allocated_pointers.push_back(alloc.allocate(1));
    }

    // Act: Deallocate all chunks
    for (void* p : allocated_pointers) {
        alloc.deallocate(p);
    }

    // Assert: All 10 chunks should be available for allocation again
    for (size_t i = 0; i < 10; i++) {
        void* p = alloc.allocate(1);
        assert(p != nullptr && "A chunk was lost after a full deallocation cycle!");
    }

    std::cout << "  PASS" << std::endl;
}

/**
 * @brief Tests robustness against double deallocation.
 * @details Ensures that freeing the same pointer twice does not corrupt
 * the internal free list, which could lead to crashes or memory leaks.
 */
void test_double_free() {
    print_test_header("Test 4: Double Free");
    PoolAllocator alloc(64, 10);

    // Arrange
    void* p1 = alloc.allocate(1);

    // Act
    alloc.deallocate(p1);
    alloc.deallocate(p1); // Second deallocation should be handled gracefully

    // Assert: We should still be able to allocate all 10 chunks.
    // If the double free corrupted the list (e.g., created a cycle), this loop would fail.
    for (size_t i = 0; i < 10; i++) {
        void* p = alloc.allocate(1);
        assert(p != nullptr && "Double free corrupted the free list!");
    }

    std::cout << "  PASS" << std::endl;
}

// --- Main test runner function ---
void run_pool_allocator_tests() {
    std::cout << "\n--- Testing PoolAllocator ---";
    test_pool_out_of_memory();
    test_allocate_deallocate_reuse();
    test_full_cycle();
    test_double_free();
}