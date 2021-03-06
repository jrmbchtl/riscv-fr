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
    for (int j = 0; j < 100; j++) {
        maccess(target);
        for (int i = 0; i < len; i++) {
            if (list[i] != NULL) maccess(list[i]);
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
    void* addresses_evict[EVICT_PAGES];

    for (int i = 0; i < SIZE; i++) {
        addresses_data[i] = &data[i];
    }

    for (int i = 0; i < EVICT_PAGES; i++) {
        addresses_evict[i] = &eviction_data[i * 64];
    }

    void* target = &data[128 * 64];
    maccess(target);
    uint64_t timing = timed_load(target).duration;

    for (int i =0; i < EVICT_PAGES; i++) {
        maccess(addresses_evict[i]);
    }
    uint64_t new_timing = timed_load(target).duration;
    printf("reference cache hit:  %lu\n", timing);
    printf("reference cache miss: %lu\n", new_timing);
    assert(timing < new_timing);
    assert(new_timing > 100);
    assert(timing < 100);

    for (int k = 0; k < 128; k++) {
        target = &data[k * 64];
        for (int i = 0; i < EVICT_PAGES; i++) {
            addresses_evict[i] = &eviction_data[i * 64];
        }

        size_t len = EVICT_PAGES;
        int index = EVICT_PAGES - 1;
        while (index >= 0) {
            void* tmp = addresses_evict[index];
            addresses_evict[index] = NULL;
            char test = eviction_test(addresses_evict, len, target);
            if (!test) {
                addresses_evict[index] = tmp;
                index--;
            } else {
                for (int i = index; i < len - 1; i++) {
                    addresses_evict[i] = addresses_evict[i + 1];
                }
                addresses_evict[len - 1] = NULL;
                len--;
                index--;
            }
            if (!eviction_test(addresses_evict, len, target)) {
                printf("failed at len %lu and index %lu\n", len, index);
                return 1;
            }
        }

        // print all addresses
        for (int i = 0; i < len; i++) {
            printf("%p\n", addresses_evict[i]);
        }

        // maccess(target);
        // timing = timed_load(target).duration;
        // printf("cache hit  with eviction: %lu\n", timing);
        
        // maccess(target);
        // for (int i = 0; i < len; i++) {
        //     maccess(addresses_evict[i]);
        // }

        // new_timing = timed_load(target).duration;
        // printf("cache miss with eviction: %lu\n", new_timing);

        printf("needed indices for offset %d: ", k);
        for (int i = 0; i < len; i++) {
            for (int j = 0; j < EVICT_PAGES; j++) {
                if (addresses_evict[i] == &eviction_data[j * 64]) {
                    printf("%d ", j);
                }
            }
        }
        printf("\n");
    }

    return 0;
}