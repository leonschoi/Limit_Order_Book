#pragma once
#include <atomic>
#include <thread>
#include <vector>
#include <barrier>
#include <iostream>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <immintrin.h>
#include <random>

#include "driver/benchmark.h"
#include "producer_params/ProducerConfig.h"
#include "queue/MPSC_bounded_ring.h"

#include "Message.h"

using namespace std;

template<typename Q>
class OrderProducer {
public:
    OrderProducer(Q& q,  const BenchmarkConfig& bcfg) : queue(q) {
        mid_price = bcfg.mid_price;
        max_price = bcfg.max_price;
    }

    void run(int producer_id, size_t N, double& mean_price, double& var_price) {
        using ::_mm_pause;

        mean_price = 0.0;
        var_price = 0.0;

        // For online mean and variance calculation of generated prices
        double M2   = 0.0;

        //uint64_t rng = std::random_device{}(); 
        uint64_t rng = rotl(0x2E28C91D5F2B8A64ULL, producer_id); // to be more deterministic

        uint64_t v = cfg.vol.base; // start at base volatility
        int drift_direction = 1;  // 1 for upward drift, -1 for downward

        // track live orders for cancellations
        std::vector<OrderId> live_orders;
        live_orders.reserve(N);

        for (size_t i = 0; i < N; ++i) {
            Message m;

            // cancelation decision based on the presence of live orders and random threshold
            if ((!live_orders.empty()) && (xorshift64(rng) < cfg.flow.cancel_threshold)) {
                m.type = MsgType::CANCEL;

                // pick random live order
                size_t idx = xorshift64(rng) % live_orders.size();
                m.id = live_orders[idx];

                // remove from live_orders list
                live_orders[idx] = live_orders.back();
                live_orders.pop_back();
            } 
            else {
                m.type = MsgType::NEW;

                // unique order ID
                m.id = producer_id * N + i;

                // buy/sell side based on random threshold
                m.side = (xorshift64(rng) < cfg.flow.buy_threshold) ? Side::BUY : Side::SELL;


                //
                // update volatility level based on random shocks and persistence
                //
                uint64_t v_next = v;
                
                // exogenous volatility shock
                if (xorshift64(rng) < cfg.vol.shock_threshold) {
                    v_next += cfg.vol.jump; 
                }
                // clustering (persistence)
                else if (v > cfg.vol.base && xorshift64(rng) < cfg.vol.persist_threshold) {
                    v_next += 1;  
                }

                // decay
                if (v_next > 1) v_next -= 1;

                // clamp
                v = std::min(v_next, cfg.vol.max_vol);


                //
                // gaussian simulation using central limit theorem. z ≈ N(0, 1)
                //                
                uint64_t g =
                    (rng & 0xFF) +
                    ((rng >> 8) & 0xFF) +
                    ((rng >> 16) & 0xFF) +
                    ((rng >> 24) & 0xFF);            
                double z = (g/4.0 - 128) / 36.8;


                //
                // mid price with drift and noise
                //
                if (xorshift64(rng) < cfg.mid.direction_threshold) {
                    drift_direction = -drift_direction;
                }
                double drift = drift_direction * cfg.mid.drift_k * v; 

                if (xorshift64(rng) < cfg.mid.direction_threshold) {
                    drift_direction = -drift_direction;
                }
                double noise = drift_direction * z * cfg.mid.noise_k * v;

                mid_price += drift + noise;


                //
                // fat tail
                //
                //if (std::generate_canonical<double, 64>(rn) < 0.2) {
                if (xorshift64(rng) < cfg.fat.tail_threshold) {
                    double u = (xorshift64(rng) & 0xFFFF) * INV_65536; // uniform [0,1) based on rng

                    double tail = pow_fast(u, -cfg.fat.alpha) * cfg.fat.k * v;
                    tail = std::min(tail, cfg.fat.max_tail_k * v);

                    int sign = (xorshift64(rng) & 1) ? 1 : -1; // random direction for tail
                    //int sign = bin_dist(rn) ? 1.0 : -1.0; // random direction for tail

                    mid_price += sign * tail;
                }

                m.price = std::clamp(static_cast<int64_t>(mid_price), static_cast<int64_t>(0), max_price);                
                
                // Price granularity of 256 to test best/ask level updates of 
                //    flat array O(n) and bitmap + flat array O(1)
                m.price &= (UINT64_MAX)^(256-1); 

                //
                // quantity with heavy-tailed distribution
                //
                double u = (xorshift64(rng) & 0xFFFF) * INV_65536; // uniform [0,1) based on rng
                uint64_t qty = static_cast<uint64_t>(pow_fast(u, -cfg.qty.alpha) * cfg.qty.scale);

                if (xorshift64(rng) < cfg.qty.snap_threshold) 
                    m.qty = cfg.qty.nearest_bucket(qty);
                else 
                    m.qty = std::min(qty, cfg.qty.max_qty);


                live_orders.push_back(m.id);

                // Welford's algorithm for online mean and variance calculation
                double delta = m.price - mean_price;
                mean_price += delta / (i+1);
                double delta2 = m.price - mean_price;
                M2 += delta * delta2;
            }

            m.ts_enqueue = rdtsc();

            while (!queue.push(m)) _mm_pause();
        }

        var_price = M2 / (N - 1);  // sample variance
    }    

private:
    Q& queue;
    ProducerConfig cfg;
    double mid_price;  // initial mid price
    int64_t max_price; // max price

    // Simple xorshift RNG 
    constexpr uint32_t xorshift32(uint32_t& state) {
        uint32_t x = state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        return state = x;
    }
    constexpr uint64_t xorshift64(uint64_t& state) {
        uint64_t x = state;
        x ^= x >> 12;
        x ^= x << 25;
        x ^= x >> 27;
        return state = x;
    }    

    constexpr double pow_fast(double base, double exp) {
        return std::exp(exp * std::log(base));
    }
    constexpr static double INV_65536 = 1.0 / 65536.0;

};