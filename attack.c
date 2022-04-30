#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "vulnerable.h"

uint64_t rdtsc() {
    uint64_t val;
    asm volatile("rdcycle %0\n" : "=r"(val)::);
    return val;
}

uint64_t timed_call(uint64_t (*p)(uint64_t, uint64_t)) {
    uint64_t start, end;
    start = rdtsc();
    p(0, 0);
    end = rdtsc();
    return end - start;
}

uint64_t timed_call_n_flush(uint64_t (*p)(uint64_t, uint64_t)) {
    uint64_t start, end;
    start = rdtsc();
    p(0, 0);
    end = rdtsc();
    asm volatile("fence.i" ::: "memory");
    asm volatile("fence" ::: "memory");
    return end - start;
}

void flush () {
    asm volatile("fence.i" ::: "memory");
    asm volatile("fence" ::: "memory");
}

int compare_uint64_t (const void * a, const void * b) {
    return *(uint64_t*)a - *(uint64_t*)b;
}

uint64_t median(uint64_t* list, uint64_t size) {
    qsort(list, size, sizeof(uint64_t), compare_uint64_t);
    return list[size/2];
}

uint64_t get_threshold() {
    uint64_t cached_timings[1024] = {0};
    uint64_t uncached_timings[1024] = {0};
    
    for (int i=0; i<1024; i++) {
        cached_timings[i] = timed_call(multiply);
        flush();
        uncached_timings[i] = timed_call(multiply);
    }

    uint64_t cached_median = median(cached_timings, 1024);
    uint64_t uncached_median = median(uncached_timings, 1024);
    uint64_t threshold = cached_median * 2;

    printf("Cached median: %lu\n", cached_median);
    printf("Uncached median: %lu\n", uncached_median);
    printf("Threshold: %lu\n", threshold);

    return threshold;
}

int main() {

    uint64_t threshold = get_threshold();
    return 0;
}
