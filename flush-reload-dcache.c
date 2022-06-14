#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SIZE     16384

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
    // load value of p into register a5
    asm volatile("ld a5, %0\n;.word 0x0277800b\n" :: "m"(p):);
    printf("1: %p\n", p);
    // dcache.civa with a5 as input
    // asm volatile (".word 0x0277800b\n":::);
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

int main() {
    uint64_t timings[SIZE] = {0};
    void* addresses[SIZE] = {0};

    for (int i = 0; i < SIZE; i++) {
        addresses[i] = &lookuptable[i];
    }

    for (int i = 0; i < SIZE; i++) {
        flush(addresses[i]);
    }

    for (int i = 0; i < SIZE; i++) {
        timings[i] = timed_load(addresses[i]);
    }
    // open cache_hits.csv
    FILE *fp = fopen("cache_hits.csv", "w");
    for (int i = 0; i < SIZE; i++) {
        fprintf(fp, "%lu\n", timings[i]);
    }
    fclose(fp);
    // now with flushing
    printf("Now with flushing\n");
    for (int i = 0; i < SIZE; i++) {
        // flush(addresses[i]);
        flush_all(addresses, SIZE);
        timings[i] = timed_load(addresses[i]);
        // printf("2: %p\n", addresses[i]);
        // printf("3: %p\n", &lookuptable[i]);
    }
    // open cache_misses.csv
    fp = fopen("cache_misses.csv", "w");
    for (int i = 0; i < SIZE; i++) {
        fprintf(fp, "%lu\n", timings[i]);
    }
    fclose(fp);
    // printf("Done\n");
    // flush(1);
    
    return 1;
}