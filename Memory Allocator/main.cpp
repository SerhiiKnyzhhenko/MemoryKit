#include <iostream>
#include "IAllocator.h"
#include <assert.h>


#include "GeneralPurposeAllocator.h" // <-- Замените на имя вашего файла
#include "PoolAllocator.h"
#include "StackAllocator.h"




int main() {
    
    StackAllocator stack(1000);
    
    size_t* p = static_cast<size_t*>(stack.allocate(1));

    stack.pop();



    return 0;
}