#ifndef STACKALLOCATOR_H
#define STACKALLOCATOR_H

#include <cstddef> // Для size_t
#include <cstdint> 
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

struct StackHeader {
	StackHeader* previous_header;
};

template<typename T>
class StackAllocator {

private:
	void* m_start_ = nullptr;
	void* m_current_pos_ = nullptr;
	StackHeader* m_last_header_;
	size_t m_total_size_ = 0;
	const size_t ALLIGMENT = 16;

public:
	StackAllocator(size_t size);
	~StackAllocator();

	[[nodiscard]] T* allocate(size_t required_size);
	void pop();
	void clear();

};

#include "StackAllocator.tpp"

#endif // !STACKALLOCATOR_H