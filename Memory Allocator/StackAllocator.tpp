
template<typename T>
StackAllocator<T>::StackAllocator(size_t size) {

    m_total_size_ = (size + ALLIGMENT - 1) & ~(ALLIGMENT - 1);

#ifdef _WIN32
    m_start_ = VirtualAlloc(NULL, m_total_size_, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (m_start_ == NULL) {
        throw std::bad_alloc();
    }
#else
    m_start = mmap(nullptr, m_totalSize_, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (m_start_ == MAP_FAILED) {
        throw std::bad_alloc();
    }
#endif

    m_current_pos_ = m_start_;

    m_last_header_ = nullptr;
}

template<typename T>
StackAllocator<T>::~StackAllocator() {

#ifdef _WIN32
    VirtualFree(m_start_, 0, MEM_RELEASE);
#else
    munmap(m_start_, m_totalSize_);
#endif

}

template<typename T>
T* StackAllocator<T>::allocate(size_t required_size) {
    size_t aligned_size = (required_size + ALLIGMENT - 1) & ~(ALLIGMENT - 1);
    size_t total_size = sizeof(StackHeader) + aligned_size;

    if ((uintptr_t)m_current_pos_ + total_size > (uintptr_t)m_start_ + m_total_size_) {
        return nullptr;
    }

    StackHeader* new_header = (StackHeader*)m_current_pos_;
    new_header->previous_header = m_last_header_;

    m_last_header_ = new_header;
    m_current_pos_ = (char*)m_current_pos_ + total_size;

    return static_cast<T*>(static_cast<void*>((char*)new_header + sizeof(StackHeader)));
}

template<typename T>
void StackAllocator<T>::pop() {
    if (m_last_header_ == nullptr)
        return;

    void* new_current_pos = m_last_header_;
    StackHeader* prev_header = m_last_header_->previous_header;

    m_current_pos_ = new_current_pos;
    m_last_header_ = prev_header;
}

template<typename T>
void StackAllocator<T>::clear() {
    m_current_pos_ = m_start_;
    m_last_header_ = nullptr;
}