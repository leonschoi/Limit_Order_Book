#include <iostream>
#include <string>
#include <memory>

#include "common/parse_args.h"
#include "queue/MPSC_bounded_ring.h"
#include "driver/run_benchmark.h"

using namespace std;

template<typename Q, typename MatchingEngine>
void run(const string& name, const BenchmarkConfig& cfg);

/*
Usage:
    ./build/LimitOrderBook [options]

Options:
    --type        assoc | flat | bitmap | all   (default: all)
    --producers   <num>                         (default: 4)
    --messages    <num>                         (default: 1'000'000)
    --pin         true | false                  (default: true)
    --mid_price   <num>                         (default: 2'000)
    --max_price   <num>                         (default: 4'000)
    --pool_size   <num>                         (default: QUEUE_CAPACITY * 10)

Example:
    ./build/LimitOrderBook --type all
    ./build/LimitOrderBook --type flat --mid_price 2000 --max_price 4000
    ./build/LimitOrderBook --type bitmap --producers 4 --messages 10000 --pin true

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

    if (cfg.type == "assoc") {
        using MatchingEngine = MatchingEngine_assoc_container<Q>;

        run<Q, MatchingEngine>("LimitOrderBook associative container implementation", cfg);
    }
    else if (cfg.type == "flat") {
        using MatchingEngine = MatchingEngine_flat_array<Q>;

        run<Q, MatchingEngine>("LimitOrderBook flat array implementation", cfg);
    }
    else if (cfg.type == "bitmap") {
        using MatchingEngine = MatchingEngine_bitmap<Q>;
        
        run<Q, MatchingEngine>("LimitOrderBook bitmap implementation", cfg);
    }
    else if (cfg.type == "all") {
        using MatchingEngine_a = MatchingEngine_assoc_container<Q>;
        run<Q, MatchingEngine_a>("LimitOrderBook associative container implementation", cfg);

        using MatchingEngine_f = MatchingEngine_flat_array<Q>;
        run<Q, MatchingEngine_f>("LimitOrderBook flat array implementation", cfg);

        using MatchingEngine_b = MatchingEngine_bitmap<Q>;
        run<Q, MatchingEngine_b>("LimitOrderBook bitmap implementation", cfg);
    }
    else {
        cerr << "Invalid type: " << cfg.type << endl;
        cerr << "Usage:\n";
        cerr << "  ./build/LimitOrderBook --type [assoc|flat|bitmap|all] --producers [num] --messages [num] --pin [true|false]\n";
        return 1;
    }    

    return 0;
}

// Runner for benchmarks based on command line args
template<typename Q, typename MatchingEngine>
void run(const string& name, const BenchmarkConfig& cfg)
{
    cout << boolalpha;
    cout.imbue(std::locale("en_US.UTF-8"));
    cout << "=====================================" << endl;
    cout << "Running:   " << name << endl;
    cout << "Producers: " << cfg.producers << endl;
    cout << "Messages:  " << cfg.messages << endl;
    cout << "Pinning:   " << cfg.pin_threads << endl;
    cout << "Mid price: " << cfg.mid_price << endl;
    cout << "Max price: " << cfg.max_price << endl;
    cout << endl;

    Q queue;
    OrderProducer<Q> producer(queue, cfg);
    MatchingEngine engine(queue, cfg);
    run_benchmark(producer, engine, cfg);
    
    cout << "=====================================" << endl << endl;
}