#include <atomic>
#include <thread>
#include <vector>
#include <barrier>
#include <iostream>
#include <algorithm>
#include <cstdint>
#include <cmath>






/*
Order Flow (struct FlowParams)

Purpose:
Controls the mix of order types entering the book.

Default parameters:
p_buy = 0.5;    // 50% buy orders
p_cancel = 0.3; // 30% cancel orders

Parameters
| Parameter  | Description                  |
| ---------- | ---------------------------- |
| `p_buy`    | Probability of buy orders    |
| `p_cancel` | Probability of cancellations |

Interpretation
| Scenario        | Behavior                      |
| --------------- | ----------------------------- |
| `p_buy = 0.5`   | Balanced market               |
| `p_buy > 0.5`   | Buy pressure (bullish)        |
| `p_buy < 0.5`   | Sell pressure (bearish)       |
| High `p_cancel` | Fast-moving                   |
| Low `p_cancel`  | Sticky liquidity              |

*/
struct FlowParams {
    uint64_t buy_threshold;    // threshold for BUY vs SELL
    uint64_t cancel_threshold; // threshold for CANCEL vs NEW

    FlowParams(double p_buy = 0.5, 
               double p_cancel = 0.3)
    {
        p_buy = std::clamp(p_buy, 0.0, 1.0);
        p_cancel = std::clamp(p_cancel, 0.0, 1.0);

        buy_threshold = static_cast<uint64_t>(p_buy * UINT64_MAX);
        cancel_threshold = static_cast<uint64_t>(p_cancel * UINT64_MAX);
    }

};
