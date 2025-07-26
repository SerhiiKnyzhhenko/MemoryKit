
template<typename T>
SegregatedListAllocator<T>::SegregatedListAllocator(size_t size) {
    m_total_size_ = (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);

#ifdef _WIN32
    m_start_ = VirtualAlloc(NULL, m_total_size_, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (m_start_ == NULL) {
        throw std::bad_alloc();
    }
#else
    m_start = mmap(nullptr, m_total_size_, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (m_start_ == MAP_FAILED) {
        throw std::bad_alloc();
    }
#endif

    Block* initial_block = static_cast<Block*>(m_start_);
    initial_block->size_ = m_total_size_;
    initial_block->is_free_ = true;
    initial_block->is_mmapped_ = false;
    initial_block->free_block_pointers.next_free = nullptr;
    initial_block->free_block_pointers.prev_free = nullptr;

    m_free_lists_.resize(14, nullptr);

    size_t index = find_list_index(m_total_size_);
    m_free_lists_[index] = initial_block;

}

template<typename T>
SegregatedListAllocator<T>::~SegregatedListAllocator() {
#ifdef _WIN32
    VirtualFree(m_start_, 0, MEM_RELEASE);
#else
    munmap(m_start_, m_total_size_);
#endif
}

template<typename T>
T* SegregatedListAllocator<T>::allocate(size_t n) {
    size_t required_size = n * sizeof(T);

    if (required_size <= LARGE_ALLOC_THRESHOLD)
        return allocate_from_free_list(required_size);
    else
        return allocate_large_block(required_size);
}

template<typename T>
void SegregatedListAllocator<T>::deallocate(T* user_data_ptr) {
    deallocate(user_data_ptr, 1);
}

template<typename T>
void SegregatedListAllocator<T>::deallocate(T* user_data_ptr, size_t n) {
    Block* current_block = reinterpret_cast<Block*>((char*)user_data_ptr - offsetof(Block, user_data));

    if (current_block->is_mmapped_) {

#ifdef _WIN32
        VirtualFree(reinterpret_cast<void*>(current_block), 0, MEM_RELEASE);
        return;
#else
        munmap(reinterpret_cast<void*>(current_block), current_block->size_);
        return;
#endif

    }

    current_block->is_free_ = true;

    current_block = coalesce(current_block);
    size_t index = find_list_index(current_block->size_);
    add_to_freelist(current_block, index);
}

template<typename T>
T* SegregatedListAllocator<T>::allocate_from_free_list(size_t required_size) {
    size_t index = find_list_index(required_size);

    size_t found_index = 0;
    Block* found_block = nullptr;

    for (size_t i = index; i < m_free_lists_.size(); i++) {
        if (m_free_lists_[i] != nullptr) {
            found_block = m_free_lists_[i];
            found_index = i;
            break;
        }        
    }
    if (found_block == nullptr)
        return nullptr;

    unlink_from_freelist(found_block, found_index);

    if ((found_block->size_ - required_size) > sizeof(Block)) {

        Block* remainder_block = split_block(found_block, required_size);
        size_t remainder_index = find_list_index(remainder_block->size_);
        add_to_freelist(remainder_block, remainder_index);
  
        return (T*)found_block->user_data;
    }
    else {
        found_block->is_free_ = false;
        return (T*)found_block->user_data;
    }
}

template<typename T>
T* SegregatedListAllocator<T>::allocate_large_block(size_t required_size) {
    size_t aligment_size = (required_size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
    size_t total_size = aligment_size + sizeof(Block);

    void* block_start = nullptr;

#ifdef _WIN32
    block_start = VirtualAlloc(NULL, total_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (block_start == NULL) {
        throw std::bad_alloc();
    }
#else
    block_start = mmap(nullptr, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (block_start == MAP_FAILED) {
        throw std::bad_alloc();
    }
#endif

    Block* header = static_cast<Block*>(block_start);
    header->size_ = total_size;
    header->is_free_ = false;
    header->is_mmapped_ = true;

    return reinterpret_cast<T*>(header->user_data);
}

template<typename T>
Block* SegregatedListAllocator<T>::split_block(Block* block_to_split, size_t required_size) {
    size_t allocated_size = required_size + sizeof(Block);
    size_t new_block_size = block_to_split->size_ - allocated_size;

    Block* new_block = reinterpret_cast<Block*>(reinterpret_cast<uintptr_t>(block_to_split) + allocated_size);
    new_block->size_ = new_block_size;
    new_block->is_free_ = true;
    new_block->is_mmapped_ = false;

    block_to_split->size_ = allocated_size;
    block_to_split->is_free_ = false;
    block_to_split->is_mmapped_ = false;

    update_footer(block_to_split);
    update_footer(new_block);

    return new_block;
}

template<typename T>
void SegregatedListAllocator<T>::unlink_from_freelist(Block* block_to_remove, size_t index) {
    Block* next_block = block_to_remove->free_block_pointers.next_free;

    m_free_lists_[index] = next_block;

    if (next_block != nullptr) {
        next_block->free_block_pointers.prev_free = nullptr;
    }
}

template<typename T>
Block* SegregatedListAllocator<T>::coalesce(Block* current_block) {
    if (reinterpret_cast<uintptr_t>(current_block) > reinterpret_cast<uintptr_t>(m_start_)) {
        current_block = merge_with_left_block(current_block);
    }

    merge_with_right_block(current_block);

    return current_block;
}

template<typename T>
Block* SegregatedListAllocator<T>::merge_with_left_block(Block* current_block) {
    size_t* left_block_foooter = reinterpret_cast<size_t*>(
        reinterpret_cast<uintptr_t>(current_block) - sizeof(size_t)
        );

    Block* left_block = reinterpret_cast<Block*>(
        reinterpret_cast<uintptr_t>(current_block) - *left_block_foooter
        );

    if (reinterpret_cast<uintptr_t>(left_block) >= reinterpret_cast<uintptr_t>(m_start_)
        && left_block->is_free_ == true) {

        size_t left_block_index = find_list_index(left_block->size_);
        unlink_from_freelist(left_block, left_block_index);

        left_block->size_ += current_block->size_;

        update_footer(left_block);

        current_block = left_block;
    }

    return current_block;
}

template<typename T>
void SegregatedListAllocator<T>::merge_with_right_block(Block* current_block) {
    Block* right_block = reinterpret_cast<Block*>(
        reinterpret_cast<uintptr_t>(current_block) + current_block->size_
        );

    if (reinterpret_cast<uintptr_t>(right_block) < reinterpret_cast<uintptr_t>(m_start_) + m_total_size_
        && right_block->is_free_ == true) {

        size_t right_block_index = find_list_index(right_block->size_);
        unlink_from_freelist(right_block, right_block_index);

        current_block->size_ += right_block->size_;

        update_footer(current_block);
    }
}

template<typename T>
void SegregatedListAllocator<T>::add_to_freelist(Block* block, size_t index) {
    block->free_block_pointers.next_free = m_free_lists_[index];
    block->free_block_pointers.prev_free = nullptr;

    if (m_free_lists_[index] != nullptr)
        m_free_lists_[index]->free_block_pointers.prev_free = block;

    m_free_lists_[index] = block;
}

template<typename T>
void SegregatedListAllocator<T>::update_footer(Block* block) const {
    size_t* current_block_footer = reinterpret_cast<size_t*>(
        reinterpret_cast<uintptr_t>(block) + block->size_ - sizeof(size_t)
        );
    *current_block_footer = block->size_;
}

template<typename T>
size_t SegregatedListAllocator<T>::find_list_index(const size_t size) const {
    if (size <= 16)
        return 0;
    size_t index = static_cast<size_t>(std::log2(size - 1)) - 3;
    const size_t max_index = 13;
    return std::min(index, max_index);
}