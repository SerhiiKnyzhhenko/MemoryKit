
template<typename T>
SegregatedListAllocator<T>::SegregatedListAllocator(size_t size) {
    m_total_size_ = (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);

#ifdef _WIN32
    m_start_ = std::shared_ptr<void>(
        VirtualAlloc(NULL, m_total_size_, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE),
        [](void* ptr) { VirtualFree(ptr, 0, MEM_RELEASE); }
    );
    if (m_start_.get() == NULL) {
        throw std::bad_alloc();
    }
#else
    m_start_ = std::shared_ptr<void>(
        mmap(nullptr, m_total_size_, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0),
        [this](void* ptr) { munmap(ptr, m_total_size_); }
    );
    if (m_start_.get() == MAP_FAILED) {
        throw std::bad_alloc();
    }
#endif

    Block* initial_block = static_cast<Block*>(m_start_.get());
    initial_block->size_ = m_total_size_;
    initial_block->is_free_ = true;
    initial_block->is_mmapped_ = false;
    initial_block->free_block_pointers.next_free = nullptr;
    initial_block->free_block_pointers.prev_free = nullptr;

    m_free_lists_ptr_ = std::make_shared<std::vector<Block*>>(NUM_FREE_LISTS, nullptr);

    size_t index = find_list_index(m_total_size_);
    (*m_free_lists_ptr_)[index] = initial_block;

    m_free_lists_bitmap_ |= (1ULL << index);
}

template<typename T>
SegregatedListAllocator<T>::SegregatedListAllocator() 
    : SegregatedListAllocator(1024 * 1024) {

}

template<typename T>
template<typename U>
SegregatedListAllocator<T>::SegregatedListAllocator(const SegregatedListAllocator<U>& other) noexcept
    : m_start_(other.get_m_start()),
    m_total_size_(other.get_m_total_size()),
    m_free_lists_ptr_(other.get_m_free_lists_ptr()),
    m_free_lists_bitmap_(other.get_m_free_lists_bitmap()) {

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
    const size_t total_needed_size = required_size + HEADER_SIZE + FOOTER_SIZE;
    size_t ideal_index = find_list_index(total_needed_size);
    size_t found_index = 0;

    uint64_t shifted_bitmap = m_free_lists_bitmap_ >> ideal_index;

    if (shifted_bitmap == 0) {
        return nullptr;
    }

    unsigned long offset;
#ifdef _MSC_VER
    _BitScanForward64(&offset, shifted_bitmap);
#else //GCC/Clang
    offset = __builtin_ffsll(shifted_bitmap) - 1;
#endif

    found_index = ideal_index + offset;

    Block* found_block = (*m_free_lists_ptr_)[found_index];

    unlink_from_freelist(found_block, found_index);

    if (found_block->size_ >= total_needed_size + HEADER_SIZE + FOOTER_SIZE) {
        Block* remainder_block = split_block(found_block, total_needed_size);
        size_t remainder_index = find_list_index(remainder_block->size_);
        add_to_freelist(remainder_block, remainder_index);
    }
    else {
        found_block->is_free_ = false;
    }

    return (T*)found_block->user_data;
}

template<typename T>
T* SegregatedListAllocator<T>::allocate_large_block(size_t required_size) {
    size_t aligment_size = (required_size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
    size_t total_size = aligment_size + HEADER_SIZE;

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
Block* SegregatedListAllocator<T>::split_block(Block* block_to_split, size_t total_needed_size) {
    size_t aligned_allocated_size = (total_needed_size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);

    size_t new_block_size = block_to_split->size_ - aligned_allocated_size;

    Block* new_block = reinterpret_cast<Block*>(reinterpret_cast<uintptr_t>(block_to_split) + aligned_allocated_size);
    new_block->size_ = new_block_size;
    new_block->is_free_ = true;
    new_block->is_mmapped_ = false;

    block_to_split->size_ = aligned_allocated_size;
    block_to_split->is_free_ = false;
    block_to_split->is_mmapped_ = false;

    update_footer(block_to_split);
    update_footer(new_block);

    return new_block;
}

template<typename T>
inline void SegregatedListAllocator<T>::unlink_from_freelist(Block* block_to_remove, size_t index) {
    Block* next_block = block_to_remove->free_block_pointers.next_free;

    (*m_free_lists_ptr_)[index] = next_block;

    if (next_block != nullptr) {
        next_block->free_block_pointers.prev_free = nullptr;
    }

    if ((*m_free_lists_ptr_)[index] == nullptr) {
        m_free_lists_bitmap_ &= ~(1ULL << index); 
    }
}

template<typename T>
inline Block* SegregatedListAllocator<T>::coalesce(Block* current_block) {
    if (reinterpret_cast<uintptr_t>(current_block) > reinterpret_cast<uintptr_t>(m_start_.get())) {
        current_block = merge_with_left_block(current_block);
    }

    merge_with_right_block(current_block);

    return current_block;
}

template<typename T>
Block* SegregatedListAllocator<T>::merge_with_left_block(Block* current_block) {
    size_t* left_block_foooter = reinterpret_cast<size_t*>(
        reinterpret_cast<uintptr_t>(current_block) - FOOTER_SIZE
        );

    Block* left_block = reinterpret_cast<Block*>(
        reinterpret_cast<uintptr_t>(current_block) - *left_block_foooter
        );

    if (reinterpret_cast<uintptr_t>(left_block) >= reinterpret_cast<uintptr_t>(m_start_.get())
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

    if (reinterpret_cast<uintptr_t>(right_block) < reinterpret_cast<uintptr_t>(m_start_.get()) + m_total_size_
        && right_block->is_free_ == true) {

        size_t right_block_index = find_list_index(right_block->size_);
        unlink_from_freelist(right_block, right_block_index);

        current_block->size_ += right_block->size_;

        update_footer(current_block);
    }
}

template<typename T>
inline void SegregatedListAllocator<T>::add_to_freelist(Block* block, size_t index) {

    if ((*m_free_lists_ptr_)[index] == nullptr) {
        m_free_lists_bitmap_ |= (1ULL << index);
    }

    block->free_block_pointers.next_free = (*m_free_lists_ptr_)[index];
    block->free_block_pointers.prev_free = nullptr;

    if ((*m_free_lists_ptr_)[index] != nullptr)
        (*m_free_lists_ptr_)[index]->free_block_pointers.prev_free = block;

    (*m_free_lists_ptr_)[index] = block;
}

template<typename T>
inline void SegregatedListAllocator<T>::update_footer(Block* block) const {
    size_t* current_block_footer = reinterpret_cast<size_t*>(
        reinterpret_cast<uintptr_t>(block) + block->size_ - FOOTER_SIZE
        );
    *current_block_footer = block->size_;
}

template<typename T>
inline size_t SegregatedListAllocator<T>::find_list_index(const size_t size) const {
    if (size <= 16) 
        return 0;

    unsigned long index;
#ifdef _MSC_VER
    _BitScanReverse64(&index, size - 1);
#else 
    index = (63 - __builtin_clzll(size - 1));
#endif

    size_t list_index = index - 3;

    const size_t max_index = NUM_FREE_LISTS - 1;
    return std::min(list_index, max_index);
}

template<typename T>
inline std::shared_ptr<std::vector<Block*>> SegregatedListAllocator<T>::get_m_free_lists_ptr() const {
    return m_free_lists_ptr_;
}

template<typename T>
inline std::shared_ptr<void> SegregatedListAllocator<T>::get_m_start() const {
    return m_start_;
}

template<typename T>
inline size_t SegregatedListAllocator<T>::get_m_total_size() const {
    return m_total_size_;
}

template<typename T>
inline uint64_t SegregatedListAllocator<T>::get_m_free_lists_bitmap() const {
    return m_free_lists_bitmap_;
}