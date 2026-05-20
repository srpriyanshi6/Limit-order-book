CXX = g++
CXXFLAGS = -std=c++20 -O3 -march=native -Wall -Wextra -pthread
TARGET = orderbook
SOURCES = src/main.cpp src/order_book.cpp
HEADERS = include/*.hpp

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -Iinclude -o $(TARGET) $(SOURCES)

benchmark: CXXFLAGS += -DBENCHMARK
benchmark: $(TARGET)

perf: $(TARGET)
	perf stat -e cache-misses,cache-references,cycles,instructions ./$(TARGET) --benchmark --orders 100000

valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET) --benchmark --orders 10000

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

demo: $(TARGET)
	./$(TARGET)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

.PHONY: all clean run demo benchmark perf valgrind install