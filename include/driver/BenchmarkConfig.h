#pragma once
#include <cstdint>
#include <string>

// Benchmark configuration parameters
struct BenchmarkConfig {
    std::string type;     // "assoc", "flat", "bitmap", "all" (default: "bitmap")
    int producers;        // number of producer threads
    uint64_t messages;    // messages per producer
    bool pin_threads;     // whether to pin threads to specific CPU cores for better isolation

    int64_t mid_price;    // initial mid price
    int64_t max_price;    // max price
    uint64_t pool_size;   // order memory pool size (queue capacity * 10 for safety margin)
};


