#pragma once
#include <vector>
#include <thread>
#include <barrier>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <cstdint>
#include <immintrin.h>
#include <vector>

#include "common/constants.h"
#include "common/rdtsc.h"
#include "common/pin_thread.h"
#include "driver/benchmark.h"
#include "queue/MPSC_bounded_ring.h"

#include "Message.h"
#include "OrderProducer.h"
#include "MatchingEngine_assoc_container.h"
#include "MatchingEngine_flat_array.h"
#include "MatchingEngine_bitmap.h"

using namespace std;

double measure_ghz();

// Helper to compute percentiles of sorted latency samples
inline uint64_t percentile(vector<uint64_t>& v, size_t size, double p) {
    size_t idx = static_cast<size_t>(p * size);
    if (idx >= size) idx = size - 1;
    return v[idx];
}


template<typename Q, typename MatchingEngine>
void run_benchmark(OrderProducer<Q>& producer, MatchingEngine& engine, const BenchmarkConfig& cfg) {
    const int P = cfg.producers;
    const size_t N = cfg.messages;

    atomic<bool> start{false};
    barrier sync(P + 1);
    vector<thread> producers;
    vector<pair<double, double>> mean_vars(P); // to store mean and variance of prices for each producer

    size_t totalMessages = N * P;
    uint64_t throughtputStat;
    vector<uint64_t> queueStats(totalMessages); 
    vector<uint64_t> entryStats(totalMessages);
    vector<uint64_t> cancelStats(totalMessages);
    size_t received = 0;
    size_t entered = 0;
    size_t cancelled = 0;
    size_t trades = 0;
    size_t entry_best_bid_level_updates = 0;
    size_t entry_best_ask_level_updates = 0;
    size_t cancel_best_bid_level_updates = 0;
    size_t cancel_best_ask_level_updates = 0;

    vector<int> cpu_map = get_physical_core_cpus(); 
    const int threadID_start = 1; 

    uint64_t now = rdtsc();

    // producers in each thread generate events and push to queue
    for (int producer_id = 0; producer_id < P; ++producer_id) {
        producers.emplace_back([&, producer_id]() {
            using ::_mm_pause;
            
            if (cfg.pin_threads) {
                pin_thread_physical(threadID_start+producer_id, cpu_map);  
                //cout << "Producer " << producer_id << " pinned to CPU " << cpu_map[threadID_start+producer_id] << " : ";
                //cout << "Actual CPU: " << sched_getcpu() << endl;
            }

            sync.arrive_and_wait();

            // wait for start signal
            while (!start.load(memory_order_acquire)) {
                _mm_pause();
            }

            double mean_price, var_price;
            producer.run(producer_id, N, mean_price, var_price);
            mean_vars[producer_id] = {mean_price, var_price};
        });
    }

    // single consumer thread pops events and records latency
    thread consumer([&]() {
        using ::_mm_pause;

        if (cfg.pin_threads) {
            pin_thread_physical(threadID_start+P, cpu_map); 
            //cout << "Consumer pinned to CPU " << cpu_map[threadID_start+P] << " : ";
            //cout << "Actual CPU: " << sched_getcpu() << endl;
        }        

        sync.arrive_and_wait();

        // start signal for producers
        start.store(true, memory_order_release);

        engine.run(totalMessages, 
                   queueStats, received, 
                   entryStats, entered, 
                   cancelStats, cancelled, 
                   trades, 
                   entry_best_bid_level_updates, entry_best_ask_level_updates,
                   cancel_best_bid_level_updates, cancel_best_ask_level_updates);
    });

    for (auto& t : producers) t.join();
    consumer.join();

    throughtputStat = (rdtsc() - now);


    sort(queueStats.begin(), queueStats.begin() + received);
    sort(entryStats.begin(), entryStats.begin() + entered);
    sort(cancelStats.begin(), cancelStats.begin() + cancelled);

    // generate report
    double GHz = measure_ghz();
    double MHz = GHz * 1000;
    double Hz = GHz * 1'000'000'000;
    

    cout << fixed << setprecision(2);
    cout << "CPU frequency: " << GHz << " GHz" << endl << endl;

    double mean_price = 0.0;
    double var_price = 0.0;
    for (int i = 0; i < P; ++i) {
        mean_price += mean_vars[i].first;
        var_price += mean_vars[i].second;
    }
    mean_price /= P;
    var_price /= P;

    cout << "Generated orders with" << endl;
    cout << "  mean price: " << mean_price << endl;
    cout << "  stddev:     " << sqrt(var_price) << endl << endl;

    cout << "Throughput: " << totalMessages/(throughtputStat/Hz) << " orders/sec" << endl;

    cout << "Queue latency:" << endl;
    cout << "p50   : " << percentile(queueStats, received, 0.50)/MHz << " µs" << endl;
    cout << "p99   : " << percentile(queueStats, received, 0.99)/MHz << " µs" << endl;
    cout << "p999  : " << percentile(queueStats, received, 0.999)/MHz << " µs" << endl;
    cout << "p9999 : " << percentile(queueStats, received, 0.9999)/MHz << " µs" << endl;

    cout << "Entry latency:" << endl;
    cout << "p50   : " << percentile(entryStats, entered, 0.50)/GHz << " ns" << endl;
    cout << "p99   : " << percentile(entryStats, entered, 0.99)/GHz << " ns" << endl;
    cout << "p999  : " << percentile(entryStats, entered, 0.999)/GHz << " ns" << endl;
    cout << "p9999 : " << percentile(entryStats, entered, 0.9999)/GHz << " ns" << endl;

    cout << "Cancel latency:" << endl;
    cout << "p50   : " << percentile(cancelStats, cancelled, 0.50)/GHz << " ns" << endl;
    cout << "p99   : " << percentile(cancelStats, cancelled, 0.99)/GHz << " ns" << endl;
    cout << "p999  : " << percentile(cancelStats, cancelled, 0.999)/GHz << " ns" << endl;
    cout << "p9999 : " << percentile(cancelStats, cancelled, 0.9999)/GHz << " ns" << endl;
    cout << endl;

    cout << "Total messages:  " << received << endl;
    cout << "Total entered:   " << entered << endl;
    cout << "Total cancelled: " << cancelled << endl;
    cout << "Total trades:    " << trades << endl;
    cout << "Entry best bid level updates:  " << entry_best_bid_level_updates << endl;
    cout << "Entry best ask level updates:  " << entry_best_ask_level_updates << endl;
    cout << "Cancel best bid level updates: " << cancel_best_bid_level_updates << endl;
    cout << "Cancel best ask level updates: " << cancel_best_ask_level_updates << endl;
}