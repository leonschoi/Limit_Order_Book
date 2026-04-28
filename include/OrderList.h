#pragma once
#include <cassert>

#include "Order.h"

// Intrusive list
struct OrderList {
    Order* head = nullptr;
    Order* tail = nullptr;

    bool empty() const { return head == nullptr; }

    void push_back(Order* o) {
        // assert(o->next == nullptr && o->prev == nullptr);

        o->next = nullptr;
        o->prev = tail;

        if (tail) tail->next = o;
        else head = o;

        tail = o;
    }

    void pop_front() {
        if (!head) return;

        Order* old = head;
        head = head->next;

        if (head) head->prev = nullptr;
        else tail = nullptr;

        old->next = old->prev = nullptr;
    }

    void remove(Order* o) {
        if (o->prev) o->prev->next = o->next;
        else head = o->next;

        if (o->next) o->next->prev = o->prev;
        else tail = o->prev;

        o->next = o->prev = nullptr;
    }

    Order* front() { return head; }
};
