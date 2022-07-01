#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define RUNS            1000
// funtcion equivalent to rdtsc on x86, but implemented on RISC-V
static inline uint64_t rdtsc()
{
    uint64_t val;
    asm volatile("rdcycle %0\n" : "=r"(val)::);
    return val;
}

// function to flush the I-cache
static inline void clflush()
{
    asm volatile("fence.i" ::: "memory");
    asm volatile("fence" ::: "memory");
}

// measure the time it takes to execute function p(0) and return start and duration
static inline uint64_t timed_call(uint64_t (*p)(uint64_t))
{
    uint64_t start, end;
    start = rdtsc();
    p(0);
    end = rdtsc();
    return end - start;
}

// simple function squaring a number
uint64_t square(uint64_t x)
{
    return x * x;
}

int main()
{
    FILE* chm = fopen("cache_hits_misses.csv", "w");
    uint64_t timing = 0;
    for (int i = 0; i < RUNS; i++) {
        square(0);
        timing =  timed_call(square);
        fprintf(chm, "%lu\n", timing);
    }

    for (int i = 0; i < RUNS; i++) {
        clflush();
        timing =  timed_call(clflush);
        fprintf(chm, "%lu\n", timing);
    }
    fclose(chm);

    return 0;
}