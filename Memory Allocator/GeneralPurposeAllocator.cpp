#include "GeneralPurposeAllocator.h"

GeneralPurposeAllocator::GeneralPurposeAllocator(size_t size) {

    size_t alignment = 16;
    m_total_size_ = (size + alignment - 1) & ~(alignment - 1);

#ifdef _WIN32
    m_start_ = VirtualAlloc(NULL, m_total_size_, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
    m_start = mmap(nullptr, m_total_size_, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
#endif

    m_current_ = m_start_;

    Block* initial_block = static_cast<Block*>(m_start_);
    initial_block->size_ = m_total_size_;
    initial_block->is_free_ = true;
    initial_block->free_block_pointers.next_free = nullptr;
    initial_block->free_block_pointers.prev_free = nullptr;

    m_free_list_head_ = initial_block;
}

GeneralPurposeAllocator::~GeneralPurposeAllocator() {

#ifdef _WIN32
    VirtualFree(m_start_, 0, MEM_RELEASE);
#else
    munmap(m_start_, m_total_size_);
#endif

}

void* GeneralPurposeAllocator::allocate(size_t required_size) {

    Block* current_block = find_first_fit(required_size);

    if (current_block == nullptr)
        return nullptr;

    if (current_block->size_ > required_size + sizeof(Block)) {

        Block* new_block = split_block(current_block, required_size);

        update_freelist_after_allocation(current_block, new_block);

        return (void*)current_block->user_data;
    }
    else {
        unlink_from_freelist(current_block);
        current_block->is_free_ = false;
        return (void*)current_block->user_data;
    }

}

void GeneralPurposeAllocator::deallocate(void* user_data_ptr) {

    Block* current_block = reinterpret_cast<Block*>(
        reinterpret_cast<uintptr_t>(user_data_ptr) - offsetof(Block, user_data)
        );

    #ifndef DEBUG
    if (current_block->is_free_) {
        fprintf(stderr, "Error: Double free detected on pointer %p\n", user_data_ptr);
        return;
    }
    #endif

    current_block->is_free_ = true;

    bool merging_with_the_left_bloc = false;

    current_block = coalesce(current_block, &merging_with_the_left_bloc);

    if (!merging_with_the_left_bloc) {
        add_to_freelist(current_block);
    }

}

Block* GeneralPurposeAllocator::find_first_fit(size_t required_size) {
    Block* current_block = m_free_list_head_;
    while (current_block != nullptr) {
        if (current_block->size_ >= required_size) {
            break;
        }
        current_block = current_block->free_block_pointers.next_free;
    }
    return current_block;
}

Block* GeneralPurposeAllocator::split_block(Block* block_to_split, size_t required_size) {

    size_t allocated_size = required_size + sizeof(Block);
    size_t new_block_size = block_to_split->size_ - allocated_size;

    Block* new_block = reinterpret_cast<Block*>(reinterpret_cast<uintptr_t>(block_to_split) + allocated_size);
    new_block->size_ = new_block_size;
    new_block->is_free_ = true;

    block_to_split->size_ = allocated_size;
    block_to_split->is_free_ = false;

    update_footer(block_to_split);
    update_footer(new_block);

    return new_block;
}

void GeneralPurposeAllocator::update_freelist_after_allocation(Block* old_block, Block* new_block) {

    Block* next_block = old_block->free_block_pointers.next_free;
    Block* prev_block = old_block->free_block_pointers.prev_free;

    if (prev_block != nullptr) {
        prev_block->free_block_pointers.next_free = new_block;
    }
    else {
        m_free_list_head_ = new_block;
    }

    if (next_block != nullptr)
        next_block->free_block_pointers.prev_free = new_block;


    new_block->free_block_pointers.prev_free = prev_block;
    new_block->free_block_pointers.next_free = next_block;

}

void GeneralPurposeAllocator::unlink_from_freelist(Block* block_to_remove) {

    Block* next_block = block_to_remove->free_block_pointers.next_free;
    Block* prev_block = block_to_remove->free_block_pointers.prev_free;

    if (prev_block != nullptr) {
        prev_block->free_block_pointers.next_free = next_block;
    }
    else {
        m_free_list_head_ = next_block;
    }

    if (next_block != nullptr)
        next_block->free_block_pointers.prev_free = prev_block;   
}

Block* GeneralPurposeAllocator::coalesce(Block* current_block, bool* merging_with_the_left_block) {

    if (reinterpret_cast<uintptr_t>(current_block) > reinterpret_cast<uintptr_t>(m_start_)) {
        current_block = merge_with_left_block(current_block, merging_with_the_left_block);
    }

    merge_with_right_block(current_block);

    return current_block;

}

Block* GeneralPurposeAllocator::merge_with_left_block(Block* current_block, bool* merging_with_the_left_bloc) {

    size_t* left_block_foooter = reinterpret_cast<size_t*>(
        reinterpret_cast<uintptr_t>(current_block) - sizeof(size_t)
        );

    Block* left_block = reinterpret_cast<Block*>(
        reinterpret_cast<uintptr_t>(current_block) - *left_block_foooter
        );

    if (reinterpret_cast<uintptr_t>(left_block) >= reinterpret_cast<uintptr_t>(m_start_)
        && left_block->is_free_ == true) {

        left_block->size_ += current_block->size_;

        update_footer(left_block); 
        
        current_block = left_block;

        *merging_with_the_left_bloc = true;
    }

    return current_block;

}

void GeneralPurposeAllocator::merge_with_right_block(Block* current_block) {

    Block* right_block = reinterpret_cast<Block*>(
        reinterpret_cast<uintptr_t>(current_block) + current_block->size_
        );

    if (reinterpret_cast<uintptr_t>(right_block) < reinterpret_cast<uintptr_t>(m_start_) + m_total_size_
        && right_block->is_free_ == true) {

        unlink_from_freelist(right_block);

        current_block->size_ += right_block->size_;

        update_footer(current_block);    
    }

}

void GeneralPurposeAllocator::add_to_freelist(Block* block) {

    block->free_block_pointers.next_free = m_free_list_head_;
    block->free_block_pointers.prev_free = nullptr;

    if (m_free_list_head_ != nullptr)
        m_free_list_head_->free_block_pointers.prev_free = block;

    m_free_list_head_ = block;

}

void GeneralPurposeAllocator::update_footer(Block* block) {

    size_t* current_block_footer = reinterpret_cast<size_t*>(
        reinterpret_cast<uintptr_t>(block) + block->size_ - sizeof(size_t)
        );
    *current_block_footer = block->size_;

}