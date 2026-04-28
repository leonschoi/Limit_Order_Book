#pragma once
#include <cstdint>
#include <x86intrin.h>

inline uint64_t rdtsc() {
    unsigned aux;
    return __rdtscp(&aux); // serialized
}
