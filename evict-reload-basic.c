#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define SIZE            16384
#define EVICT_PAGES     512
#define THRESHOLD       100
char __attribute__((aligned(4096))) data[4096 * 4];
char __attribute__((aligned(4096))) eviction_data[EVICT_PAGES * 64];

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

int main() {
    // initialize data to avoid lazy allocation
    memset(data, 0, sizeof(data));
    memset(eviction_data, 0, sizeof(eviction_data));
    void* addresses_data[SIZE];
    void* addresses_evict[4];

    for (int i = 0; i < SIZE; i++) {
        addresses_data[i] = &data[i];
    }

    for (int k = 0; k < 128; k++) {
        uint64_t target_index = k;
        void* target = addresses_data[target_index];
        // get down to cache line granularity
        uint64_t tmp = target_index / 64;
        uint64_t base = target_index % 128;
        for (int i = 0; i < 4; i ++) {
            addresses_evict[i] = &eviction_data[(base + i * 128) * 64];
        }
        maccess(target);
        uint64_t cached_timing = timed_load(target).duration;
        for (int i = 0; i < 4; i++) {
            maccess(addresses_evict[i]);
        }
        uint64_t uncached_timing = timed_load(target).duration;
        printf("cached timing: %lu\n", cached_timing);
        printf("uncached timing: %lu\n", uncached_timing);

        // for (int i = 0; i < 4; i++) {
        //     printf("%p\n", addresses_evict[i]);
        // }
    }
    

    return 0;
}