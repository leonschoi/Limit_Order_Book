#pragma once
#include <cstdint>
#include <vector>

#include "Order.h"
#include "OrderList.h"
#include "OrderPool.h"


struct OrderBook_flat_array {
    std::vector<OrderList> bid_levels;
    std::vector<OrderList> ask_levels;

    std::vector<Order*> order_index; // contains all orders for reference in cancelation

    int64_t best_bid_level = -1;
    int64_t best_ask_level = -1;    

    // price and level are used interchangeably for now.    
    OrderBook_flat_array(size_t max_price, size_t max_orders)
        : bid_levels(max_price + 1),  // price levels = max_price + 1 (including 0)
          ask_levels(max_price + 1),
          order_index(max_orders, nullptr) {}
};

