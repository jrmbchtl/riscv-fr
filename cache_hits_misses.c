#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define RUNS            1000
char __attribute__((aligned(4096))) data[4096];

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

// flush p from dcache
void flush(void* p) {
    asm volatile("mv a5, %0; .word 0x0277800b\n" : : "r"(p) :"a5","memory");
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

void maccess(void* p) {
    *(volatile char*)p; 
}

uint64_t timed_load(void* p) { 
    uint64_t start, end; 
    start = rdtsc(); 
    maccess(p); 
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
    FILE* ichm = fopen("icache_hits_misses.csv", "w");
    uint64_t timing = 0;
    for (int i = 0; i < RUNS; i++) {
        square(0);
        timing =  timed_call(square);
        fprintf(ichm, "%lu\n", timing);
    }

    for (int i = 0; i < RUNS; i++) {
        clflush();
        timing =  timed_call(clflush);
        fprintf(ichm, "%lu\n", timing);
    }
    fclose(ichm);

    FILE* dchm = fopen("dcache_hits_misses.csv", "w");
    for (int i = 0; i < RUNS; i++) {
        maccess(&data[0]);
        timing =  timed_load(&data[0]);
        fprintf(dchm, "%lu\n", timing);
    }

    for (int i = 0; i < RUNS; i++) {
        flush(&data[0]);
        timing =  timed_load(&data[0]);
        fprintf(dchm, "%lu\n", timing);
    }
    fclose(dchm);


    return 0;
}