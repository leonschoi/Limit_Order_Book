#include <algorithm>
#include <cstdint>

/*
Fat Tails (struct FatTailParams)

Purpose:
Introduces rare but large price jumps (power-law behavior).

Default parameters:
p_tail     = 0.01; // 1% of orders have fat tails
alpha      = 2.0;  // tail exponent (power-law)
k          = 0.1;  // tail size scale (∝v)
max_tail_k = 5.0;  // cap tail size to 5x volatility

Parameters
| Parameter    | Description                      |
| ------------ | -------------------------------- |
| `p_tail`     | Probability of a fat-tail event  |
| `alpha`      | Power-law exponent               |
| `k`          | Tail size relative to volatility |
| `max_tail_k` | Maximum tail size (cap)          |

Tail heaviness
| `alpha` | Behavior                            |
| ------- | ----------------------------------- |
| 1.2–1.5 | Extremely heavy tails (crash-prone) |
| 1.8–2.2 | Realistic                           |
| 2.5–3.0 | Mild                                |

Tail scale (∝k⋅v)
| `k`     | Behavior   |
| ------- | ---------- |
| 0.05    | Subtle     |
| 0.1–0.2 | Realistic  |
| 0.3     | Aggressive |

Tail scale (∝max_tail_k⋅v)
| `max_tail_k` | Behavior       |
| ------------ | -------------- |
| 2–3          | Tight control  |
| 4–6          | Realistic      |
| 8–10         | Extreme stress |

*/
struct FatTailParams {
    uint64_t tail_threshold;  // threshold for tail event
    double alpha;             // power-law exponent for tail size distribution
    double k;                 // scale factor for tail size (∝k⋅v)
    double max_tail_k;        // scale factor for maximum tail size (∝max_tail_k⋅v)

    FatTailParams(double p_tail = 0.2, 
                  double alpha = 2.0, 
                  double k = 0.3, 
                  double max_tail_k = 10.0)
        : alpha(alpha), k(k), max_tail_k(max_tail_k)
    {
        p_tail = std::clamp(p_tail, 0.0, 1.0);
        tail_threshold = static_cast<uint64_t>(p_tail * UINT64_MAX);
    }
};

