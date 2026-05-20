#pragma once
#include <cstdint>
#include <cstddef>

struct alignas(64) Order {
    uint64_t order_id;
    uint32_t price;
    uint32_t quantity;
    uint64_t timestamp_ns;
    uint8_t side;      // 0 = BUY, 1 = SELL
    uint8_t type;      // 0 = LIMIT, 1 = MARKET
    uint8_t status;    // 0 = ACTIVE, 1 = FILLED, 2 = CANCELLED
    
    Order* next;
    Order* prev;
    
    Order() : order_id(0), price(0), quantity(0), timestamp_ns(0),
              side(0), type(0), status(0), next(nullptr), prev(nullptr) {}
    
    Order(uint64_t id, uint32_t p, uint32_t q, uint64_t ts, uint8_t s, uint8_t t)
        : order_id(id), price(p), quantity(q), timestamp_ns(ts),
          side(s), type(t), status(0), next(nullptr), prev(nullptr) {}
    
    void prefetch() const {
        __builtin_prefetch(this, 0, 3);
    }
};

struct PriceLevel {
    uint32_t price;
    uint32_t total_quantity;
    uint32_t order_count;
    Order* head;
    Order* tail;
    PriceLevel* next;
    PriceLevel* prev;
    
    PriceLevel() : price(0), total_quantity(0), order_count(0),
                   head(nullptr), tail(nullptr), next(nullptr), prev(nullptr) {}
};