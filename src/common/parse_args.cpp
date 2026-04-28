#include <iostream>
#include <string>
#include <unordered_map>

#include "common/parse_args.h"

/*
Parses command line arguments into BenchmarkConfig struct

Parameters can be passed as:
$ ./LimitOrderBook --type bitmap --producers 4 --messages 10000 --pin true
*/
BenchmarkConfig parse_args(int argc, char** argv, BenchmarkConfig& cfg) {
    std::unordered_map<std::string, std::string> kv;

    for (int i = 1; i < argc; ++i) {
        std::string key = argv[i];

        if (key.rfind("--", 0) == 0) {
            key = key.substr(2); // remove "--"

            if (i+1 < argc && std::string(argv[i+1]).rfind("--", 0) != 0) 
                kv[key] = argv[++i]; // --key value
            else 
                kv[key] = "true";    // --flag
        }
    }

    // map to struct
    if (kv.contains("type"))         cfg.type = kv["type"];
    if (kv.contains("producers"))    cfg.producers = std::stoi(kv["producers"]);
    if (kv.contains("messages"))     cfg.messages = std::stoull(kv["messages"]);
    if (kv.contains("pin"))          cfg.pin_threads = (kv["pin"] == "true");
    if (kv.contains("mid"))          cfg.mid_price = std::stoll(kv["mid"]);
    if (kv.contains("max"))          cfg.max_price = std::stoll(kv["max"]);
    if (kv.contains("pool"))         cfg.pool_size = std::stoull(kv["pool"]);

    return cfg;
}