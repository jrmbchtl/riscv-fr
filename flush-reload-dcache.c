#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SIZE        16384
#define PRIME_RUNS  10000
// make data span over exactly 4 pages (a 4K))
char __attribute__((aligned(4096))) data[4096 * 4];

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
    uint64_t val; 
    asm volatile("ld %0, %1\n" :"=r" (val) : "m"(p):); 
    val; 
}

uint64_t timed_load(void* p) { 
    uint64_t start, end; 
    start = rdtsc(); 
    maccess(p); 
    end = rdtsc(); 
    return end-start; 
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

int main()
{
    // avoid lazy allocation
    memset(data, 0, SIZE);
    void* addresses[SIZE];
    for (size_t i=0; i<SIZE; i++) {
        addresses[i] = &(data[i]);
    }

    uint64_t cached_timing[PRIME_RUNS];
    uint64_t uncached_timing[PRIME_RUNS];

    timed_load(&data[SIZE-1]);
    // put data[0] into d-cache
    maccess(addresses[0]);

    for (int i = 0; i < PRIME_RUNS; i++) {
        cached_timing[i] = timed_load(addresses[0]);
    }

    for(int i = 0; i < PRIME_RUNS; i++) {
        flush(addresses[0]);
        uncached_timing[i] = timed_load(addresses[0]);
    }

    uint64_t cached_median = median(cached_timing, PRIME_RUNS);
    uint64_t uncached_median = median(uncached_timing, PRIME_RUNS);
    uint64_t threshold = (cached_median + uncached_median) / 2;

    printf("cached median: %lu\n", cached_median);
    printf("uncached median: %lu\n", uncached_median);
    printf("threshold: %lu\n", threshold);

    return 0;
}