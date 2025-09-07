#ifndef SEGREGATED_LIST_ALLOCATOR_2_H
#define SEGREGATED_LIST_ALLOCATOR_2_H

#include <cstddef> // Для size_t
#include <cstdint> // Для uintptr_t
#include <iostream>
#include <vector>
#include <stdexcept>
#include <intrin.h >
#include "Block.h"
#include "ThreadCache.h"
#include "ClassMap.h"


#include <limits> 

#ifdef _MSC_VER 
#include <intrin.h>
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif


inline thread_local ThreadCache t_cache;

inline constexpr auto size_to_class_map = create_size_to_class_map();


template<typename T>
class SegregatedListAllocator {

private:
    std::shared_ptr<std::vector<Block*>> m_free_lists_ptr_;
    std::shared_ptr<void> m_start_;
    size_t m_total_size_ = 0;
    static constexpr size_t LARGE_ALLOC_THRESHOLD = 128 * 1024;
    static constexpr size_t ALIGNMENT = 16;
    static constexpr size_t NUM_FREE_LISTS = 14;
    static constexpr size_t HEADER_SIZE = sizeof(Block);
    static constexpr size_t FOOTER_SIZE = sizeof(size_t);

    uint64_t m_free_lists_bitmap_ = 0;

public:
    SegregatedListAllocator(size_t size);
    SegregatedListAllocator();

    template<typename U>
    SegregatedListAllocator(const SegregatedListAllocator<U>& other) noexcept;

    SegregatedListAllocator& operator=(const SegregatedListAllocator&) = delete;
    SegregatedListAllocator(SegregatedListAllocator&&) = delete;
    SegregatedListAllocator& operator=(SegregatedListAllocator&&) = delete;

    template<typename U>
    bool operator==(const SegregatedListAllocator<U>& other) const noexcept {
        return m_start_.get() == other.m_start_.get();
    }

    template<typename U>
    bool operator!=(const SegregatedListAllocator<U>& other) const noexcept {
        return m_start_.get() != other.m_start_.get();
    }


    T* allocate(size_t n);
    void deallocate(T* user_data_ptr);
    void deallocate(T* user_data_ptr, size_t n);

    std::shared_ptr<std::vector<Block*>> get_m_free_lists_ptr() const;
    std::shared_ptr<void> get_m_start() const;
    size_t get_m_total_size() const;
    uint64_t get_m_free_lists_bitmap() const;

private:

    template<typename U>
    friend class SegregatedListAllocator;

    T* allocate_from_free_list(size_t requested_size);.
    T* allocate_from_central_storage(size_t requested_size);
    T* allocate_large_block(size_t required_size);
    Block* split_block(Block* block_to_split, size_t required_size);
    Block* coalesce(Block* block);
    Block* merge_with_left_block(Block* block);
    void merge_with_right_block(Block* block);
    void unlink_from_freelist(Block* block_to_remove, size_t index);
    void add_to_freelist(Block* block, size_t index);
    void update_footer(Block* block) const;
    size_t find_list_index(const size_t size) const;

public:
    // Required typedefs for a standard-compliant allocator
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using size_type = size_t;

    // Required rebind struct
    template<typename U>
    struct rebind {
        using other = SegregatedListAllocator<U>;
    };

};

#include "SegregatedListAllocator_2.tpp"

#endif // !SEGREGATED_LIST_ALLOCATOR_2_H