#pragma once
#include <cstdint>
#include <vector>

#include "Order.h"


class OrderPool {
private:
    std::vector<Order> pool;
    Order* free_head = nullptr;

public:
    explicit OrderPool(size_t capacity) {
        pool.resize(capacity);

        // build free list
        for (size_t i = 0; i < capacity - 1; ++i) {
            pool[i].next = &pool[i + 1];
        }
        pool[capacity - 1].next = nullptr;

        free_head = &pool[0];
    }

    Order* allocate() {
        if (!free_head) {
            // optional: grow or fail
            throw std::runtime_error("OrderPool exhausted");
        }

        Order* o = free_head;
        free_head = free_head->next;

        // clean fields
        //o->next = o->prev = nullptr;

        return o;
    }

    void deallocate(Order* o) {
        // push back to free list
        o->next = free_head;
        free_head = o;
    }
};