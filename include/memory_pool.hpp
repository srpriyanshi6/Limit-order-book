#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <new>

template<typename T, size_t PoolSize = 1000000>
class MemoryPool {
private:
    alignas(64) std::array<T, PoolSize> pool;
    alignas(64) std::atomic<size_t> next_index{0};
    alignas(64) std::array<size_t, PoolSize> free_list;
    std::atomic<size_t> free_count{0};
    std::atomic<bool> initialized{false};
    
    void init_free_list() {
        if (initialized.exchange(true)) return;
        for (size_t i = 0; i < PoolSize - 1; ++i) {
            free_list[i] = i + 1;
        }
        free_list[PoolSize - 1] = SIZE_MAX;
    }
    
public:
    MemoryPool() {
        init_free_list();
    }
    
    T* acquire() {
        size_t idx = free_count.fetch_add(1, std::memory_order_acquire);
        if (idx >= PoolSize) return nullptr;
        
        size_t current = idx;
        size_t next = free_list[current];
        
        while (next != SIZE_MAX && 
               !free_list.compare_exchange_weak(current, next, 
                std::memory_order_acq_rel)) {
            current = next;
            next = free_list[current];
        }
        
        T* obj = &pool[next];
        new (obj) T();
        return obj;
    }
    
    void release(T* obj) {
        if (!obj) return;
        obj->~T();
        size_t idx = obj - &pool[0];
        size_t old_head = free_count.load(std::memory_order_acquire);
        do {
            free_list[idx] = old_head;
        } while (!free_count.compare_exchange_weak(old_head, idx, 
                 std::memory_order_release));
    }
    
    size_t capacity() const { return PoolSize; }
};