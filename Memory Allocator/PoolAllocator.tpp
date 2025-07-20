

template <typename T>
PoolAllocator<T>::PoolAllocator(size_t num_chunks) : chunk_size_(std::max(sizeof(T), sizeof(Chunk*))), num_chunks_(num_chunks){
    size_t alignment = 16;
    m_total_size_ = ((chunk_size_ * num_chunks) + alignment - 1) & ~(alignment - 1);

#ifdef _WIN32
    m_start_ = VirtualAlloc(NULL, m_total_size_, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (m_start_ == NULL) {
        throw std::bad_alloc();
    }
#else
    m_start_ = mmap(nullptr, m_total_size_, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (m_start_ == MAP_FAILED) {
        throw std::bad_alloc();
    }
#endif

    m_free_list_head = reinterpret_cast<Chunk*>(m_start_);
    Chunk* current_chunk = m_free_list_head;

    for (size_t i = 0; i < num_chunks_ - 1; i++) {
        Chunk* new_chunk = reinterpret_cast<Chunk*>(reinterpret_cast<uintptr_t>(current_chunk) + chunk_size_);
        current_chunk->next_free = new_chunk;
        current_chunk = current_chunk->next_free;
    }
    current_chunk->next_free = nullptr;
}

template <typename T>
PoolAllocator<T>::~PoolAllocator() {
#ifdef _WIN32
    VirtualFree(m_start_, 0, MEM_RELEASE);
#else
    munmap(m_start_, m_total_size_);
#endif
}

template <typename T>
T* PoolAllocator<T>::allocate() {
    if (m_free_list_head == nullptr)
        return nullptr;

    void* untyped_memory = static_cast<void*>(m_free_list_head);
    T* chunk_for_user = static_cast<T*>(untyped_memory);

    m_free_list_head = m_free_list_head->next_free;

    return chunk_for_user;
}

template <typename T>
void PoolAllocator<T>::deallocate(T* data) {
    Chunk* deallocate_chunk = reinterpret_cast<Chunk*>(data);

    deallocate_chunk->next_free = m_free_list_head;
    m_free_list_head = deallocate_chunk;
}

