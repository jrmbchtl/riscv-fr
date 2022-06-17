#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SIZE            16384
#define SAMPLE_SIZE     10000
// make data span over exactly 4 pages (a 4KiB))
char __attribute__((aligned(4096))) data[4096 * 4];
char __attribute__((aligned(4096))) tmp[4096];

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
}

uint64_t timed_load(void* p) { 
    uint64_t start, end; 
    start = rdtsc(); 
    maccess(p); 
    end = rdtsc(); 
    return end-start; 
}

int min(int a, int b) { 
    return a < b ? a : b; 
}

int max(int a, int b) { 
    return a > b ? a : b; 
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

int main()
{
    // avoid lazy allocation
    memset(data, 0, SIZE);
    memset(tmp, 0, 4096);

    // store addresses in separate array to avoid accidental accesses
    void* addresses_data[SIZE];
    void* addresses_tmp[4096];
    for (size_t i=0; i<SIZE; i++) {
        addresses_data[i] = &data[i];
    }
    for (size_t i=0; i<SIZE; i++) {
        addresses_tmp[i] = &tmp[i];
    }

    // get threhold for cache hit/miss
    uint64_t cached_timings[SAMPLE_SIZE];
    uint64_t uncached_timings[SAMPLE_SIZE];
    
    // needed to put all necessary functions into i-cache
    timed_load(&data[SIZE-1]);
    for (size_t i=0; i<SAMPLE_SIZE; i++) {
        cached_timings[i] = timed_load(addresses_data[i % SIZE]);
    }

    // needed to put all necessary functions into i-cache
    timed_load(&data[SIZE-1]);
    // put data[0] into d-cache
    maccess(addresses_data[0]);
    // measure cache hits
    for (size_t i=0; i<SAMPLE_SIZE; i++) {
        cached_timings[i] = timed_load(addresses_data[i]);
    }

    // measure cache misses
    for (size_t i=0; i<SAMPLE_SIZE; i++) {
        for (int j = 0; j < 4096; j++)
        {
            maccess(addresses_tmp[j]);
        }
        uncached_timings[i] = timed_load(addresses_data[i]);
    }

    uint64_t cached_timing = median(cached_timings, SAMPLE_SIZE);
    uint64_t uncached_timing = median(uncached_timings, SAMPLE_SIZE);
    uint64_t threshold = (cached_timing + uncached_timing) / 2;
    printf("Uncached: %lu\n", uncached_timing);
    printf("Cached: %lu\n", cached_timing);
    printf("Threshold: %lu\n", threshold);   

    return 0;
}