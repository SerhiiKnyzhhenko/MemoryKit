#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <string>
#include <numeric>
#include "tests.h"
#include "benchmarks.h"





template<typename TAllocator>
void run_fragmentation_benchmark(TAllocator& alloc, const std::string& allocator_name) {
    std::cout << "\n--- Benchmarking " << allocator_name << " (Fragmentation Test) ---" << std::endl;

    const int NUM_ITERATIONS = 2000;
    const int ALLOCS_PER_ITER = 1000;
    const std::vector<size_t> SIZES = { 8, 16, 24, 32, 48, 64, 96, 128 };

    std::vector<void*> pointers;
    pointers.reserve(ALLOCS_PER_ITER);

    std::mt19937 rng(12345); 
    std::uniform_int_distribution<size_t> size_dist(0, SIZES.size() - 1);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_ITERATIONS; ++i) {

        for (int j = 0; j < ALLOCS_PER_ITER; ++j) {
            pointers.push_back(alloc.allocate(SIZES[size_dist(rng)]));
        }


        for (size_t j = 0; j < pointers.size(); j += 2) {
            alloc.deallocate((char*)pointers[j], 1);
            pointers[j] = nullptr;
        }


        for (size_t j = 0; j < pointers.size(); j += 2) {
            pointers[j] = alloc.allocate(SIZES[size_dist(rng)]);
        }
        int a = 0;
   
        for (void* p : pointers) {
            if (p != nullptr) { 
                alloc.deallocate((char*)p, 1);
                a += 1;
            }
        }
        pointers.clear();
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "Total time: " << duration.count() << " ms" << std::endl;
}


void benchmark_standard_new_fragmentation() {
    std::cout << "\n--- Benchmarking std::allocator (new/delete) (Fragmentation Test) ---" << std::endl;

    const int NUM_ITERATIONS = 2000;
    const int ALLOCS_PER_ITER = 1000;
    const std::vector<size_t> SIZES = { 8, 16, 24, 32, 48, 64, 96, 128 };

    std::vector<void*> pointers;
    pointers.reserve(ALLOCS_PER_ITER);

    std::mt19937 rng(12345);
    std::uniform_int_distribution<size_t> size_dist(0, SIZES.size() - 1);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_ITERATIONS; ++i) {

        for (int j = 0; j < ALLOCS_PER_ITER; ++j) {
            pointers.push_back(new char[SIZES[size_dist(rng)]]);
        }


        for (size_t j = 0; j < pointers.size(); j += 2) {
            delete[] static_cast<char*>(pointers[j]);
        }

    
        for (size_t j = 0; j < pointers.size(); j += 2) {
            pointers[j] = new char[SIZES[size_dist(rng)]];
        }

    
        for (void* p : pointers) {
            delete[] static_cast<char*>(p);
        }
        pointers.clear();
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "Total time: " << duration.count() << " ms" << std::endl;
}


int main() {
    
    GeneralPurposeAllocator<char> gp_alloc(5 * 1024 * 1024); // 5 MB
    SegregatedListAllocator<char> seg_alloc(5 * 1024 * 1024); // 5 MB

   
    benchmark_standard_new_fragmentation();
    run_fragmentation_benchmark(seg_alloc, "SegregatedListAllocator");
    run_fragmentation_benchmark(gp_alloc, "GeneralPurposeAllocator");

    return 0;
}