#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PRIME_RUNS      1000
#define RUNS            10000
char __attribute__((aligned(4096))) data[4096];

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

void flush(void* p) {
    asm volatile("mv a5, %0; .word 0x0277800b\n" : : "r"(p) :"a5","memory");
}

void maccess(void* p) {
    *(volatile char*)p; 
}

sample_t timed_flush(void* p) { 
    uint64_t start, end; 
    start = rdtsc(); 
    flush(p);
    end = rdtsc(); 
    return (sample_t) {start, end - start};
}

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

int main() {
    // avoid lazy allocation
    memset(data, 0, 4096);

    uint64_t cached_timing[PRIME_RUNS];
    uint64_t uncached_timing[PRIME_RUNS];

    for (int i=0; i<PRIME_RUNS; i++) {
        maccess(&data[0]);
        cached_timing[i] = timed_flush(&data[0]).duration;
    }

    for (int i=0; i<PRIME_RUNS; i++) {
        uncached_timing[i] = timed_flush(&data[0]).duration;
    }

    uint64_t cached_median = median(cached_timing, PRIME_RUNS);
    uint64_t uncached_median = median(uncached_timing, PRIME_RUNS);
    uint64_t threshold = (cached_median + uncached_median) / 2;

    printf("cached median: %lu\n", cached_median);
    printf("uncached median: %lu\n", uncached_median);
    printf("threshold: %lu\n", threshold);
}