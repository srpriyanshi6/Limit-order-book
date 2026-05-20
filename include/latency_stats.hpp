#pragma once
#include <vector>
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdint>

class LatencyStats {
private:
    static constexpr size_t MAX_SAMPLES = 1000000;
    alignas(64) std::vector<uint64_t> latencies;
    std::atomic<size_t> write_index{0};
    std::atomic<uint64_t> total_latency{0};
    std::atomic<uint64_t> min_latency{UINT64_MAX};
    std::atomic<uint64_t> max_latency{0};
    std::atomic<uint64_t> sum_squares{0};
    
public:
    LatencyStats() : latencies(MAX_SAMPLES) {}
    
    void record(uint64_t latency_ns) {
        size_t idx = write_index.fetch_add(1, std::memory_order_relaxed);
        if (idx < MAX_SAMPLES) {
            latencies[idx] = latency_ns;
            total_latency += latency_ns;
            sum_squares += latency_ns * latency_ns;
            
            uint64_t current_min = min_latency.load();
            while (latency_ns < current_min && 
                   !min_latency.compare_exchange_weak(current_min, latency_ns));
            
            uint64_t current_max = max_latency.load();
            while (latency_ns > current_max && 
                   !max_latency.compare_exchange_weak(current_max, latency_ns));
        }
    }
    
    double p50() {
        size_t count = write_index.load();
        if (count == 0) return 0;
        auto copy = latencies;
        std::nth_element(copy.begin(), copy.begin() + count / 2, 
                        copy.begin() + count);
        return copy[count / 2] / 1000.0;
    }
    
    double p99() {
        size_t count = write_index.load();
        if (count == 0) return 0;
        auto copy = latencies;
        size_t idx = static_cast<size_t>(count * 0.99);
        std::nth_element(copy.begin(), copy.begin() + idx, 
                        copy.begin() + count);
        return copy[idx] / 1000.0;
    }
    
    double mean() {
        size_t count = write_index.load();
        if (count == 0) return 0;
        return total_latency.load() / static_cast<double>(count) / 1000.0;
    }
    
    double stddev() {
        size_t count = write_index.load();
        if (count == 0) return 0;
        double mean_val = mean() * 1000.0;
        double mean_sq = sum_squares.load() / static_cast<double>(count);
        return std::sqrt(mean_sq - mean_val * mean_val) / 1000.0;
    }
    
    void print_stats(const char* name = "Operation") {
        printf("\n=== %s Latency Statistics ===\n", name);
        printf("  P50:  %8.2f μs\n", p50());
        printf("  P99:  %8.2f μs\n", p99());
        printf("  Mean: %8.2f μs\n", mean());
        printf("  Std:  %8.2f μs\n", stddev());
        printf("  Min:  %8.2f μs\n", min_latency.load() / 1000.0);
        printf("  Max:  %8.2f μs\n", max_latency.load() / 1000.0);
        printf("  Samples: %zu\n", write_index.load());
    }
};