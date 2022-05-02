#include <pthread.h>
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

uint64_t get_over(uint64_t* list, uint64_t size, uint64_t threshold) {
    uint64_t over = 0;
    for (uint64_t i = 0; i < size; i++) {
        if (list[i] > threshold) {
            over++;
        }
    }
    return over;
}

uint64_t get_under(uint64_t* list, uint64_t size, uint64_t threshold) {
    uint64_t under = 0;
    for (uint64_t i = 0; i < size; i++) {
        if (list[i] < threshold) {
            under++;
        }
    }
    return under;
}

uint64_t get_threshold(uint64_t (*p)(uint64_t, uint64_t)) {
    uint64_t cached_timings[1024] = {0};
    uint64_t uncached_timings[1024] = {0};
    
    for (int i=0; i<1024; i++) {
        cached_timings[i] = timed_call(p);
        flush();
        uncached_timings[i] = timed_call(p);
    }

    uint64_t cached_median = median(cached_timings, 1024);
    uint64_t uncached_median = median(uncached_timings, 1024);
    uint64_t threshold = (cached_median + uncached_median) / 2;

    printf("Cached median: %lu\n", cached_median);
    printf("Uncached median: %lu\n", uncached_median);
    printf("Threshold: %lu\n", threshold);
    printf("Cached over: %lu\n", get_over(cached_timings, 1024, threshold));
    printf("Uncached under: %lu\n", get_under(uncached_timings, 1024, threshold));

    return threshold;
}

void* spam(void* args) {
    for(uint64_t i=0; i<10000000; i++) {
        multiply(0, 0);
    }
    printf("Done\n");
}

int main() {

    uint64_t threshold = get_threshold(multiply);

    pthread_t spam;
    pthread_create(&spam, NULL, spam, NULL);

    flush();
    uint64_t min = 500;
    while (1)
    {
        uint64_t timing = timed_call_n_flush(multiply);
        if (timing < threshold)
        {
            printf("Timing: %lu\n", timing);
            printf("Threshold: %lu\n", threshold);
            printf("Timing is less than threshold\n");
            printf("exiting\n");
            break;
        }
        if (timing < min)
        {
            min = timing;
            printf("Timing: %lu\n", timing);
        }
    }

    return 0;
}
