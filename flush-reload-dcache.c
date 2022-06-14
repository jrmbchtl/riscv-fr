#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SIZE     4096

unsigned long lookuptable[SIZE] = {0};

// funtcion equivalent to rdtsc on x86, but implemented on RISC-V
static inline uint64_t rdtsc()
{
    uint64_t val;
    asm volatile("rdcycle %0\n" : "=r"(val)::);
    return val;
}

static inline void flush(void *p) {
    uint64_t val;
    // load value of pointer into into val
    // as a sideeffect, this will put the value of p into register a5
    asm volatile("ld %0, %1\n" :"=r" (val) : "m"(p):);
    // dcache.civa with a5 as input
    asm volatile (".word 0x0277800b\n":::);
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

int main() {
    uint64_t timings[SIZE] = {0};
    void* addresses[SIZE] = {0};

    for (int i = 0; i < SIZE; i++) {
        addresses[i] = &lookuptable[i];
    }

    for (int i = 0; i < SIZE; i++) {
        timings[i] = timed_load(addresses[i]);
    }
    for (int i = 0; i < SIZE; i++) {
        // print i and timing[i]
        if(timings[i] > 28) printf("%d %lu\n", i, timings[i]);
    }
    // now with flushing
    printf("Now with flushing\n");
    for (int i = 0; i < SIZE; i++) {
        flush(addresses[i]);
        timings[i] = timed_load(addresses[i]);
    }
    for (int i = 0; i < SIZE; i++) {
        // print i and timing[i]
        if(timings[i] > 28) printf("%d %lu\n", i, timings[i]);
    }
    
    return 1;
}