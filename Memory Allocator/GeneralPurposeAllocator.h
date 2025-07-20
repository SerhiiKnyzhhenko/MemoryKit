#ifndef GENERALPURPOSEALLOCATOR_H
#define GENERALPURPOSEALLOCATOR_H

#include <cstddef> // Для size_t
#include <cstdint> // Для uintptr_t
#include <iostream>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

struct Block {
    size_t size_;
    bool is_free_;
    bool is_mmapped_;

    union {
        struct {
            Block* next_free;
            Block* prev_free;
        }free_block_pointers;
        char user_data[1];
    };
};

template<typename T>
class GeneralPurposeAllocator {

private:
    Block* m_free_list_head_ = nullptr;
    void* m_start_ = nullptr;
    size_t m_total_size_ = 0;
    const size_t LARGE_ALLOC_THRESHOLD = 128 * 1024;
    const size_t ALIGNMENT = 16;

public:
    GeneralPurposeAllocator(size_t size);
    ~GeneralPurposeAllocator();

    T* allocate(size_t n);
    void deallocate(T* user_data_ptr);
    void deallocate(T* user_data_ptr, size_t n);

private:
    T* allocate_from_free_list(size_t requested_size);
    T* allocate_large_block(size_t required_size);
    Block* find_first_fit(size_t required_size);
    Block* split_block(Block* block_to_split, size_t required_size);
    void update_freelist_after_allocation(Block* old_block, Block* new_block);
    void unlink_from_freelist(Block* block_to_remove);
    Block* coalesce(Block* block, bool* merging_with_the_left_block);
    Block* merge_with_left_block(Block* block, bool* merging_with_the_left_bloc);
    void merge_with_right_block(Block* block);
    void add_to_freelist(Block* block);
    void update_footer(Block* block);

};

#include "GeneralPurposeAllocator.tpp"

#endif // !GENERALPURPOSEALLOCATOR_H
