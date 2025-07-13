#ifndef GENERALPURPOSEALLOCATOR_H
#define GENERALPURPOSEALLOCATOR_H

#include "IAllocator.h"
#include <cstddef> // Для size_t
#include <cstdint> // Для uintptr_t
#include <iostream>
#include "Block.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

class GeneralPurposeAllocator : public IAllocator {

private:
    Block* m_free_list_head_ = nullptr;
    void* m_start_ = nullptr;
    void* m_current_ = nullptr;
    size_t m_total_size_ = 0;

public:
    GeneralPurposeAllocator(size_t size);
    ~GeneralPurposeAllocator();

    void* allocate(size_t requested_size) override;
    void deallocate(void* user_data_ptr) override;

private:
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

#endif // !GENERALPURPOSEALLOCATOR_H
