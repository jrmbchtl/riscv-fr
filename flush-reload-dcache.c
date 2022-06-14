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
    asm volatile("ld %0, %1\n" : "=r"(p): "m"(p): "a5");
    asm volatile (".byte 0x0b, 0x78, 0x70, 0x03\n":::);
    printf("%p\n", p);
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
    for (int i = 0; i < SIZE; i++) {
        timings[i] = timed_load(&lookuptable[i]);
    }
    for (int i = 0; i < SIZE; i++) {
        // print i and timing[i]
        if(timings[i] > 28) printf("%d %lu\n", i, timings[i]);
    }
    flush(&lookuptable[0]);
    return 1;
}