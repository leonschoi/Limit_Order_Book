#include "VolParams.h"
#include "MidParams.h"
#include "FatTailParams.h"
#include "QtyParams.h"
#include "FlowParams.h"

struct ProducerConfig {
    VolParams     vol{};
    MidParams     mid{};
    FatTailParams fat{};
    QtyParams     qty{};
    FlowParams    flow{};
};

/*
🔹  How to Use
- Start with Balanced Equity
- Adjust:
  - vol.jump + p_vol_persist → regime intensity
  - qty.alpha                → order size distribution
  - flow.p_cancel            → market speed


🔹 1. Ultra-Liquid (HFT / Large Cap)
Characteristics
- Tight spreads
- Low volatility
- High cancellations
- Small, frequent trades

    VolParams vol {
        .base = 1,
        .jump = 3,
        .max_vol = 15,
        .p_vol_shock = 0.001,
        .p_vol_persist = 0.25
    };

    MidParams mid {
        .direction_threshold = 0.5,
        .drift_k = 0.005,
        .noise_k = 0.05
    };

    FatTailParams fat {
        .p_tail = 0.005,
        .alpha = 2.5,
        .k     = 0.05,
        .max_tail_k = 3
    };

    QtyParams qty {
        .alpha = 2.3,
        .scale = 20,
        .max_qty = 200
    };

    FlowParams flow {
        .p_buy = 0.5,
        .p_cancel = 0.5
    };


🔹 2. Balanced Equity Market (Default)
Characteristics
- Realistic equity behavior
- Moderate spreads and volatility
- Mix of retail + institutional flow

    VolParams vol {
        .base = 2,
        .jump = 7,
        .max_vol = 25,
        .p_vol_shock = 0.002,
        .p_vol_persist = 0.35
    };

    MidParams mid {
        .direction_threshold = 0.5,
        .drift_k = 0.015,
        .noise_k = 0.1
    };

    FatTailParams fat {
        .p_tail = 0.01,
        .alpha = 2.0,
        .k     = 0.1,
        .max_tail_k = 5
    };

    QtyParams qty {
        .alpha = 2.0,
        .scale = 50,
        .max_qty = 500
    };

    FlowParams flow {
        .p_buy = 0.5,
        .p_cancel = 0.3
    };


🔹 3. Institutional-Dominated Market
Characteristics
- Larger order sizes
- More persistent trends
- Lower cancellation rate
- Occasional large moves

    VolParams vol {
        .base = 2,
        .jump = 10,
        .max_vol = 30,
        .p_vol_shock = 0.003,
        .p_vol_persist = 0.45
    };

    MidParams mid {
        .direction_threshold = 0.45,
        .drift_k = 0.02,
        .noise_k = 0.08
    };

    FatTailParams fat {
        .p_tail = 0.015,
        .alpha = 1.8,
        .k     = 0.15,
        .max_tail_k = 6
    };

    QtyParams qty {
        .alpha = 1.7,
        .scale = 100,
        .max_qty = 2000
    };

    FlowParams flow {
        .p_buy = 0.5,
        .p_cancel = 0.2
    };


🔹 4. Retail / Crypto-Like Market
Characteristics
- High volatility
- Noisy price action
- Heavy-tailed order sizes
- Frequent spikes

    VolParams vol {
        .base = 3,
        .jump = 15,
        .max_vol = 50,
        .p_vol_shock = 0.005,
        .p_vol_persist = 0.4
    };

    MidParams mid {
        .direction_threshold = 0.5,
        .drift_k = 0.01,
        .noise_k = 0.25
    };

    FatTailParams fat {
        .p_tail = 0.02,
        .alpha = 1.5,
        .k     = 0.2,
        .max_tail_k = 8
    };

    QtyParams qty {
        .alpha = 1.5,
        .scale = 30,
        .max_qty = 1000
    };

    FlowParams flow {
        .p_buy = 0.5,
        .p_cancel = 0.25
    };


🔹 5. Stress / Crisis Scenario
Characteristics
- Extreme volatility spikes
- Wide spreads
- Strong directional moves
- Liquidity deterioration

    VolParams vol {
        .base = 4,
        .jump = 20,
        .max_vol = 80,
        .p_vol_shock = 0.01,
        .p_vol_persist = 0.65
    };

    MidParams mid {
        .direction_threshold = 0.4,
        .drift_k = 0.05,
        .noise_k = 0.2
    };

    FatTailParams fat {
        .p_tail = 0.05,
        .alpha = 1.3,
        .k     = 0.3,
        .max_tail_k = 10
    };

    QtyParams qty {
        .alpha = 1.6,
        .scale = 120,
        .max_qty = 3000
    };

    FlowParams flow {
        .p_buy = 0.35,
        .p_cancel = 0.4
    };


*/