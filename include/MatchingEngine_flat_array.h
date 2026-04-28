#pragma once
#include <immintrin.h>

#include "common/rdtsc.h"
#include "driver/BenchmarkConfig.h"
#include "queue/MPSC_bounded_ring.h"

#include "Message.h"
#include "OrderBook_flat_array.h"

using namespace std;
using::_mm_pause;

template<typename Queue>
class MatchingEngine_flat_array {
public:
    MatchingEngine_flat_array(Queue& queue, const BenchmarkConfig& cfg)
        : queue(queue),
          book(cfg.max_price, cfg.producers * cfg.messages),
          pool(cfg.pool_size),
          max_index(price_to_index(cfg.max_price)) {}


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

    OrderBook_flat_array book;
    OrderPool pool;
    int64_t max_index; 

    bool running = true;

    // price and level are used interchangeably for now.    
    int64_t price_to_index(Price p) const {
        return (int64_t)p;
    }
    Price index_to_price(int64_t px) const {
        return (Price)px; 
    }

    void handle_new(const Message& msg, 
                    size_t& trades, 
                    size_t& best_bid_level_updates, 
                    size_t& best_ask_level_updates) 
    {
        Order* o = pool.allocate();

        o->id    = msg.id;
        o->side  = msg.side;
        o->price = msg.price;
        o->qty   = msg.qty;

        int64_t px = price_to_index(o->price);

        if (msg.side == Side::BUY) {
            match_buy(o, trades, best_ask_level_updates);

            if (o->qty > 0) {
                auto& level = book.bid_levels[px];
                bool was_empty = level.empty();

                level.push_back(o);
                book.order_index[o->id] = o;

                // update best bid
                if (was_empty && (book.best_bid_level == -1 || px > book.best_bid_level)) {
                    book.best_bid_level = px;

                    // no need for best_bid_level_updates++ since no data structure modified. 
                }
            } 
            else 
                pool.deallocate(o);

        } else {
            match_sell(o, trades, best_bid_level_updates);

            if (o->qty > 0) {
                auto& level = book.ask_levels[px];
                bool was_empty = level.empty();

                level.push_back(o);
                book.order_index[o->id] = o;

                // update best ask
                if (was_empty && (book.best_ask_level == -1 || px < book.best_ask_level)) {
                    book.best_ask_level = px;

                    // no need for best_ask_level_updates++ since no data structure modified. 
                }
            } 
            else 
                pool.deallocate(o);
        }
    }

    void handle_cancel(const Message& msg, 
                       size_t& best_bid_level_updates, 
                       size_t& best_ask_level_updates) 
    {
        if (msg.id >= book.order_index.size()) return;

        Order* o = book.order_index[msg.id];
        if (!o) return;

        int64_t px = price_to_index(o->price);

        if (o->side == Side::BUY) {
            auto& level = book.bid_levels[px];
            level.remove(o);

            if (level.empty() && px == book.best_bid_level) {
                int64_t i = book.best_bid_level - 1;
                while (i >= 0 && book.bid_levels[i].empty()) --i; 

                book.best_bid_level = i;
                best_bid_level_updates++;
            }

        } else {
            auto& level = book.ask_levels[px];
            level.remove(o);

            if (level.empty() && px == book.best_ask_level) {
                int64_t i = book.best_ask_level + 1;
                while (i <= max_index && book.ask_levels[i].empty()) ++i; 

                book.best_ask_level = (i <= max_index) ? i : -1;
                best_ask_level_updates++;
            }
        }

        book.order_index[msg.id] = nullptr;
        pool.deallocate(o);
    }    

    void match_buy(Order* incoming, 
                   size_t& trades, 
                   size_t& best_ask_level_updates) 
    {
        while (incoming->qty > 0 && book.best_ask_level != -1) {

            int64_t px = book.best_ask_level;

            // stop if not marketable
            if (index_to_price(px) > incoming->price) break;

            OrderList& level = book.ask_levels[px];

            while (incoming->qty > 0 && !level.empty()) {
                Order* top = level.front();

                int traded = std::min(incoming->qty, top->qty);

                incoming->qty -= traded;
                top->qty -= traded;

                // fully filled resting order → remove it
                if (top->qty == 0) {
                    level.pop_front();
                    book.order_index[top->id] = nullptr;
                    pool.deallocate(top);
                }
            
                trades++;
            }

            // if price level empty → move best_ask_level to next non-empty level
            if (level.empty()) {
                int64_t i = px + 1;
                while (i <= max_index && book.ask_levels[i].empty()) ++i; 

                book.best_ask_level = (i <= max_index) ? i : -1;
                best_ask_level_updates++;
            }
        }
    }    

    void match_sell(Order* incoming, 
                    size_t& trades, 
                    size_t& best_bid_level_updates)
    {    
        while (incoming->qty > 0 && book.best_bid_level != -1) {

            int64_t px = book.best_bid_level;

            // stop if not marketable
            if (index_to_price(px) < incoming->price) break;

            OrderList& level = book.bid_levels[px];

            while (incoming->qty > 0 && !level.empty()) {
                Order* top = level.front();

                int traded = std::min(incoming->qty, top->qty);

                incoming->qty -= traded;
                top->qty -= traded;

                // fully filled resting order → remove it
                if (top->qty == 0) {
                    level.pop_front();
                    book.order_index[top->id] = nullptr;
                    pool.deallocate(top);
                }

                trades++;
            }

            // if price level empty → move best_bid_level to next non-empty level
            if (level.empty()) {
                int64_t i = px - 1;
                while (i >= 0 && book.bid_levels[i].empty()) --i; 

                book.best_bid_level = i;
                best_bid_level_updates++;
            }
        }
    }    
};

