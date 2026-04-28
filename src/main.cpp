#include <iostream>
#include <string>
#include <memory>

#include "common/parse_args.h"
#include "queue/MPSC_bounded_ring.h"
#include "driver/run_benchmark.h"

using namespace std;

/*
Main

Usage:
    ./build/LimitOrderBook [options]

Options:
    --type assoc | flat | bitmap | all  (default: all)
        - assoc : map + unordered_map
        - flat  : flat array with direct index (no hashing)
        - bitmap: flat array + bitmap for price discovery (best bid/ask)
        - all   : run all three implementations sequentially
    --producers   <num>                           (default: 4)
    --messages    <num>                           (default: 1'000'000)
    --pin         true | false                    (default: true)
    --mid_price   <num>                           (default: 2'000)
    --max_price   <num>                           (default: 4'000)
    --pool_size   <num>                           (default: QUEUE_CAPACITY * 10)

Example:
    ./LimitOrderBook --type all
    ./LimitOrderBook --type flat --mid_price 2000 --max_price 4000
    ./LimitOrderBook --type bitmap --producers 4 --messages 10000 --pin true

*/
int main(int argc, char** argv) {
    constexpr size_t QUEUE_CAPACITY = 1 << 16; // MPSC queue capacity (must be power of 2 for ring buffer)

    BenchmarkConfig cfg {
        .type        = "all",               // "assoc", "flat", "bitmap", "all" (default: "all")
        .producers   = 4,                   // number of producer threads
        .messages    = 1'000'000,           // messages per producer
        .pin_threads = true,                // whether to pin threads to specific CPU cores for better isolation

        .mid_price   = 2'000,               // initial mid price (doubles as price level)
        .max_price   = 4'000,               // max price. Bitmap handles up to 4096
        .pool_size   = QUEUE_CAPACITY * 10  // order memory pool size
    };

    parse_args(argc, argv, cfg);

    using Q = MPSC_bounded_ring<Message, QUEUE_CAPACITY>;


    cout << boolalpha;
    cout.imbue(std::locale("en_US.UTF-8"));
 
    bool flag = false;

    if (cfg.type == "assoc" || cfg.type == "all") {
        cout << "=====================================" << endl;
        cout << "Running:   " << "LimitOrderBook associative container implementation" << endl;
        cout << "Producers: " << cfg.producers << endl;
        cout << "Messages:  " << cfg.messages << endl;
        cout << "Pinning:   " << cfg.pin_threads << endl;
        cout << "Mid price: " << cfg.mid_price << endl;
        cout << "Max price: " << cfg.max_price << endl;

        Q queue;
        OrderProducer<Q> producer(queue, cfg);
        MatchingEngine_assoc_container<Q> engine(queue, cfg);
        run_benchmark(producer, engine, cfg);
        flag = true;
        cout << "=====================================" << endl << endl;
    }
    if (cfg.type == "flat" || cfg.type == "all") {
        cout << "=====================================" << endl;
        cout << "Running:   " << "LimitOrderBook flat array implementation" << endl;
        cout << "Producers: " << cfg.producers << endl;
        cout << "Messages:  " << cfg.messages << endl;
        cout << "Pinning:   " << cfg.pin_threads << endl;
        cout << "Mid price: " << cfg.mid_price << endl;
        cout << "Max price: " << cfg.max_price << endl;

        Q queue;
        OrderProducer<Q> producer(queue, cfg);
        MatchingEngine_flat_array<Q> engine(queue, cfg);
        run_benchmark(producer, engine, cfg);
        flag = true;
        cout << "=====================================" << endl << endl;
    }
    if (cfg.type == "bitmap" || cfg.type == "all") {
        cout << "=====================================" << endl;
        cout << "Running:   " << "LimitOrderBook bitmap implementation" << endl;
        cout << "Producers: " << cfg.producers << endl;
        cout << "Messages:  " << cfg.messages << endl;
        cout << "Pinning:   " << cfg.pin_threads << endl;
        cout << "Mid price: " << cfg.mid_price << endl;
        cout << "Max price: " << cfg.max_price << endl;

        Q queue;
        OrderProducer<Q> producer(queue, cfg);
        MatchingEngine_bitmap<Q> engine(queue, cfg);
        run_benchmark(producer, engine, cfg);
        flag = true;
        cout << "=====================================" << endl << endl;
    }
    if (!flag) {
        cerr << "Invalid type: " << cfg.type << endl;
        return 1;
    }    

    return 0;
}