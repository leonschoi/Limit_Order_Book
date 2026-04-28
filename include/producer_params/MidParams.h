#include <atomic>
#include <thread>
#include <vector>
#include <barrier>
#include <iostream>
#include <algorithm>
#include <cstdint>
#include <cmath>



/*
Mid-Price (struct MidParams)

Purpose:
Controls directional drift and microstructure noise of the mid-price.

Default parameters:
direction_threshold = 0.5; // symmetric flips
drift_k             = 0.015;
noise_k             = 0.1;

Assume mid = 10k

Parameters
| Parameter             | Description                             |
| --------------------- | --------------------------------------- |
| `direction_threshold` | Probability of drift flipping direction |
| `drift_k`             | Strength of directional movement        |
| `noise_k`             | Magnitude of random fluctuations        |

Drift strength  (∝drift_k⋅v)
| `drift_k` | Behavior                 |
| --------- | ------------------------ |
| 0.005     | Nearly flat (pure noise) |
| 0.01–0.02 | Realistic intraday trend |
| 0.05      | Strong directional trend |

Noise level  (∝noise_k⋅v)
| `noise_k` | Behavior               |
| --------- | ---------------------- |
| 0.05      | Very stable            |
| 0.1–0.2   | Realistic market noise |
| 0.3       | Noisy / crypto-like    |

*/
struct MidParams {
    uint64_t direction_threshold; // threshold for drift direction change
    double drift_k;              // scaling factor for drift component of mid price (∝drift_k⋅v)
    double noise_k;              // scaling factor for noise component of mid price (∝noise_k⋅v)

    MidParams(double p_direction = 0.5, 
              double drift_k = 0.015, 
              double noise_k = 0.3)
        : drift_k(drift_k), noise_k(noise_k)
    {
        p_direction = std::clamp(p_direction, 0.0, 1.0);
        direction_threshold = static_cast<uint64_t>(p_direction * UINT64_MAX);
    }
};
