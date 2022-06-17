#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SIZE 32768
char __attribute__((aligned(4096))) data[4096 * 4];
void *max_addr = &data[SIZE - 1];
void *min_addr = &data[0];

char __attribute__((aligned(4096))) tmp[4096 * 4];

// funtcion equivalent to rdtsc on x86, but implemented on RISC-V
uint64_t rdtsc() { 
    uint64_t val; 
    asm volatile("rdcycle %0\n" : "=r"(val)::); 
    val; 
}

void flush(void* p) { 
    uint64_t val; \
    asm volatile("mv a5, %0; .word 0x0277800b\n" : : "r"(p) :"a5","memory"); \
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

void* calculate(void* d)
{
    size_t* done = (size_t*)d;

    for (size_t i=0; i<10; i++) {
        usleep(1000);
    }
    usleep(1000);
    *done = 1;
}

int main()
{
    for (size_t i=0; i<SIZE; i++) {
        data[i] = 0;
    }
    // memset(data, 0, SIZE);
    void *address = &data[64];
    uint64_t timing_low, timing_high, threshold;

    // maccess(address);
    timing_low = timed_load(&data[SIZE-1]);
    timing_low = timed_load(address);
    printf("%lu\n", timing_low);
    for (int i = 0; i<64; i++) {
        flush(&data[i]);
    }
    timing_high = timed_load(address);
    printf("%lu\n", timing_high);
    assert(timing_high > timing_low);

    threshold = (timing_high + timing_low) / 2;
    printf("threshold: %lu\n", threshold);

    return 0;
}