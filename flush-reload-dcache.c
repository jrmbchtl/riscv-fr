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
    void *address = &data[0];
    uint64_t timing, start, end;
    uint64_t tmp1, tmp2, tmp3, tmp4;

    flush(address);
    timing = timed_load(address);
    printf(("This should be high: %lu\n"), timing);

    timing = timed_load(address);
    printf(("This should be low: %lu\n"), timing);

    timing = timed_load(address);
    printf(("This should be low: %lu\n"), timing);

    flush(address);
    timing = timed_load(address);
    printf(("This should be high: %lu\n"), timing);

    timing = timed_load(address);
    printf(("This should be low: %lu\n"), timing);

    return 0;
}