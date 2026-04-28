#include <atomic>
#include <thread>
#include <vector>
#include <barrier>
#include <iostream>
#include <algorithm>
#include <cstdint>
#include <cmath>





/*
Quantity (struct QtyParams)

Purpose:
Generates order sizes using a power-law distribution.

Default parameters:
alpha   = 2.0
scale   = 50
max_qty = 500

Parameters
| Parameter | Description              |
| --------- | ------------------------ |
| `alpha`   | Tail exponent            |
| `scale`   | Typical order size       |
| `max_qty` | Maximum allowed quantity |

Tail shape
| `alpha` | Behavior                       |
| ------- | ------------------------------ |
| 1.3–1.6 | Heavy (crypto / retail bursts) |
| 1.8–2.2 | Realistic                      |
| 2.5+    | Too thin                       |

Scale
| `scale` | Behavior      |
| ------- | ------------- |
| 10      | Retail        |
| 50      | Mixed         |
| 100     | Institutional |

Max quantity
| `max_qty` | Behavior            |
| --------- | ------------------- |
| 5× scale  | Tight               |
| 10× scale | Realistic           |
| 20× scale | Allows block trades |


Balanced parameters for a realistic mix of retail and institutional flow:
alpha = 2.0;
scale = 50;
max_qty = 500;

Institutional-heavy flow (more large orders):
alpha = 1.7;
scale = 100;
max_qty = 2000;

Retail-heavy flow (more small orders):
alpha = 2.3;
scale = 20;
max_qty = 200;

*/
struct QtyParams {
    uint64_t snap_threshold; // threshold for snapping to common buckets
    double alpha; 
    double scale; 
    uint64_t max_qty; 

    QtyParams(double p_snap = 0.7, 
              double alpha = 2.0, 
              double scale = 50, 
              uint64_t max_qty = 500)
        : alpha(alpha), scale(scale), max_qty(max_qty) 
    {
        p_snap = std::clamp(p_snap, 0.0, 1.0);
        snap_threshold = static_cast<uint64_t>(p_snap * UINT64_MAX);
    }

    uint64_t nearest_bucket(uint64_t qty) {
        constexpr static const uint64_t buckets[] = {10, 20, 50, 100, 200, 500, 1000}; // need to match with max_qty for consistency
        constexpr static const uint64_t N = sizeof(buckets) / sizeof(buckets[0]);

        // find first element >= qty
        auto it = std::lower_bound(buckets, buckets + N, qty);

        if (it == buckets) return qty; // smaller than smallest bucket, return as is
        if (it == buckets + N) return buckets[N - 1];

        uint64_t higher = *it;
        uint64_t lower = *(it - 1);

        return (qty - lower <= higher - qty) ? lower : higher;
    }        
};
