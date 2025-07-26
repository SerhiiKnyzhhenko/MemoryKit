#ifndef GENERAL_PURPOSE_ALLOCATOR_H
#define GENERAL_PURPOSE_ALLOCATOR_H

#include <cstddef> // Для size_t
#include <cstdint> // Для uintptr_t
#include <iostream>
#include <stdexcept>
#include "Block.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif


template<typename T>
class GeneralPurposeAllocator {

private:
    std::shared_ptr<Block*> m_free_list_head_ptr_;
    std::shared_ptr<void> m_start_;
    size_t m_total_size_ = 0;
    static constexpr size_t LARGE_ALLOC_THRESHOLD = 128 * 1024;
    static constexpr size_t ALIGNMENT = 16;

public:
    GeneralPurposeAllocator(size_t size);
    GeneralPurposeAllocator();
    GeneralPurposeAllocator(const GeneralPurposeAllocator& other) = default;

    // Шаблонный конструктор копирования
    template<typename U>
    GeneralPurposeAllocator(const GeneralPurposeAllocator<U>& other) noexcept;

    // Также добавьте эту строку в private секцию, чтобы конструктор мог работать
    template<typename U>
    friend class GeneralPurposeAllocator;

    GeneralPurposeAllocator& operator=(const GeneralPurposeAllocator&) = delete;
    GeneralPurposeAllocator(GeneralPurposeAllocator&&) = delete;
    GeneralPurposeAllocator& operator=(GeneralPurposeAllocator&&) = delete;

    template<typename U>
    bool operator==(const GeneralPurposeAllocator<U>& other) const noexcept {
        return m_start_.get() == other.m_start_.get();
    }

    template<typename U>
    bool operator!=(const GeneralPurposeAllocator<U>& other) const noexcept {
        return m_start_.get() != other.m_start_.get();
    }

    T* allocate(size_t n);
    void deallocate(T* user_data_ptr);
    void deallocate(T* user_data_ptr, size_t n);

    std::shared_ptr<Block*> get_m_free_list_head_ptr_() const;
    std::shared_ptr<void> get_m_start() const;
    size_t get_m_total_size() const;

private:
    T* allocate_from_free_list(size_t requested_size);
    T* allocate_large_block(size_t required_size);
    Block* find_first_fit(size_t total_neded_size) const;
    Block* split_block(Block* block_to_split, size_t required_size);
    void update_freelist_after_allocation(Block* old_block, Block* new_block);
    void unlink_from_freelist(Block* block_to_remove);
    Block* coalesce(Block* block, bool* merging_with_the_left_block);
    Block* merge_with_left_block(Block* block, bool* merging_with_the_left_bloc);
    void merge_with_right_block(Block* block);
    void add_to_freelist(Block* block);
    void update_footer(Block* block) const;

public:
    // Required typedefs for a standard-compliant allocator
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using size_type = size_t;

    // Required rebind struct
    template<typename U>
    struct rebind {
        using other = GeneralPurposeAllocator<U>;
    };

};

#include "GeneralPurposeAllocator.tpp"

#endif // !GENERAL_PURPOSE_ALLOCATOR_H
