#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SIZE     16384

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
    unsigned char *p = malloc(SIZE);
    void* addresses[SIZE] = {0};

    for (int i = 0; i < SIZE; i++) {
        addresses[i] = &p[i];
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
    for (int i = 0; i < SIZE; i++) {
        flush(addresses[i]);
        timings[i] = timed_load(addresses[i]);
    }
    for (int i = 0; i < SIZE; i++) {
        if (timings[i] > 30) {
            printf("%d: %lu: %p\n", i, timings[i], addresses[i]);
        }
    }
    free(p);
    return 0;
}