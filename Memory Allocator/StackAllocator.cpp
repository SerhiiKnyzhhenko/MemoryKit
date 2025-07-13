#include "StackAllocator.h"


StackAllocator::StackAllocator(size_t size) {

    size_t alignment = 16;
    m_total_size_ = (size + alignment - 1) & ~(alignment - 1);

#ifdef _WIN32
    m_start_ = VirtualAlloc(NULL, m_total_size_, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
    m_start = mmap(nullptr, m_totalSize_, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
#endif

    m_current_pos_ = m_start_;

}

StackAllocator::~StackAllocator() {

#ifdef _WIN32
    VirtualFree(m_start_, 0, MEM_RELEASE);
#else
    munmap(m_start_, m_totalSize);
#endif

}

void* StackAllocator::allocate(size_t required_size) {

    size_t aligned_size = (required_size + ALLIGMENT - 1) & ~(ALLIGMENT - 1);

    const size_t total_chunk_size = METADATA_SIZE + aligned_size;

    if ((uintptr_t)m_current_pos_ + total_chunk_size > (uintptr_t)m_start_ + m_total_size_) {
        return nullptr;
    }

    m_last_allocation_size_ = total_chunk_size;

    void* header_address = m_current_pos_;

    *static_cast<void**>(header_address) = header_address; 

    void* user_ptr = (char*)header_address + METADATA_SIZE;

    m_current_pos_ = (char*)m_current_pos_ + total_chunk_size;

    return user_ptr;

}

void StackAllocator::pop() {
    
    if (m_start_ == m_current_pos_)
        return;

    m_current_pos_ = (char*)m_current_pos_ - m_last_allocation_size_;

}


void StackAllocator::clear() {

    m_current_pos_ = m_start_;

}
