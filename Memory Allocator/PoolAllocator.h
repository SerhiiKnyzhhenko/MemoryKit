#ifndef POOLALLOCATOR_H
#define POOLALLOCATOR_H

#include <cstddef> // Для size_t
#include <cstdint>
#include <stdexcept>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

struct Chunk {
    Chunk* next_free;
};

template<typename T>
class PoolAllocator {

private:
    void* m_start_ = nullptr;
    Chunk* m_free_list_head = nullptr;
    size_t chunk_size_;
    size_t num_chunks_;
    size_t m_total_size_;

public:
    explicit PoolAllocator(size_t num_chunks);
    ~PoolAllocator();

    [[nodiscard]] T* allocate();
    void deallocate(T* data);

};

#include "PoolAllocator.tpp"

#endif // !POOLALLOCATOR_H
