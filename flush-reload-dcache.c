#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SIZE 16384
// make data span over exactly 4 pages (a 4KiB))
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

void flush_range(void** list, int n, size_t len) {
    assert(len > 8192);
    if (n < len / 2) {
        printf("%d, %d\n", n, n+4096);
        for (int i = n; i < n + 4096; i++)
        {
            flush(list[i]);
        }
    } else {
        for (int i = n - 4096; i < n; i++)
        {
            flush(list[i]);
        }
    }
}

int main()
{
    size_t index = 2048;
    // avoid lazy allocation
    memset(data, 0, SIZE);
    void* addresses[SIZE];
    for (size_t i=0; i<SIZE; i++) {
        addresses[i] = &data[i];
    }
    void* tmp;

    // timings for cache hit/cache miss
    uint64_t timing_low, timing_high;
    // needed to put all necessary functions into i-cache
    timing_low = timed_load(&data[SIZE-1]);
    // put data[0] into d-cache
    maccess(addresses[0]);
    // should be a cache hit
    timing_low = timed_load(addresses[index]);
    printf("This should be a cache hit:  %lu\n", timing_low);

    for (int i = 0; i < SIZE; i++) {
        if (i < SIZE) {
            for (int j = index; j < index + 8192; j+=64)
            {
                // flush(addresses[j]);
                tmp = addresses[j];
            }
        } else {
            for (int j = index - 4096; j < index; j++)
            {
                // flush(addresses[j]);
                tmp = addresses[j];
            }
        }
        // should be a cache miss since everything was flushed
        timing_high = timed_load(addresses[i]);
        printf("This should be a cache miss @ %d: %lu\n", i, timing_high);
        assert(timing_high > timing_low);
        assert(timing_high > 100);
    }
    printf("This should be a cache miss: %lu\n", timing_high);

    return 0;
}