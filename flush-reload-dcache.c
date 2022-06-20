#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SIZE            16384
#define PRIME_RUNS      100
#define RUNS            10000
#define CACHE_LINE_SIZE 64
// make data span over exactly 4 pages (a 4K))
char __attribute__((aligned(4096))) data[4096 * 4];

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

sample_t timed_load(void* p) { 
    uint64_t start, end; 
    start = rdtsc(); 
    maccess(p); 
    end = rdtsc(); 
    return (sample_t) {start, end - start};
}

void* calculate(void* d) {
    size_t* done = (size_t*)d;
    void* address_0 = &data[0];
    void* address_1 = &data[1 * CACHE_LINE_SIZE];
    flush(address_0);
    flush(address_1);

    for (int i = 0; i < 10; i++) {
        usleep(1000);
        maccess(address_0);
        usleep(1000);
        maccess(address_1);
    }
    usleep(1000);

    *done = 1;
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

    maccess(addresses[0]);

    for (int i = 0; i < PRIME_RUNS; i++) {
        cached_timing[i] = timed_load(addresses[0]).duration;
    }

    for(int i = 0; i < PRIME_RUNS; i++) {
        flush(addresses[0]);
        uncached_timing[i] = timed_load(addresses[0]).duration;
    }

    uint64_t cached_median = median(cached_timing, PRIME_RUNS);
    uint64_t uncached_median = median(uncached_timing, PRIME_RUNS);
    uint64_t threshold = (cached_median + uncached_median) / 2;

    printf("cached median: %lu\n", cached_median);
    printf("uncached median: %lu\n", uncached_median);
    printf("threshold: %lu\n", threshold);

    // focus data[0]
    // victim thread
    printf("Observing data[0]\n");
    pthread_t victim;
    flush(addresses[0 * CACHE_LINE_SIZE]);
    FILE* data_0 = fopen("data_0.csv", "w");
    for (int i = 0; i < RUNS; i++) {
        size_t done = 0;
        pthread_create(&victim, NULL, calculate, &done);
        
        while(!done) {
            sample_t timing = timed_load(addresses[0 * CACHE_LINE_SIZE]);
            uint64_t start = rdtsc();
            flush(addresses[0 * CACHE_LINE_SIZE]);

            if (timing.duration < threshold) {
                fprintf(data_0, "%lu\n", timing.start - start);
            }
        }
        pthread_join(victim, NULL);
    }
    fclose(data_0);
    printf("Observing data[0] done\n");

    printf("Observing data[64]\n");
    flush(addresses[1 * CACHE_LINE_SIZE]);
    FILE* data_1 = fopen("data_1.csv", "w");
    for (int i = 0; i < RUNS; i++) {
        size_t done = 0;
        pthread_create(&victim, NULL, calculate, &done);
        
        while(!done) {
            sample_t timing = timed_load(addresses[1 * CACHE_LINE_SIZE]);
            uint64_t start = rdtsc();
            flush(addresses[1 * CACHE_LINE_SIZE]);

            if (timing.duration < threshold) {
                fprintf(data_1, "%lu\n", timing.start - start);
            }
        }
        pthread_join(victim, NULL);
    }
    fclose(data_1);
    printf("Observing data[64] done\n");

    return 0;
}