#pragma once
#include <immintrin.h>

#include "common/rdtsc.h"
#include "driver/BenchmarkConfig.h"
#include "queue/MPSC_bounded_ring.h"

#include "Message.h"
#include "OrderBook_assoc_container.h"

using namespace std;
using::_mm_pause;

template<typename Queue>
class MatchingEngine_assoc_container {
public:
    MatchingEngine_assoc_container(Queue& queue, const BenchmarkConfig& cfg)
        : queue(queue),
          book(cfg.max_price, cfg.producers * cfg.messages),
          pool(cfg.pool_size) {}

    void run(size_t totalMessages, 
             std::vector<uint64_t>& queueStats, size_t& received,
             std::vector<uint64_t>& entryStats, size_t& entered,
             std::vector<uint64_t>& cancelStats, size_t& cancelled,
             size_t& trades,
             size_t& entry_best_bid_level_updates,
             size_t& entry_best_ask_level_updates,
             size_t& cancel_best_bid_level_updates,
             size_t& cancel_best_ask_level_updates) 
    {
        uint64_t now;

        received = entered = cancelled = trades = 0;
        entry_best_bid_level_updates = entry_best_ask_level_updates = 0;
        cancel_best_bid_level_updates = cancel_best_ask_level_updates = 0;

        std::vector<Message> batch;
        batch.reserve(1024);

        while (received < totalMessages && running) {
            Message m;

            // drain queue
            while (queue.pop(m)) {
                queueStats[received++] = rdtsc() - m.ts_enqueue;
                batch.push_back(m);
            }

            // process batch
            for (auto& msg : batch) {
                switch (msg.type) {
                    case MsgType::NEW:
                        now = rdtsc();
                        handle_new(msg, trades, entry_best_bid_level_updates, entry_best_ask_level_updates);
                        entryStats[entered++] = rdtsc() - now;
                        break;                        
                    case MsgType::CANCEL:
                        now = rdtsc();
                        handle_cancel(msg, cancel_best_bid_level_updates, cancel_best_ask_level_updates);
                        cancelStats[cancelled++] = rdtsc() - now;
                        break;
                }                                
            }

            batch.clear();

            _mm_pause(); 
        }
    }

    void stop() {
        running = false;
    }

private:
    Queue& queue;

    OrderBook_assoc_container book;
    OrderPool pool;

    bool running = true;

    void handle_new(const Message& msg, 
                    size_t& trades, 
                    size_t& best_bid_level_updates, 
                    size_t& best_ask_level_updates) 
    {
        Order* o = pool.allocate();

        o->id = msg.id;
        o->side = msg.side;
        o->price = msg.price;
        o->qty = msg.qty;

        if (msg.side == Side::BUY) {
            match_buy(o, trades, best_ask_level_updates);

            if (o->qty > 0) {
                auto& level = book.bids[o->price];
                bool was_empty = level.empty();

                level.push_back(o);
                book.order_index[o->id] = o;

                if (was_empty) 
                    best_bid_level_updates++;
            } 
            else {
                pool.deallocate(o);
            }

        } else {
            match_sell(o, trades, best_bid_level_updates);

            if (o->qty > 0) {
                auto& level = book.asks[o->price];
                bool was_empty = level.empty();

                level.push_back(o);
                book.order_index[o->id] = o;

                if (was_empty) 
                    best_ask_level_updates++;
            } 
            else {
                pool.deallocate(o);
            }
        }
    }    

    void handle_cancel(const Message& msg, 
                       size_t& best_bid_level_updates, 
                       size_t& best_ask_level_updates) 
    {
        auto it = book.order_index.find(msg.id);
        if (it == book.order_index.end()) return; 

        Order* o = it->second;

        if (o->side == Side::BUY) {
            auto& book_side = book.bids;

            auto level_it = book_side.find(o->price);
            if (level_it != book_side.end()) {
                OrderList& level = level_it->second;

                level.remove(o);

                // if price level empty → remove level
                if (level.empty() && level_it == book_side.begin()) {
                    book_side.erase(level_it);  
                    best_bid_level_updates++;
                }
            }
        } 
        else {
            auto& book_side = book.asks;

            auto level_it = book_side.find(o->price);
            if (level_it != book_side.end()) {
                OrderList& level = level_it->second;

                level.remove(o);

                // if price level empty → remove level
                if (level.empty() && level_it == book_side.begin()) { 
                    book_side.erase(level_it); 
                    best_ask_level_updates++;
                }   
            }
        }

        book.order_index.erase(it);
        pool.deallocate(o);
    }    

    void match_buy(Order* incoming, 
                   size_t& trades, 
                   size_t& best_ask_level_updates) 
    {
        while (incoming->qty > 0 && !book.asks.empty()) {

            auto it = book.asks.begin();  // best ask (lowest price)
            Price ask_price = it->first;

            // stop if not marketable
            if (ask_price > incoming->price) break;

            OrderList& level = it->second;

            while (incoming->qty > 0 && !level.empty()) {
                Order* top = level.front(); 

                int traded = min(incoming->qty, top->qty);

                incoming->qty -= traded;
                top->qty -= traded;

                // fully filled resting order → remove it
                if (top->qty == 0) {
                    level.pop_front();
                    book.order_index.erase(top->id);
                    pool.deallocate(top);
                }

                trades++;
            }

            // if price level empty → remove level
            if (level.empty()) {
                book.asks.erase(it);  
                best_ask_level_updates++;
            }
        }
    }    

    void match_sell(Order* incoming, 
                    size_t& trades, 
                    size_t& best_bid_level_updates)
    {    
        while (incoming->qty > 0 && !book.bids.empty()) {

            auto it = book.bids.begin();  // best bid (highest price)
            Price bid_price = it->first;

            // stop if not marketable
            if (bid_price < incoming->price) break;

            OrderList& level = it->second;

            while (incoming->qty > 0 && !level.empty()) {
                Order* top = level.front(); 

                int traded = min(incoming->qty, top->qty);

                incoming->qty -= traded;
                top->qty -= traded;

                // fully filled resting order → remove it
                if (top->qty == 0) {
                    level.pop_front();
                    book.order_index.erase(top->id);
                    pool.deallocate(top);
                }

                trades++;
            }

            // if price level empty → remove level
            if (level.empty()) {
                book.bids.erase(it);  
                best_bid_level_updates++;
            }
        }
    }    
};