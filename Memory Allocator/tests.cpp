#include "tests.h"

//// Вспомогательная функция для печати заголовков тестов
//void print_test_header(const std::string& test_name) {
//    std::cout << "\n--- " << test_name << " ---\n";
//}
//
//// Тест 1: Простой цикл выделения и освобождения. Проверяет базовую работу и слияние в один блок.
//void test_simple_cycle() {
//    print_test_header("Test 1: Simple Allocate/Free Cycle & Coalescing");
//    GeneralPurposeAllocator alloc(1024);
//
//    void* p1 = alloc.allocate(100);
//    void* p2 = alloc.allocate(200);
//    void* p3 = alloc.allocate(300);
//
//    std::cout << "Allocated p1: " << p1 << std::endl;
//    std::cout << "Allocated p2: " << p2 << std::endl;
//    std::cout << "Allocated p3: " << p3 << std::endl;
//
//    alloc.deallocate(p1);
//    alloc.deallocate(p2);
//    alloc.deallocate(p3);
//
//    std::cout << "All blocks freed. Should result in one large free block." << std::endl;
//    // В отладчике m_free_list_head->size_ должен быть равен m_totalSize
//}
//
//// Тест 2: Повторное использование освобождённого блока.
//void test_reuse() {
//    print_test_header("Test 2: Block Reuse");
//    GeneralPurposeAllocator alloc(1024);
//
//    void* p1 = alloc.allocate(128);
//    void* p2 = alloc.allocate(128);
//    std::cout << "Allocated p1 at: " << p1 << std::endl;
//    std::cout << "Allocated p2 at: " << p2 << std::endl;
//
//    alloc.deallocate(p1); // Освобождаем первый блок
//    std::cout << "Freed p1." << std::endl;
//
//    void* p3 = alloc.allocate(128); // Запрашиваем блок такого же размера
//    std::cout << "Allocated p3 at: " << p3 << std::endl;
//
//    if (p1 == p3) {
//        std::cout << "PASS: Allocator correctly reused the freed block." << std::endl;
//    }
//    else {
//        std::cout << "FAIL: Allocator did not reuse the freed block." << std::endl;
//    }
//}
//
//// Тест 3: Слияние с правым соседом.
//void test_coalesce_right() {
//    print_test_header("Test 3: Coalesce with Right Neighbor");
//    GeneralPurposeAllocator alloc(1024);
//
//    void* p1 = alloc.allocate(100);
//    void* p2 = alloc.allocate(100);
//    void* p3 = alloc.allocate(100);
//
//    alloc.deallocate(p2);
//    alloc.deallocate(p1); // Должен объединиться с p2
//
//    std::cout << "Freed p2, then p1. Should coalesce." << std::endl;
//    // В отладчике должен быть один свободный блок размером (100+Hdr) + (100+Hdr)
//}
//
//// Тест 4: Слияние с левым соседом.
//void test_coalesce_left() {
//    print_test_header("Test 4: Coalesce with Left Neighbor");
//    GeneralPurposeAllocator alloc(1024);
//
//    void* p1 = alloc.allocate(100);
//    void* p2 = alloc.allocate(100);
//    void* p3 = alloc.allocate(100);
//
//    alloc.deallocate(p1);
//
//    // Установите здесь точку останова. m_free_list_head должен указывать на блок p1
//    alloc.deallocate(p2); // Должен найти p1 слева и объединиться с ним
//
//    std::cout << "Freed p1, then p2. Should coalesce." << std::endl;
//    // Результат должен быть таким же, как в тесте 3
//}
//
//// Тест 5: Слияние в обе стороны ("Сэндвич").
//void test_sandwich_coalesce() {
//    print_test_header("Test 5: Sandwich Coalesce (Both Sides)");
//    GeneralPurposeAllocator alloc(1024);
//
//    void* p1 = alloc.allocate(100);
//    void* p2 = alloc.allocate(100);
//    void* p3 = alloc.allocate(100);
//    void* p4 = alloc.allocate(100);
//
//    alloc.deallocate(p2); // Освобождаем "начинку"
//    alloc.deallocate(p4); // Освобождаем блок справа от "хлеба"
//
//    // Установите здесь точку останова. В списке свободных должны быть блоки p2 и p4.
//    alloc.deallocate(p3); // Освобождаем "хлеб". Должен слиться с p2 и p4.
//
//    std::cout << "Freed p2, p4, then p3. Should result in a 3-block merge." << std::endl;
//    // В отладчике должен появиться большой свободный блок размером ~3*(100+Hdr)
//}
//
//void test_GeneralPurposeAllocator() {
//
//    
//
//}