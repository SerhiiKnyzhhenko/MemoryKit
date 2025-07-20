
template<typename T>
GeneralPurposeAllocator<T>::GeneralPurposeAllocator(size_t size) {
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

    m_free_list_head_ = initial_block;
}

template<typename T>
GeneralPurposeAllocator<T>::~GeneralPurposeAllocator() {
#ifdef _WIN32
    VirtualFree(m_start_, 0, MEM_RELEASE);
#else
    munmap(m_start_, m_total_size_);
#endif
}

template<typename T>
T* GeneralPurposeAllocator<T>::allocate(size_t n) {
    size_t required_size = n * sizeof(T);

    if (required_size <= LARGE_ALLOC_THRESHOLD)
        return allocate_from_free_list(required_size);
    else
        return allocate_large_block(required_size);
}

template<typename T>
void GeneralPurposeAllocator<T>::deallocate(T* user_data_ptr) {
    deallocate(user_data_ptr, 1);
}

template<typename T>
void GeneralPurposeAllocator<T>::deallocate(T* user_data_ptr, size_t n) {
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

    bool merging_with_the_left_block = false;

    current_block = coalesce(current_block, &merging_with_the_left_block);

    if (!merging_with_the_left_block) {
        add_to_freelist(current_block);
    }
}

template<typename T>
T* GeneralPurposeAllocator<T>::allocate_from_free_list(size_t required_size) {
    Block* current_block = find_first_fit(required_size);
    if (current_block == nullptr)
        return nullptr;

    if (current_block->size_ > required_size + sizeof(Block)) {

        Block* new_block = split_block(current_block, required_size);

        update_freelist_after_allocation(current_block, new_block);

        return (T*)current_block->user_data;
    }
    else {
        unlink_from_freelist(current_block);
        current_block->is_free_ = false;
        return (T*)current_block->user_data;
    }
}
template<typename T>
T* GeneralPurposeAllocator<T>::allocate_large_block(size_t required_size) {
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
Block* GeneralPurposeAllocator<T>::find_first_fit(size_t required_size) {
    Block* current_block = m_free_list_head_;
    while (current_block != nullptr) {
        if (current_block->size_ >= required_size) {
            break;
        }
        current_block = current_block->free_block_pointers.next_free;
    }
    return current_block;
}

template<typename T>
Block* GeneralPurposeAllocator<T>::split_block(Block* block_to_split, size_t required_size) {
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
void GeneralPurposeAllocator<T>::update_freelist_after_allocation(Block* old_block, Block* new_block) {
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

template<typename T>
void GeneralPurposeAllocator<T>::unlink_from_freelist(Block* block_to_remove) {
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

template<typename T>
Block* GeneralPurposeAllocator<T>::coalesce(Block* current_block, bool* merging_with_the_left_block) {
    if (reinterpret_cast<uintptr_t>(current_block) > reinterpret_cast<uintptr_t>(m_start_)) {
        current_block = merge_with_left_block(current_block, merging_with_the_left_block);
    }

    merge_with_right_block(current_block);

    return current_block;
}

template<typename T>
Block* GeneralPurposeAllocator<T>::merge_with_left_block(Block* current_block, bool* merging_with_the_left_block) {
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

        *merging_with_the_left_block = true;
    }

    return current_block;
}

template<typename T>
void GeneralPurposeAllocator<T>::merge_with_right_block(Block* current_block) {
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

template<typename T>
void GeneralPurposeAllocator<T>::add_to_freelist(Block* block) {
    block->free_block_pointers.next_free = m_free_list_head_;
    block->free_block_pointers.prev_free = nullptr;

    if (m_free_list_head_ != nullptr)
        m_free_list_head_->free_block_pointers.prev_free = block;

    m_free_list_head_ = block;
}

template<typename T>
void GeneralPurposeAllocator<T>::update_footer(Block* block) {
    size_t* current_block_footer = reinterpret_cast<size_t*>(
        reinterpret_cast<uintptr_t>(block) + block->size_ - sizeof(size_t)
        );
    *current_block_footer = block->size_;
}