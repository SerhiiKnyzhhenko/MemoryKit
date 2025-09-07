#ifndef THREADCACHE_HPP
#define THREADCACHE_HPP

#include "Block.h"

struct SlabInfo {
	char* start = nullptr;
	size_t size = 0;
};

const size_t CACHE_SIZE_LIMIT = 128;
const size_t NUM_FREE_LISTS = 32;

struct ThreadCache {

	Block* free_lists[NUM_FREE_LISTS] = { nullptr };
	SlabInfo slabs[NUM_FREE_LISTS];
	bool initialized = false;

};

#endif // !THREADCACHE_HPP
