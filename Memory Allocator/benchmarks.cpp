#include "benchmarks.h"

void print_benchmarks_header(const std::string& test_name) {
    std::cout << "\n--- " << test_name << " ---\n";
}

void benchmark_std_vector() {
    print_benchmarks_header("Benchmarking std::vector with std::allocator");

    auto start = std::chrono::high_resolution_clock::now();

    std::vector<int> vec;
    for (int i = 0; i < 1000; ++i) {
        vec.reserve(10000);
        for (int j = 0; j < 10000; ++j) {
            vec.push_back(j);
        }
        vec.clear();
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "Total time: " << duration.count() << " ms" << std::endl;
}


void benchmark_custom_vector() {
    print_benchmarks_header("Benchmarking std::vector with GeneralPurposeAllocator");

    auto start = std::chrono::high_resolution_clock::now();

    std::vector<int, GeneralPurposeAllocator<int>> vec;
    for (int i = 0; i < 1000; ++i) {
        vec.reserve(10000);
        for (int j = 0; j < 10000; ++j) {
            vec.push_back(j);
        }
        vec.clear();
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "Total time: " << duration.count() << " ms" << std::endl;
}

void run_general_purpose_allocator_benchmarks() {
    benchmark_std_vector();
    benchmark_custom_vector();
}
