#include "order_book.hpp"
#include <iostream>
#include <chrono>
#include <thread>

void print_banner() {
    printf("\n");
    printf("  ╔═══════════════════════════════════════════════════════╗\n");
    printf("  ║     HIGH-FREQUENCY TRADING ORDER BOOK ENGINE          ║\n");
    printf("  ║     C++20 | Lock-free | Cache-Optimized               ║\n");
    printf("  ╚═══════════════════════════════════════════════════════╝\n");
    printf("\n");
}

int main(int argc, char* argv[]) {
    print_banner();
    
    OrderBook book;
    
    // Check for command line arguments
    bool run_benchmark = false;
    int num_orders = 10000;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--benchmark" || arg == "-b") {
            run_benchmark = true;
        } else if (arg == "--orders" && i + 1 < argc) {
            num_orders = std::atoi(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            printf("Usage: ./orderbook [options]\n");
            printf("  --benchmark, -b    Run performance benchmark\n");
            printf("  --orders N         Number of orders to process (default: 10000)\n");
            printf("  --help, -h         Show this help\n");
            return 0;
        }
    }
    
    if (run_benchmark) {
        printf("🔬 Running benchmark with %d orders...\n", num_orders);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Add random orders
        for (int i = 0; i < num_orders; i++) {
            uint32_t price = 90 + (i % 21);
            uint32_t qty = 1 + (i % 100);
            uint8_t side = i % 2;
            book.add_order(price, qty, side, 0);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        double throughput = num_orders / (duration / 1000.0);
        printf("\n📊 Benchmark Results:\n");
        printf("   Orders processed: %d\n", num_orders);
        printf("   Time: %lld ms\n", duration);
        printf("   Throughput: %.0f orders/sec\n", throughput);
        printf("   Latency (avg): %.2f μs\n", 1000000.0 / throughput);
        
        book.print_stats();
    } else {
        // Run demo
        book.run_demo();
    }
    
    printf("\n✨ Build complete! Check out the performance metrics above.\n");
    printf("   For full benchmark: ./orderbook --benchmark --orders 100000\n\n");
    
    return 0;
}