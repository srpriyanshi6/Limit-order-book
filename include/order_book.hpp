#pragma once
#include <unordered_map>
#include <vector>
#include <atomic>
#include <chrono>
#include "order.hpp"
#include "memory_pool.hpp"
#include "latency_stats.hpp"

class OrderBook {
private:
    struct alignas(64) SideBook {
        PriceLevel* best;
        std::unordered_map<uint32_t, PriceLevel*> price_levels;
        size_t depth;
        
        SideBook() : best(nullptr), depth(0) {}
    };
    
    alignas(64) SideBook bids;
    alignas(64) SideBook asks;
    
    MemoryPool<Order, 2000000> order_pool;
    std::unordered_map<uint64_t, Order*> active_orders;
    std::atomic<uint64_t> next_order_id{1};
    
    // Statistics
    LatencyStats add_latency;
    LatencyStats match_latency;
    LatencyStats cancel_latency;
    
    std::chrono::high_resolution_clock::time_point start_time;
    
public:
    OrderBook();
    
    // Core operations with nanosecond timestamps
    struct OrderResult {
        uint64_t order_id;
        uint64_t timestamp_ns;
        uint32_t filled_quantity;
        bool success;
    };
    
    OrderResult add_order(uint32_t price, uint32_t quantity, uint8_t side, uint8_t type);
    bool cancel_order(uint64_t order_id);
    bool modify_order(uint64_t order_id, uint32_t new_price, uint32_t new_quantity);
    
    // Matching
    struct Match {
        uint64_t taker_order_id;
        uint64_t maker_order_id;
        uint32_t price;
        uint32_t quantity;
    };
    
    std::vector<Match> match_order(Order* taker);
    
    // Stats
    void print_stats();
    size_t get_order_count() const { return active_orders.size(); }
    
    // Demo mode
    void run_demo();
    
private:
    PriceLevel* get_or_create_price_level(uint32_t price, uint8_t side);
    void insert_order_to_level(PriceLevel* level, Order* order);
    void remove_order_from_level(PriceLevel* level, Order* order);
    uint64_t get_current_nanos();
};