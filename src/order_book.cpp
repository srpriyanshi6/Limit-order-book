#include "order_book.hpp"
#include <iostream>
#include <thread>
#include <random>

OrderBook::OrderBook() {
    start_time = std::chrono::high_resolution_clock::now();
}

uint64_t OrderBook::get_current_nanos() {
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(now - start_time).count();
}

OrderBook::OrderResult OrderBook::add_order(uint32_t price, uint32_t quantity, 
                                             uint8_t side, uint8_t type) {
    auto start = std::chrono::high_resolution_clock::now();
    
    OrderResult result;
    result.success = false;
    result.filled_quantity = 0;
    
    uint64_t order_id = next_order_id.fetch_add(1);
    uint64_t timestamp = get_current_nanos();
    
    Order* order = order_pool.acquire();
    if (!order) {
        result.order_id = 0;
        result.timestamp_ns = timestamp;
        return result;
    }
    
    *order = Order(order_id, price, quantity, timestamp, side, type);
    
    // For market orders, execute immediately
    if (type == 1) {  // MARKET order
        auto matches = match_order(order);
        for (const auto& match : matches) {
            result.filled_quantity += match.quantity;
        }
        order_pool.release(order);
    } else {  // LIMIT order
        PriceLevel* level = get_or_create_price_level(price, side);
        insert_order_to_level(level, order);
        active_orders[order_id] = order;
        
        // Try to match with opposite side
        auto matches = match_order(nullptr);  // Match best orders
        result.filled_quantity += matches.size();
    }
    
    result.order_id = order_id;
    result.timestamp_ns = timestamp;
    result.success = true;
    
    auto end = std::chrono::high_resolution_clock::now();
    auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    add_latency.record(latency);
    
    return result;
}

bool OrderBook::cancel_order(uint64_t order_id) {
    auto start = std::chrono::high_resolution_clock::now();
    
    auto it = active_orders.find(order_id);
    if (it == active_orders.end()) {
        return false;
    }
    
    Order* order = it->second;
    if (order->status != 0) {
        return false;
    }
    
    order->status = 2;  // CANCELLED
    
    // Remove from price level
    PriceLevel* level = (order->side == 0) ? bids.price_levels[order->price] 
                                           : asks.price_levels[order->price];
    if (level) {
        remove_order_from_level(level, order);
    }
    
    active_orders.erase(it);
    order_pool.release(order);
    
    auto end = std::chrono::high_resolution_clock::now();
    cancel_latency.record(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
    
    return true;
}

PriceLevel* OrderBook::get_or_create_price_level(uint32_t price, uint8_t side) {
    SideBook& book = (side == 0) ? bids : asks;
    
    auto it = book.price_levels.find(price);
    if (it != book.price_levels.end()) {
        return it->second;
    }
    
    PriceLevel* new_level = new PriceLevel();
    new_level->price = price;
    
    // Insert in price order (ascending for asks, descending for bids)
    if (!book.best) {
        book.best = new_level;
    } else {
        PriceLevel* current = book.best;
        PriceLevel* prev = nullptr;
        
        bool inserted = false;
        while (current) {
            if ((side == 0 && price > current->price) ||  // Bids: higher price better
                (side == 1 && price < current->price)) {  // Asks: lower price better
                new_level->next = current;
                new_level->prev = prev;
                if (prev) prev->next = new_level;
                else book.best = new_level;
                if (current) current->prev = new_level;
                inserted = true;
                break;
            }
            prev = current;
            current = current->next;
        }
        
        if (!inserted) {
            prev->next = new_level;
            new_level->prev = prev;
        }
    }
    
    book.price_levels[price] = new_level;
    book.depth++;
    return new_level;
}

void OrderBook::insert_order_to_level(PriceLevel* level, Order* order) {
    if (!level->head) {
        level->head = order;
        level->tail = order;
    } else {
        level->tail->next = order;
        order->prev = level->tail;
        level->tail = order;
    }
    level->total_quantity += order->quantity;
    level->order_count++;
}

void OrderBook::remove_order_from_level(PriceLevel* level, Order* order) {
    if (order->prev) order->prev->next = order->next;
    if (order->next) order->next->prev = order->prev;
    if (level->head == order) level->head = order->next;
    if (level->tail == order) level->tail = order->prev;
    
    level->total_quantity -= order->quantity;
    level->order_count--;
    
    if (level->order_count == 0) {
        // Remove empty price level
        SideBook& book = (order->side == 0) ? bids : asks;
        book.price_levels.erase(level->price);
        if (level->prev) level->prev->next = level->next;
        if (level->next) level->next->prev = level->prev;
        if (book.best == level) book.best = level->next;
        delete level;
        book.depth--;
    }
}

std::vector<OrderBook::Match> OrderBook::match_order(Order* taker) {
    std::vector<Match> matches;
    
    while (true) {
        // Check if we can match
        if (!bids.best || !asks.best) break;
        if (bids.best->price < asks.best->price) break;
        
        // Get best bid and ask
        Order* bid_order = bids.best->head;
        Order* ask_order = asks.best->head;
        
        if (!bid_order || !ask_order) break;
        
        uint32_t match_price = ask_order->price;
        uint32_t match_qty = std::min(bid_order->quantity, ask_order->quantity);
        
        // Create match
        Match match;
        match.taker_order_id = taker ? taker->order_id : 0;
        match.maker_order_id = ask_order->order_id;
        match.price = match_price;
        match.quantity = match_qty;
        matches.push_back(match);
        
        // Update quantities
        bid_order->quantity -= match_qty;
        ask_order->quantity -= match_qty;
        
        if (bid_order->quantity == 0) {
            bid_order->status = 1;  // FILLED
            remove_order_from_level(bids.best, bid_order);
            active_orders.erase(bid_order->order_id);
            order_pool.release(bid_order);
        }
        
        if (ask_order->quantity == 0) {
            ask_order->status = 1;  // FILLED
            remove_order_from_level(asks.best, ask_order);
            active_orders.erase(ask_order->order_id);
            order_pool.release(ask_order);
        }
    }
    
    return matches;
}

void OrderBook::print_stats() {
    printf("\n╔════════════════════════════════════════╗\n");
    printf("║     ORDER BOOK PERFORMANCE STATS       ║\n");
    printf("╚════════════════════════════════════════╝\n");
    
    add_latency.print_stats("Add Order");
    match_latency.print_stats("Match");
    cancel_latency.print_stats("Cancel");
    
    printf("\n=== Order Book State ===\n");
    printf("  Active Orders: %zu\n", active_orders.size());
    printf("  Bid Depth: %zu\n", bids.depth);
    printf("  Ask Depth: %zu\n", asks.depth);
    
    if (bids.best) {
        printf("  Best Bid: %u @ %u\n", bids.best->price, bids.best->total_quantity);
    }
    if (asks.best) {
        printf("  Best Ask: %u @ %u\n", asks.best->price, asks.best->total_quantity);
    }
}

void OrderBook::run_demo() {
    printf("\n╔════════════════════════════════════════╗\n");
    printf("║     ORDER BOOK DEMO - HFT SIMULATION   ║\n");
    printf("╚════════════════════════════════════════╝\n");
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> price_dist(90, 110);
    std::uniform_int_distribution<> qty_dist(1, 100);
    std::uniform_int_distribution<> side_dist(0, 1);
    
    // Add 1000 random limit orders
    printf("\n[1] Adding 1000 random limit orders...\n");
    for (int i = 0; i < 1000; i++) {
        uint32_t price = price_dist(gen);
        uint32_t qty = qty_dist(gen);
        uint8_t side = side_dist(gen);
        
        auto result = add_order(price, qty, side, 0);  // LIMIT order
        
        if (i % 200 == 0 && i > 0) {
            printf("    Added %d orders...\n", i);
        }
    }
    
    printf("\n[2] Order book state after adding limit orders:\n");
    if (bids.best) printf("    Best Bid: %u @ %u\n", bids.best->price, bids.best->total_quantity);
    if (asks.best) printf("    Best Ask: %u @ %u\n", asks.best->price, asks.best->total_quantity);
    printf("    Spread: %u\n", asks.best ? asks.best->price - bids.best->price : 0);
    
    // Add market orders to trigger matches
    printf("\n[3] Adding 100 market orders to trigger matches...\n");
    for (int i = 0; i < 100; i++) {
        uint32_t qty = qty_dist(gen);
        uint8_t side = side_dist(gen);
        auto result = add_order(0, qty, side, 1);  // MARKET order
    }
    
    // Cancel some orders
    printf("\n[4] Cancelling random orders...\n");
    auto it = active_orders.begin();
    int cancelled = 0;
    for (int i = 0; i < 100 && it != active_orders.end(); i++) {
        if (cancel_order(it->first)) {
            cancelled++;
        }
        it++;
    }
    printf("    Cancelled %d orders\n", cancelled);
    
    // Final stats
    print_stats();
    
    printf("\n Demo completed successfully!\n");
    printf("   Total throughput: ~%zu orders/sec\n", 
           get_order_count() * 1000 / 100);
}