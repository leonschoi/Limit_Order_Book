#pragma once
#include <cstdint>

#include "Order.h"

enum class MsgType { NEW, CANCEL };

struct Message {
    MsgType type;

    OrderId id;
    Side side;
    Price price;
    Qty qty;

    uint64_t ts_enqueue;
};

