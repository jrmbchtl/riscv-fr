#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SIZE     16384
#define CALIBRATE  16384

unsigned char lookuptable[SIZE] = {0};

// funtcion equivalent to rdtsc on x86, but implemented on RISC-V
static inline uint64_t rdtsc()
{
    uint64_t val;
    asm volatile("rdcycle %0\n" : "=r"(val)::);
    return val;
}

static inline void flush(void *p) {
    uint64_t val;
    // load p into a5 and flush the dcache line with this address
    asm volatile("mv a5, %0; .word 0x0277800b\n" : : "r"(p) :"a5","memory");
}

static inline void flush_all(void** list, size_t size) {
    for (int i = 0; i < size; i++) {
        flush(list[i]);
    }
}

static inline void maccess(void *p) {
    uint64_t val;
    asm volatile("ld %0, %1\n" :"=r" (val) : "m"(p):);
}

static inline uint64_t timed_load(void *p){
    uint64_t start, end;
    start = rdtsc();
    maccess(p);
    end = rdtsc();
    return end-start;
} 

uint64_t calibrate_offset()
{
    uint64_t timings[CALIBRATE];

    for (int i = 0; i < CALIBRATE; i++)
    {
        flush(&lookuptable[i]);
        timings[i] = timed_load(&lookuptable[i]);
    }

    for (int i = 0; i < CALIBRATE; i++)
    {
        if (timings[i] > 100) {
            printf("%d: %lu: %p\n", i, timings[i], &lookuptable[i]);
            
        }
    }
}

int main() {
    uint64_t timings[SIZE] = {0};
    unsigned char *p = malloc(SIZE);
    void* addresses[SIZE] = {0};

    for (int i = 0; i < SIZE; i++)
    {
        addresses[i] = &p[i];
    }

    for (int i = 0; i < SIZE; i++) {
        flush_all(addresses, SIZE);
        timings[i] = timed_load(addresses[i]);
    }
    // open cache_misses.csv
    for (int i = 0; i < SIZE; i++) {
        if (timings[i] > 30) {
            printf("%d: %lu: %p\n", i, timings[i], addresses[i]);
        }
    }
    free(p);    
    return 0;
}