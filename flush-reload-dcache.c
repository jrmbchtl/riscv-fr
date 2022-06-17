#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SIZE 16384
// make data span over exactly 4 pages (a 4K))
char __attribute__((aligned(4096))) data[4096 * 4];

// funtcion equivalent to rdtsc on x86, but implemented on RISC-V
uint64_t rdtsc() { 
    uint64_t val; 
    asm volatile("rdcycle %0\n" : "=r"(val)::); 
    val; 
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

int main()
{
    // avoid lazy allocation
    memset(data, 0, SIZE);
    void* addresses[SIZE];
    for (size_t i=0; i<SIZE; i++) {
        addresses[i] = &data[i];
    }

    // timings for cache hit/cache miss
    uint64_t timing_low, timing_high;
    // needed to put all necessary function into i-cache
    timing_low = timed_load(&data[SIZE-1]);
    // put data[0] into d-cache
    maccess(addresses[0]);
    // should be a cache hit
    timing_low = timed_load(addresses[0]);
    printf("This should be a cache hit:  %lu\n", timing_low);

    flush(addresses[0]);
    // should be a cache miss since everything was flushed
    timing_high = timed_load(addresses[0]);
    printf("This should be a cache miss: %lu\n", timing_high);
    assert(timing_high > timing_low);

    return 0;
}