#pragma once
#include <cstdint>

using OrderId = uint64_t;
using Price   = int64_t;
using Qty     = int64_t;

enum class Side { BUY, SELL };

struct alignas(64) Order {
    // used in match loop, so keep these fields together for better cache locality
    Qty qty;
    Order* next = nullptr;
    Order* prev = nullptr;

    // less frequently accessed fields
    OrderId id;
    Side side;
    Price price;

    Order() = default;
    Order(const Order& other) = default;
};
