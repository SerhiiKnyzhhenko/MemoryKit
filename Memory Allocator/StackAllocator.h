#ifndef STACKALLOCATOR_H
#define STACKALLOCATOR_H

#include "IAllocator.h"
#include <cstddef> // Для size_t
#include <cstdint> 

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

const size_t METADATA_SIZE = sizeof(void*);
const size_t ALLIGMENT = 16;


class StackAllocator : public IAllocator {

private:
	void* m_start_ = nullptr;
	void* m_current_pos_ = nullptr;
	size_t m_total_size_ = 0;
	size_t m_last_allocation_size_ = 0;
	
public:
	StackAllocator(size_t size);
	~StackAllocator();

	void* allocate(size_t required_size) override;
	void pop();
	void clear();

private:
	void deallocate(void*) override {};
};

#endif // !STACKALLOCATOR_H



