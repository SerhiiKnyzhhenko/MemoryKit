#ifndef CLASSMAP_H
#define CLASSMAP_H

#include <array>

//cache
const size_t MAX_CACHEABLE_SIZE = 256;
const int NUM_CACHED_LISTS = MAX_CACHEABLE_SIZE / 8; // = 32

constexpr std::array<size_t, NUM_CACHED_LISTS> size_classes = {
    8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112,
    120, 128, 136, 144, 152, 160, 168, 176, 184, 192, 200,
    208, 216, 224, 232, 240, 248, 256
};


constexpr std::array<size_t, MAX_CACHEABLE_SIZE + 1> create_size_to_class_map() {
    std::array<size_t, MAX_CACHEABLE_SIZE + 1> map{};
    size_t class_index = 0;

    for (size_t s = 1; s <= MAX_CACHEABLE_SIZE; s++) {
        if (s > size_classes[class_index])
            class_index++;
        map[s] = class_index;
    }
    return map;
}


#endif // !CLASSMAP_H
