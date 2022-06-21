#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SIZE            16384
#define THRESHOLD       100
char __attribute__((aligned(4096))) data[4096 * 4];
char __attribute__((aligned(4096))) eviction_data[4096 * 64];

typedef struct {
    uint64_t start;
    uint64_t duration;
} sample_t;

// funtcion equivalent to rdtsc on x86, but implemented on RISC-V
uint64_t rdtsc() { 
    uint64_t val; 
    asm volatile("rdcycle %0\n" : "=r"(val)::); 
    return val; 
}

void maccess(void* p) {
    *(volatile char*)p; 
}

sample_t timed_load(void* p) { 
    uint64_t start, end; 
    start = rdtsc(); 
    maccess(p); 
    end = rdtsc(); 
    return (sample_t) {start, end - start};
}

void* pop(void** list, size_t len, size_t index) {
    // pop item at index from list
    void* item = list[index];
    for (size_t i = index; i < len - 1; i++) {
        list[i] = list[i + 1];
    }
    list[len - 1] = NULL;
    return item;
}

void append(void** list, size_t len, void* item) {
    // append item to list
    list[len - 1] = item;
}

char eviction_test(void** list, size_t len, void* target) {
    // get reference value for cache hit
    maccess(target);
    uint64_t cached_timing = timed_load(target).duration;
    if (cached_timing > THRESHOLD) {
        // it can happen that the first check fails due to context switches, so just retry
        maccess(target);
        cached_timing = timed_load(target).duration;
        // if it happens twice in a row, something is broken
        if (cached_timing > THRESHOLD) {
            printf("no cache hit after maccess\n");
            printf("This should never happen and if it does, your're in for some trouble\n");
            return 0;
        }
    }

    // test eviction 10 times to make sure it's working and not just working at
    // random due to context switches
    for (int j = 0; j < 10; j++) {
        maccess(target);
        for (int i = 0; i < len; i++) {
            maccess(list[i]);
        }
        uint64_t uncached_timing = timed_load(target).duration;
        // if timing is lower than threshold, even just once, then
        // the eviction set isn't working
        // printf("uncached timing: %lu\n", uncached_timing);
        if (uncached_timing < THRESHOLD) {
            return 0;
        }
    }

    return 1;
}

int main() {
    // initialize data to avoid lazy allocation
    memset(data, 0, sizeof(data));
    memset(eviction_data, 0, sizeof(eviction_data));
    void* addresses_data[SIZE];
    void* addresses_evict[4096];

    for (int i = 0; i < SIZE; i++) {
        addresses_data[i] = &data[i];
    }

    for (int i = 0; i < 4096; i++) {
        addresses_evict[i] = &eviction_data[i * 64];
    }

    void* target = &data[0];
    maccess(target);
    uint64_t timing = timed_load(target).duration;

    for (int i =0; i < 4096; i++) {
        maccess(addresses_evict[i]);
    }
    uint64_t new_timing = timed_load(target).duration;
    printf("%lu\n", timing);
    printf("%lu\n", new_timing);
    assert(timing < new_timing);
    assert(new_timing > 100);
    assert(timing < 100);

    size_t len = 4096;
    size_t index = 0;
    while (index < len) {
        void* tmp = pop(addresses_evict, len, index);
        len--;
        char test = eviction_test(addresses_evict, len, target) + eviction_test(addresses_evict, len, target);
        if (test < 2) {
            len++;
            index++;
            append(addresses_evict, len, tmp);
            // printf("revert\n");
        }
        printf("new len: %lu\n", len);
        // printf("new index: %lu\n", index);
        // printf("test1: %d\n", eviction_test(addresses_evict, len, target));
        // printf("test2: %d\n", eviction_test(addresses_evict, len, target));
        maccess(target);
        assert(eviction_test(addresses_evict, len, target));
    }
    printf("new len: %lu\n", len);

    // print all addresses
    // for (int i = 0; i < len; i++) {
    //     printf("%p\n", addresses_evict[i]);
    // }

    maccess(target);
    timing = timed_load(target).duration;
    printf("%lu\n", timing);
    
    for (int i = 0; i < len; i++) {
        maccess(addresses_evict[i]);
    }
    new_timing = timed_load(target).duration;
    printf("%lu\n", new_timing);

    return 0;
}