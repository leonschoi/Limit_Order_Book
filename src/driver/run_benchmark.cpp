#include <chrono>
#include <thread>
#include <x86intrin.h>
#include <iostream>

double measure_ghz() {
    using namespace std::chrono;

    auto t0 = steady_clock::now();
    unsigned long long c0 = __rdtsc();

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    auto t1 = steady_clock::now();
    unsigned long long c1 = __rdtsc();

    double ns = duration_cast<nanoseconds>(t1 - t0).count();
    double cycles = double(c1 - c0);

    return cycles / ns;  // cycles per ns == GHz
}