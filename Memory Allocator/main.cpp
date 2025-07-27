#include <iostream>
#include <vector>
#include <chrono>
#include <numeric>
#include <cassert>
#include "tests.h"
#include "benchmarks.h"





int main() {
    
    run_general_purpose_allocator_benchmarks();
    run_segregated_list_allocator_benchmarks();

    return 0;
}