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

    size_t free_memory = reinterpret_cast<size_t>(
        reinterpret_cast<uintptr_t>(m_start_) + m_total_size_ - reinterpret_cast<uintptr_t>(m_current_pos_)
        );

    if (required_size > free_memory)
        return nullptr;

    size_t alignment = 16;
    size_t alligment_size = (required_size + alignment - 1) & ~(alignment - 1);

    update_footer(m_current_pos_, alligment_size);

    m_current_pos_ = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_current_pos_) + alligment_size + sizeof(size_t));
   
    return m_current_pos_;

}

void StackAllocator::deallocate(void* pos) {

    size_t* left_pos_foooter = reinterpret_cast<size_t*>(
        reinterpret_cast<uintptr_t>(pos) - sizeof(size_t)
        );

    m_current_pos_ = reinterpret_cast<void*>(
        reinterpret_cast<uintptr_t>(pos) - *left_pos_foooter
        );

}

void StackAllocator::clear() {

    m_current_pos_ = m_start_;

}

void StackAllocator::update_footer(void* pos, size_t alligment_size) {

    size_t* current_pos_footer = reinterpret_cast<size_t*>(
        reinterpret_cast<uintptr_t>(pos) + alligment_size - sizeof(size_t)
        );
    *current_pos_footer = alligment_size;

}