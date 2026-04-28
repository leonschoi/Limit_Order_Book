#pragma once
#include <cstdint>
#include <map>
#include <unordered_map>

#include "Order.h"
#include "OrderList.h"
#include "OrderPool.h"


struct OrderBook_assoc_container {
    std::map<Price, OrderList, std::greater<>> bids;
    std::map<Price, OrderList> asks;

    std::unordered_map<OrderId, Order*> order_index;

    // price and level are used interchangeably for now.    
    OrderBook_assoc_container(size_t max_price, size_t max_orders) {}
};