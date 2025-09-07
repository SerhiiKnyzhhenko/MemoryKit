#ifndef THREADCACHE_HPP
#define THREADCACHE_HPP

#include "Block.h"

const size_t CACHE_SIZE_LIMIT = 128;
const size_t NUM_FREE_LISTS = 32;

struct ThreadCache {

	Block* free_lists[NUM_FREE_LISTS] = { nullptr };
	bool initialized = false;

};

#endif // !THREADCACHE_HPP
