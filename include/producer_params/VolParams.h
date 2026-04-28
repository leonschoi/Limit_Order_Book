#include <atomic>
#include <thread>
#include <vector>
#include <barrier>
#include <iostream>
#include <algorithm>
#include <cstdint>
#include <cmath>


/*
Volatility (struct VolParams)

Purpose:
Controls short-term price movement intensity and regime shifts.

Default parameters:
base    = 1;
jump    = 5;
max_vol = 25;
p_vol_shock   = 0.002;
p_vol_persist = 0.35;

Assume mid price = 2000

Parameters
| Parameter       | Description                                |
| --------------- | ------------------------------------------ |
| `base`          | Baseline volatility level                  |
| `jump`          | Size of volatility spike during shocks     |
| `max_vol`       | Hard cap on volatility                     |
| `p_vol_shock`   | Probability of entering a volatility spike |
| `p_vol_persist` | Probability volatility remains elevated    |

Volatility Regimes
| Market type       | base | jump  | Behavior                              |
| ----------------- | ---- | ----- | ------------------------------------- |
| Ultra-liquid      | 1    | 3–5   | Very stable, tight spreads            |
| Equity-style      | 2    | 5–10  | Normal regime shifts (earnings, news) |
| Crypto / high-vol | 3    | 10–20 | Large shocks, flash events            |

Persistence 
| Persistence level | `p_vol_persist` | Behavior               |
| ----------------- | --------------- | ---------------------- |
| Low               | 0.15–0.25       | Mean-reverting         |
| Medium            | 0.3–0.5         | Realistic default      |
| High              | 0.5–0.7         | Crisis / stress regime |

*/
struct VolParams {
    uint64_t shock_threshold;   // threshold for shock to occur
    uint64_t persist_threshold; // threshold for persistence to occur
    uint64_t base;              // base volatility level 
    uint64_t jump;              // jump size when shock happens 
    uint64_t max_vol;           // cap on volatility

    VolParams(double p_vol_shock = 0.01, 
              double p_vol_persist = 0.35, 
              uint64_t base = 1, 
              uint64_t jump = 5, 
              uint64_t max_vol = 25)
        : base(base), jump(jump), max_vol(max_vol) 
    {
        // Convert probabilities to integer thresholds for RNG comparison
        p_vol_shock = std::clamp(p_vol_shock, 0.0, 1.0);
        p_vol_persist = std::clamp(p_vol_persist, 0.0, 1.0);

        shock_threshold = static_cast<uint64_t>(p_vol_shock * UINT64_MAX);
        persist_threshold = static_cast<uint64_t>(p_vol_persist * UINT64_MAX);
    }
};
