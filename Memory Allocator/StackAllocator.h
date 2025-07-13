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

class StackAllocator : public IAllocator {

private:
	void* m_start_;
	void* m_current_pos_;
	size_t m_total_size_;

public:
	StackAllocator(size_t size);
	~StackAllocator();

	void* allocate(size_t required_size) override;
	void deallocate(void*) override;
	
	void clear();
	void update_footer(void* pos, size_t alligment_size);
};

#endif // !STACKALLOCATOR_H



