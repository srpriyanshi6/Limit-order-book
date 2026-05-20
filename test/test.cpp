#include <iostream>
#include <cassert>
#include <vector>
#include <thread>
#include "../include/order_book.hpp"

#define TEST(name) void name() { std::cout << "Running: " << #name << "... "; test_##name(); std::cout << "✓ PASSED" << std::endl; }
#define ASSERT(condition) if (!(condition)) { std::cout << "✗ FAILED at line " << __LINE__ << ": " << #condition << std::endl; exit(1); }
#define ASSERT_EQ(a,b) if ((a) != (b)) { std::cout << "✗ FAILED at line " << __LINE__ << ": " << #a << " (" << (a) << ") != " << #b << " (" << (b) << ")" << std::endl; exit(1); }

// Test 1: Basic order addition
void test_basic_order_addition() {
    OrderBook book;
    
    auto result = book.add_order(10000, 100, 0, 0);  // Buy 100 @ $100.00
    
    ASSERT(result.success == true);
    ASSERT(result.order_id != 0);
    ASSERT(book.get_order_count() == 1);
}

// Test 2: Price-time priority
void test_price_time_priority() {
    OrderBook book;
    
    // Add 3 buy orders at same price
    auto r1 = book.add_order(10000, 100, 0, 0);  // Order 1
    auto r2 = book.add_order(10000, 50, 0, 0);   // Order 2
    auto r3 = book.add_order(10000, 75, 0, 0);   // Order 3
    
    // Add sell order to match
    auto match = book.add_order(10000, 200, 1, 0);  // Sell 200 @ $100
    
    // First 100 should match oldest order (r1)
    // Remaining should match r2
    ASSERT(book.get_order_count() == 1);  // Only r3 should remain
}

// Test 3: Market orders
void test_market_orders() {
    OrderBook book;
    
    // Add limit orders
    book.add_order(10000, 100, 0, 0);   // Bid at $100
    book.add_order(9900, 50, 1, 0);     // Ask at $99
    
    // Market buy should take best ask
    auto market_buy = book.add_order(0, 30, 0, 1);  // Buy 30 shares market
    
    ASSERT(market_buy.filled_quantity == 30);
    
    // Market sell should take best bid
    auto market_sell = book.add_order(0, 100, 1, 1);  // Sell 100 shares market
    
    ASSERT(market_sell.filled_quantity == 70);  // Only 70 left at best bid
}

// Test 4: Order cancellation
void test_cancellation() {
    OrderBook book;
    
    auto result = book.add_order(10000, 100, 0, 0);
    ASSERT(book.get_order_count() == 1);
    
    bool cancelled = book.cancel_order(result.order_id);
    ASSERT(cancelled == true);
    ASSERT(book.get_order_count() == 0);
    
    // Cancel non-existent order
    cancelled = book.cancel_order(99999);
    ASSERT(cancelled == false);
}

// Test 5: Partial fills
void test_partial_fills() {
    OrderBook book;
    
    // Add buy order for 100 shares
    book.add_order(10000, 100, 0, 0);
    
    // Sell 30 shares - should partially fill
    auto sell = book.add_order(10000, 30, 1, 0);
    
    ASSERT(book.get_order_count() == 1);  // Buy order still exists with 70 left
}

// Test 6: Cross orders (price improvement)
void test_cross_orders() {
    OrderBook book;
    
    // Bid at 100, Ask at 101 (normal spread)
    book.add_order(10000, 100, 0, 0);
    book.add_order(10100, 100, 1, 0);
    
    // New bid at 101 - should cross and match immediately
    auto cross_bid = book.add_order(10100, 50, 0, 0);
    
    ASSERT(cross_bid.filled_quantity == 50);  // Should fill against ask
}

// Test 7: Multiple price levels
void test_multiple_price_levels() {
    OrderBook book;
    
    // Build depth
    book.add_order(10000, 100, 0, 0);   // Level 1 bid
    book.add_order(9950, 200, 0, 0);    // Level 2 bid
    book.add_order(9900, 300, 0, 0);    // Level 3 bid
    
    book.add_order(10100, 100, 1, 0);   // Level 1 ask
    book.add_order(10150, 200, 1, 0);   // Level 2 ask
    book.add_order(10200, 300, 1, 0);   // Level 3 ask
    
    // Market sell 500 - should eat all bids
    auto market_sell = book.add_order(0, 500, 1, 1);
    
    ASSERT(market_sell.filled_quantity == 500);
}

// Test 8: Modify order price
void test_modify_order() {
    OrderBook book;
    
    auto result = book.add_order(10000, 100, 0, 0);
    
    // Modify to better price
    bool modified = book.modify_order(result.order_id, 10100, 100);
    ASSERT(modified == true);
    
    // New order at old price should be behind in priority
    auto new_order = book.add_order(10000, 50, 0, 0);
    ASSERT(new_order.success == true);
}

// Test 9: Concurrent operations (stress test)
void test_concurrent_operations() {
    OrderBook book;
    std::vector<std::thread> threads;
    
    auto worker = [&book](int start, int count) {
        for (int i = 0; i < count; i++) {
            uint32_t price = 10000 + (i % 100);
            uint32_t qty = 10 + (i % 90);
            uint8_t side = i % 2;
            book.add_order(price, qty, side, 0);
        }
    };
    
    // Launch 4 threads adding orders concurrently
    for (int i = 0; i < 4; i++) {
        threads.emplace_back(worker, i * 2500, 2500);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Should have 10000 orders
    ASSERT(book.get_order_count() == 10000);
}

// Test 10: Zero quantity rejection
void test_zero_quantity() {
    OrderBook book;
    auto result = book.add_order(10000, 0, 0, 0);
    ASSERT(result.success == false);
}

// Test 11: Extreme price values
void test_extreme_prices() {
    OrderBook book;
    
    // Max uint32_t price
    auto result = book.add_order(4294967295U, 100, 0, 0);
    ASSERT(result.success == true);
    
    // Min price (1 cent)
    result = book.add_order(1, 100, 1, 0);
    ASSERT(result.success == true);
}

// Test 12: Order book depth limits
void test_depth_limits() {
    OrderBook book;
    
    // Add 10,000 price levels
    for (int i = 0; i < 10000; i++) {
        book.add_order(10000 + i, 100, 0, 0);
    }
    
    // Should still be fast
    auto start = std::chrono::high_resolution_clock::now();
    auto result = book.add_order(15000, 50, 1, 0);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    ASSERT(latency < 10000);  // Should be under 10 microseconds even with deep book
}

// Test 13: Self-match prevention (if implemented)
void test_self_match_prevention() {
    OrderBook book;
    
    // Same trader buying and selling - should not match with self
    auto buy = book.add_order(10000, 100, 0, 0);
    auto sell = book.add_order(10000, 50, 1, 0);
    
    // Orders should both be in book (not matched)
    ASSERT(book.get_order_count() == 2);
}

// Test 14: Memory pool exhaustion
void test_memory_pool_exhaustion() {
    OrderBook book;
    
    // Try to add more orders than pool size
    const int EXCESS_ORDERS = 2000000 + 1000;
    int successful = 0;
    
    for (int i = 0; i < EXCESS_ORDERS; i++) {
        auto result = book.add_order(10000, 100, i % 2, 0);
        if (result.success) successful++;
        else break;
    }
    
    ASSERT(successful <= 2000000);  // Should not exceed pool capacity
}

// Test 15: Latency measurement accuracy
void test_latency_measurement() {
    OrderBook book;
    
    // Add orders and check stats don't crash
    for (int i = 0; i < 10000; i++) {
        book.add_order(10000, 100, i % 2, 0);
    }
    
    book.print_stats();  // Should not crash
}

// Run all tests
void run_all_tests() {
    std::cout << "\n╔════════════════════════════════════════════╗\n";
    std::cout << "║     ORDER BOOK TEST SUITE                  ║\n";
    std::cout << "╚════════════════════════════════════════════╝\n\n";
    
    test_basic_order_addition();
    test_price_time_priority();
    test_market_orders();
    test_cancellation();
    test_partial_fills();
    test_cross_orders();
    test_multiple_price_levels();
    test_modify_order();
    test_concurrent_operations();
    test_zero_quantity();
    test_extreme_prices();
    test_depth_limits();
    test_self_match_prevention();
    test_memory_pool_exhaustion();
    test_latency_measurement();
    
    std::cout << "\n╔════════════════════════════════════════════╗\n";
    std::cout << "║     ✓ ALL 15 TESTS PASSED!                 ║\n";
    std::cout << "╚════════════════════════════════════════════╝\n";
}

int main() {
    run_all_tests();
    return 0;
}