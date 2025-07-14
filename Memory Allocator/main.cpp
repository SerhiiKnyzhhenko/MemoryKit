#include <iostream>
#include <vector>
#include <chrono>
#include <numeric>


#include "GeneralPurposeAllocator.h"
#include "PoolAllocator.h"
#include "StackAllocator.h"



int main() {
   
    StackAllocator stack = StackAllocator(1000);

    char* p = (char*)stack.allocate(100);
    void* p1 = stack.allocate(800);

    *p = 'c';

     stack.pop();
    

    return 0;
}