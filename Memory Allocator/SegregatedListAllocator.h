#ifndef SEGREGATED_LIST_ALLOCATOR_H
#define SEGREGATED_LIST_ALLOCATOR_H

#include <cstddef> // Для size_t
#include <cstdint> // Для uintptr_t
#include <iostream>
#include <vector>
#include <stdexcept>
#include <intrin.h >
#include "Block.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif


template<typename T>
class SegregatedListAllocator {

private:
    std::vector<Block*> m_free_lists_;
    void* m_start_ = nullptr;
    size_t m_total_size_ = 0;
    static constexpr size_t LARGE_ALLOC_THRESHOLD = 128 * 1024;
    static constexpr size_t ALIGNMENT = 16;

public:
    SegregatedListAllocator(size_t size);
    ~SegregatedListAllocator();

    T* allocate(size_t n);
    void deallocate(T* user_data_ptr);
    void deallocate(T* user_data_ptr, size_t n);

private:
    T* allocate_from_free_list(size_t requested_size);
    T* allocate_large_block(size_t required_size);
    Block* split_block(Block* block_to_split, size_t required_size);
    void unlink_from_freelist(Block* block_to_remove, size_t index);
    Block* coalesce(Block* block);
    Block* merge_with_left_block(Block* block);
    void merge_with_right_block(Block* block);
    void add_to_freelist(Block* block, size_t index);
    void update_footer(Block* block) const;
    size_t find_list_index(const size_t size) const;

};

#include "SegregatedListAllocator.tpp"

#endif // !SEGREGATED_LIST_ALLOCATOR_H