#!/bin/bash

echo "=========================================="
echo "  Building Order Book from Scratch"
echo "=========================================="

# Clean previous build
make clean

# Build the project
echo "Compiling C++20 code..."
make

if [ $? -eq 0 ]; then
    echo " Build successful!"
    
    echo ""
    echo "=========================================="
    echo "  Running Interactive Demo"
    echo "=========================================="
    ./orderbook
    
    echo ""
    echo "=========================================="
    echo "  Running Performance Benchmark"
    echo "=========================================="
    ./orderbook --benchmark --orders 50000
    
    echo ""
    echo "=========================================="
    echo "  Cache Performance Analysis"
    echo "=========================================="
    make perf 2>/dev/null || echo "perf not available (run with sudo)"
    
else
    echo "Build failed!"
    exit 1
fi