#ifndef BENCHMARKS_H
#define BECNHMARKS_H

#include <iostream>
#include <assert.h>
#include <vector>
#include <chrono>
#include <cassert>

#include "GeneralPurposeAllocator.h"
#include "PoolAllocator.h"
#include "StackAllocator.h"
#include "SegregatedListAllocator_2.h"

void print_benchmarks_header(const std::string& test_name);

void run_general_purpose_allocator_benchmarks();
void run_pool_allocator_benchmarks();
void run_stack_allocator_benchmarks();
void run_segregated_list_allocator_benchmarks();

#endif // !BENCHMARKS_H


