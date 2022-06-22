#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define SIZE            16384
#define EVICT_PAGES     512
#define PRIME_RUNS      100
#define RUNS            1000
#define CACHE_LINE_SIZE 64
char __attribute__((aligned(4096))) data[SIZE];
char __attribute__((aligned(4096))) eviction_data[EVICT_PAGES * CACHE_LINE_SIZE];

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

// evict by accessing all addresses in eviction_set
// len of eviction_set has to be 4
void evict(void* eviction_set[]) {
    for (int i = 0; i < 4; i++) {
        maccess(eviction_set[i]);
    }
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

void* calculate(void* d) {
    size_t* done = (size_t*)d;
    void* address_0 = &data[0];
    void* address_1 = &data[1 * CACHE_LINE_SIZE];

    for (int i = 0; i < 10; i++) {
        usleep(1000);
        maccess(address_0);
        usleep(1000);
        maccess(address_1);
    }
    usleep(1000);

    *done = 1;
}

uint64_t get_threshold() {
    void* target = &data[0];
    uint64_t cached_timings[100];
    for (int i = 0; i < 100; i++) {
        maccess(target);
        cached_timings[i] = timed_load(target).duration;
    }
    uint64_t cached_median = median(cached_timings, 100);

    void* addresses_evict[4];
    get_eviction_set(target, addresses_evict);

    uint64_t uncached_timings[100];
    for (int i = 0; i < 100; i++) {
        evict(addresses_evict);
        uncached_timings[i] = timed_load(target).duration;
    }
    uint64_t uncached_median = median(uncached_timings, 100);
    return (cached_median + uncached_median) / 2;
}

int main() {
    // initialize data to avoid lazy allocation
    memset(data, 0, sizeof(data));
    memset(eviction_data, 0, sizeof(eviction_data));
    void* addresses_data[SIZE];
    for (size_t i=0; i<SIZE; i++) {
        addresses_data[i] = &(data[i]);
    }
    void* addresses_evict[4];
    uint64_t threshold = get_threshold();
    printf("threshold: %lu\n", threshold);
    get_eviction_set(&data[0 * CACHE_LINE_SIZE], addresses_evict);
    
    printf("Observing data[0]\n");
    pthread_t victim;
    evict(addresses_evict);
    FILE* data_0 = fopen("data_0.csv", "w");
    for (int i = 0; i < RUNS; i++) {
        size_t done = 0;
        pthread_create(&victim, NULL, calculate, &done);
        uint64_t start = rdtsc();
        evict(addresses_evict);
        
        while(!done) {
            sample_t timing = timed_load(addresses_data[0 * CACHE_LINE_SIZE]);
            
            evict(addresses_evict);

            if (timing.duration < threshold) {
                fprintf(data_0, "%lu\n", timing.start - start);
            }
        }
        pthread_join(victim, NULL);
    }
    fclose(data_0);
    printf("Observing data[0] done\n");

    get_eviction_set(&data[1 * CACHE_LINE_SIZE], addresses_evict);
    printf("Observing data[64]\n");
    evict(addresses_evict);
    FILE* data_1 = fopen("data_1.csv", "w");
    for (int i = 0; i < RUNS; i++) {
        size_t done = 0;
        pthread_create(&victim, NULL, calculate, &done);
        uint64_t start = rdtsc();
        evict(addresses_evict);
        
        while(!done) {
            sample_t timing = timed_load(addresses_data[1 * CACHE_LINE_SIZE]);
            
            evict(addresses_evict);
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