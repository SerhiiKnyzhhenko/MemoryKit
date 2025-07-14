#ifndef TESTS_H
#define TESTS_H

#include <iostream>
#include <assert.h>

void print_test_header(const std::string& test_name);

void test_GeneralPurposeAllocator();
void test_PoolAllocator();
void test_StackAllocator();

#endif // !TESTS_H


