#pragma once
#include <cstdint>
#include <string>

// Benchmark configuration parameters
struct BenchmarkConfig {
    std::string type   = "bitmap";      // "assoc", "flat", "bitmap", "all" (default: "bitmap")
    int producers      = 2;             // number of producer threads
    uint64_t messages  = 1'000'000;     // messages per producer
    bool pin_threads   = true;          // whether to pin threads to specific CPU cores for better isolation

    int64_t mid_price   = 10'000;       // initial mid price
    int64_t max_price   = 20'000;       // max price
    uint64_t pool_size  = 10'000'000;   // order memory pool size (queue capacity * 10 for safety margin)
};


