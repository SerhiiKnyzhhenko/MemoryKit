#include "PoolAllocator.h"
#include <iostream>

PoolAllocator::PoolAllocator(size_t chunk_size, size_t num_chunks) : chunk_size_(chunk_size), num_chunks_(num_chunks) {
	
    size_t alignment = 16;
    size_t m_total_size = ((chunk_size_ * num_chunks) + alignment - 1) & ~(alignment - 1);

#ifdef _WIN32
    m_start_ = VirtualAlloc(NULL, m_total_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
    m_start_ = mmap(nullptr, m_total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
#endif

    m_free_list_head = reinterpret_cast<Chunk*>(m_start_);
    Chunk* current_chunk = m_free_list_head;

    for (size_t i = 0; i < num_chunks_ - 1; i++) {
        Chunk* new_chunk = reinterpret_cast<Chunk*>(reinterpret_cast<uintptr_t>(current_chunk) + chunk_size);
        current_chunk->next_free = new_chunk;
        current_chunk = current_chunk->next_free;
    }
    current_chunk->next_free = nullptr;

}

PoolAllocator::~PoolAllocator() {

#ifdef _WIN32
    VirtualFree(m_start_, 0, MEM_RELEASE);
#else
    munmap(m_start_, m_total_size_);
#endif

}

void* PoolAllocator::allocate(size_t size) {

    if (m_free_list_head == nullptr)
        return nullptr;

    Chunk* chunk_for_user = m_free_list_head;
    m_free_list_head = m_free_list_head->next_free;

    return chunk_for_user;

}

void PoolAllocator::deallocate(void* data) {

    Chunk* deallocate_chunk = reinterpret_cast<Chunk*>(reinterpret_cast<uintptr_t>(data));

    deallocate_chunk->next_free = m_free_list_head;
    m_free_list_head = deallocate_chunk;

}