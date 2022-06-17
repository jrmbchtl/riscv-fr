#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SIZE            16384
#define SAMPLE_SIZE     10000
#define RUNS            1000
#define EVICTION_SIZE   4096
// make data span over exactly 4 pages (a 4KiB))
char __attribute__((aligned(4096))) data[4096 * 4];
char __attribute__((aligned(4096))) tmp[EVICTION_SIZE];

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
    uint64_t val; 
    asm volatile("ld %0, %1\n" :"=r" (val) : "m"(p):); 
}

sample_t timed_load(void* p) { 
    uint64_t start, end; 
    start = rdtsc(); 
    maccess(p); 
    end = rdtsc(); 
    return (sample_t) {start, end-start}; 
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

void* calculate(void* d)
{
    size_t* done = (size_t*)d;
    // char tmp;

    // for (size_t i=0; i<10; i++) {
    //     usleep(1000);
    //     tmp = data[0];
    // }
    usleep(1000);
    *done = 1;
}

int main()
{
    // avoid lazy allocation
    memset(data, 0, SIZE);
    memset(tmp, 0, 4096);

    // store addresses in separate array to avoid accidental accesses
    void* addresses_data[SIZE];
    void* addresses_tmp[EVICTION_SIZE];
    for (size_t i=0; i<SIZE; i++) {
        addresses_data[i] = &data[i];
    }
    for (size_t i=0; i<EVICTION_SIZE; i++) {
        addresses_tmp[i] = &tmp[i];
    }

    // get threhold for cache hit/miss
    uint64_t cached_timings[SAMPLE_SIZE];
    uint64_t uncached_timings[SAMPLE_SIZE];

    // needed to put all necessary functions into i-cache
    timed_load(&data[SIZE-1]);
    // put data[0] into d-cache
    maccess(addresses_data[0]);
    // measure cache hits
    for (size_t i=0; i<SAMPLE_SIZE; i++) {
        cached_timings[i] = timed_load(addresses_data[i]).duration;
    }

    // measure cache misses
    for (size_t i=0; i<SAMPLE_SIZE; i++) {
        for (int j = 0; j < 4096; j++)
        {
            maccess(addresses_tmp[j]);
        }
        uncached_timings[i] = timed_load(addresses_data[i]).duration;
    }

    uint64_t cached_timing = median(cached_timings, SAMPLE_SIZE);
    uint64_t uncached_timing = median(uncached_timings, SAMPLE_SIZE);
    uint64_t threshold = (cached_timing + uncached_timing) / 2;
    printf("Uncached: %lu\n", uncached_timing);
    printf("Cached: %lu\n", cached_timing);
    printf("Threshold: %lu\n", threshold);

    // for (size_t i=0; i<SAMPLE_SIZE; i++) {
    //     printf("%lu\n", uncached_timings[i]);
    //     assert(uncached_timings[i] > threshold);
    // }

    // victim thread
    pthread_t victim;

    // size_t done1 = 0;
    // size_t done2 = 0;
    // size_t done3 = 0;
    // size_t done4 = 0;
    // size_t done5 = 0;
    // size_t done6 = 0;
    // size_t done7 = 0;
    // size_t done8 = 0;
    // size_t done9 = 0;

    // observing data[0]
    FILE* data_0 = fopen("data_0.csv", "w");
    int k = 0;
    for (size_t i = 0; i < RUNS; i++) {
        size_t done = 0;
        size_t done2 = 0;
        pthread_create(&victim, NULL, calculate, &done);
        uint64_t start = rdtsc();
        for (int j = 0; j < EVICTION_SIZE; j++) {
            maccess(addresses_tmp[j]);
        }
        while (!done) {
            sample_t timing = timed_load(addresses_data[4096]);
            for (int j = 0; j < EVICTION_SIZE; j++) {
                maccess(addresses_tmp[j]);
            }
            if (timing.duration < threshold) {
                fprintf(data_0, "%lu\n", timing.start - start);
            }
            if (k < 100000) k++;
            else done = 1;
        }
        pthread_join(victim, NULL);
    }
    fclose(data_0);

    return 0;
}