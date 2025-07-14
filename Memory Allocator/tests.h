#ifndef TESTS_H
#define TESTS_H

#include <iostream>
#include <assert.h>

#include "GeneralPurposeAllocator.h"
#include "PoolAllocator.h"
#include "StackAllocator.h"

void print_test_header(const std::string& test_name);

void run_general_purpose_allocator_tests();
void run_pool_allocator_tests();
void run_stack_allocator_tests();

#endif // !TESTS_H


