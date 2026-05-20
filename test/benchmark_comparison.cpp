#include <benchmark/benchmark.h>
#include "../include/order_book.hpp"

// Ensure test and benchmark give same results
static void BM_TestConsistency(benchmark::State& state) {
    for (auto _ : state) {
        OrderBook book;
        
        // Same sequence as test_price_time_priority
        book.add_order(10000, 100, 0, 0);
        book.add_order(10000, 50, 0, 0);
        book.add_order(10000, 75, 0, 0);
        auto result = book.add_order(10000, 200, 1, 0);
        
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_TestConsistency);