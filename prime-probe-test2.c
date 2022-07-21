#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CACHE_LINES     512
#define CACHE_LINE_SIZE 64
#define RUNS            32768

char __attribute__((aligned(8192))) dummy_data[8192];
char __attribute__((aligned(8192))) evict_data[CACHE_LINES * CACHE_LINE_SIZE];
char __attribute__((aligned(8192))) prime_data[CACHE_LINES * CACHE_LINE_SIZE];

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

/* target is the address of the data to be evicted
 * eviction_set is a list of addresses that will be used to evict the target
 * eviction_set must be of length 4 and will be overwritten
 * array is the array to get the eviction set from and has to be of size CACHE_LINES * CACHE_LINE_SIZE and needs to be page-aligned
**/
void get_eviction_set(void* target, void* eviction_set[], char* array) {
    uint64_t base = ((uint64_t) target / 64) % 128;

    for (int i = 0; i < 4; i++) {
        eviction_set[i] = &array[(base + i * 128) * 64];
    }
}

uint64_t get_threshold() {
    uint64_t cached_timings[100];
    for (int i = 0; i < 100; i++) {
        maccess(&dummy_data[0]);
        cached_timings[i] = timed_load(&dummy_data[0]).duration;
    }
    uint64_t cached_median = median(cached_timings, 100);

    void* addresses_evict[4];
    get_eviction_set(&dummy_data[0], addresses_evict, evict_data);

    uint64_t uncached_timings[100];
    for (int i = 0; i < 100; i++) {
        evict(addresses_evict);
        uncached_timings[i] = timed_load(&dummy_data[0]).duration;
    }
    uint64_t uncached_median = median(uncached_timings, 100);
    return (cached_median + uncached_median) / 2;
}

typedef struct {
    uint8_t proc_2_ready;
    uint8_t proc_2_go;
    uint8_t proc_2_done;
} communication_t;

void* process_2(void* p) {
    communication_t* comm = (communication_t*) p;
    // needs to be between 0 and 127
    uint64_t cache_set_no = 51;

    // get eviction set for cache_set_no
    void* eviction_set[4];
    for (int i = 0; i < 4; i++) {
        flush(&dummy_data[0]);
        eviction_set[i] = &evict_data[(cache_set_no + i * 128) * 64];
    }

    // signal ready
    comm->proc_2_ready = 1;
    while (!comm->proc_2_go) {
        usleep(1);
    }

    // evict cache_set_no
    evict(eviction_set);
    comm->proc_2_done = 1;
}


int main() {
    // avoid lazy memory allocation
    memset(dummy_data, 0, 4096);
    memset(evict_data, 0, CACHE_LINES * CACHE_LINE_SIZE);
    memset(prime_data, 0, CACHE_LINES * CACHE_LINE_SIZE);
    
    uint8_t possible_cache_sets[128];
    memset(possible_cache_sets, 1, 128);

    // get threshold
    uint64_t threshold = get_threshold();
    printf("threshold: %lu\n", threshold);

    for (int k=0; k<RUNS; k++) {

        for (int j=0; j<128; j++) {

            // start process 2
            communication_t comm;
            comm.proc_2_ready = 0;
            comm.proc_2_go = 0;
            comm.proc_2_done = 0;
            pthread_t thread;
            pthread_create(&thread, NULL, process_2, &comm);

            // wait for process 2 to be ready
            while (!comm.proc_2_ready) {
                usleep(1);
            }

            // disable prefetcher
            flush(&dummy_data[0]);

            // prime cache
            for (int i = j; i < CACHE_LINES; i+=128) {
                flush(&dummy_data[0]);
                maccess(&prime_data[i * CACHE_LINE_SIZE]);
            }

            // let process 2 start
            comm.proc_2_go = 1;
            // wait for process 2 to finish
            while (!comm.proc_2_done) {
                usleep(1);
            }
            
            // probe cache
            flush(&dummy_data[0]);
            uint64_t cached_timing = 0;
            cached_timing = timed_load(&prime_data[(j+3*128) * CACHE_LINE_SIZE]).duration;

            if (cached_timing < threshold) {
                possible_cache_sets[j] = 0;
            }

            // wait for process 2 to finish
            pthread_join(thread, NULL);
        }
    }
    // print possibe cache sets
    for (int i=0; i<128; i++) {
        if (possible_cache_sets[i]) {
            printf("%d\n", i);
        }
    }
}