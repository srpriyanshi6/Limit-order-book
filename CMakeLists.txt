cmake_minimum_required(VERSION 3.20)
project(OrderBook VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mtune=native -DNDEBUG -flto")
set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -fno-exceptions -fno-rtti")

# Find dependencies
find_package(benchmark REQUIRED)
find_package(Threads REQUIRED)

# Main library
add_library(orderbook STATIC
    src/order_book.cpp
    src/matching_engine.cpp
    src/memory_pool.cpp
)

target_include_directories(orderbook PUBLIC include)
target_link_libraries(orderbook Threads::Threads)

# Benchmark executable
add_executable(orderbook_bench bench/benchmark.cpp)
target_link_libraries(orderbook_bench orderbook benchmark::benchmark)

# Test executable
add_executable(orderbook_test test/test_order_book.cpp)
target_link_libraries(orderbook_test orderbook)

# Performance profiling
add_custom_target(perf
    COMMAND perf stat -e cache-misses,cache-references,cycles,instructions ./orderbook_bench
    COMMAND perf record -g ./orderbook_bench
    COMMAND perf report
)