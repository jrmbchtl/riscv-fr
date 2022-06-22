#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define SIZE            16384
#define EVICT_PAGES     512
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

// compare function for qsort
int compare_uint64_t (const void * a, const void * b) 
{
   return ( *(int*)a - *(int*)b );
}

// returns the median of a list given a list and its length
// works by sorting the list and returning the middle element
uint64_t median(uint64_t* list, uint64_t size)
{
    uint64_t* sorted = malloc(size * sizeof(uint64_t));
    memcpy(sorted, list, size * sizeof(uint64_t));
    qsort(sorted, size, sizeof(uint64_t), compare_uint64_t);
    uint64_t median = sorted[size / 2];
    free(sorted);
    return median;
}

// target is the address of the data to be evicted
// eviction_set is a list of addresses that will be used to evict the target
// eviction_set must be of length 4 and will be overwritten
void get_eviction_set(void* target, void* eviction_set[]) {
    uint64_t base;
    // since beginning of page can be offset, this needs to be asjusted
    if (((uint64_t) &eviction_data[0] / 64) % 128 == 0) {
        base = ((uint64_t) target / 64) % 128;
    } else {
        base = (((uint64_t) target / 64) + 64) % 128;
    }

    for (int i = 0; i < 4; i++) {
        eviction_set[i] = &eviction_data[(base + i * 128) * 64];
    }
}

uint64_t get_threshold() {
    void* target = &data[0];
    uint64_t cached_timings[100];
    for (int i = 0; i < 100; i++) {
        maccess(target);
        cached_timings[i] = timed_load(target).duration;
    }
    uint64_t cached_median = median(cached_timings, 100);

    // uint64_t base;
    // // since beginning of page can be offset, this needs to be asjusted
    // if (((uint64_t) target / 64) % 128 == 0) {
    //     base = ((uint64_t) target / 64) % 128;
    // } else {
    //     base = (((uint64_t) target / 64) + 64) % 128;
    // }
    void* addresses_evict[4];
    get_eviction_set(target, addresses_evict);
    // for (int i = 0; i < 4; i++) {
    //     addresses_evict[i] = &eviction_data[(base + i * 128) * 64];
    // }
    uint64_t uncached_timings[100];
    for (int i = 0; i < 100; i++) {
        for (int j = 0; j < 4; j++) {
            maccess(addresses_evict[j]);
        }
        uncached_timings[i] = timed_load(target).duration;
    }
    uint64_t uncached_median = median(uncached_timings, 100);
    printf("cached median: %lu\n", cached_median);
    printf("uncached median: %lu\n", uncached_median);
    return (cached_median + uncached_median) / 2;
}

int main() {
    // initialize data to avoid lazy allocation
    memset(data, 0, sizeof(data));
    memset(eviction_data, 0, sizeof(eviction_data));
    void* addresses_data[SIZE];
    void* addresses_evict[4];
    uint64_t threshold = get_threshold();
    

    // for (int i = 0; i < SIZE; i++) {
    //     addresses_data[i] = &data[i];
    // }

    // for (int k = 0; k < 128; k++) {
    //     // step with cachline size
    //     void* target = addresses_data[k * 64];
    //     // uint64_t target_index = k * 64;
    //     // void* target = addresses_data[target_index];
    //     // get down to cache line granularity
    //     uint64_t tmp = ((uint64_t) target) / 64;
    //     uint64_t base = tmp % 128;
    //     printf("target: %p, base: %lu, tmp: %lu\n", target, base, tmp);
    //     for (int i = 0; i < 4; i ++) {
    //         // printf("%d\n", (base + i * 128));
    //         addresses_evict[i] = &eviction_data[(base + i * 128) * 64];
    //     }
    //     maccess(target);
    //     uint64_t cached_timing = timed_load(target).duration;
    //     for (int i = 0; i < 4; i++) {
    //         maccess(addresses_evict[i]);
    //     }
    //     uint64_t uncached_timing = timed_load(target).duration;
    //     printf("cached timing: %lu\n", cached_timing);
    //     printf("uncached timing: %lu\n", uncached_timing);

    //     for (int i = 0; i < 4; i++) {
    //         printf("%p\n", addresses_evict[i]);
    //     }
    // }
    

    return 0;
}