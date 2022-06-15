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
static inline uint64_t rdtsc()
{
    uint64_t val;
    asm volatile("rdcycle %0\n"
                 : "=r"(val)::);
    return val;
}

static inline void flush(void *p)
{
    uint64_t val;
    asm volatile("fence");
    asm volatile("mv a5, %0; .word 0x0277800b\n"
                 :
                 : "r"(p)
                 : "a5", "memory");
    asm volatile("fence");
}

static inline void maccess(void *p)
{
    uint64_t val;
    asm volatile("ld %0, %1\n"
                 : "=r"(val)
                 : "m"(p)
                 :);
}

static inline uint64_t timed_load(void *p)
{
    uint64_t start, end;
    asm volatile("fence");
    start = rdtsc();
    asm volatile("fence");
    maccess(p);
    asm volatile("fence");
    end = rdtsc();
    asm volatile("fence");
    return end - start;
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
    memset(data, 0, SIZE);
    void *address = &data[0];
    uint64_t timings[3] = {0};

    timed_load(address);
    uint64_t timing = timed_load(address);
    printf("This should be low: %lu\n", timing);
    timing = timed_load(address);
    printf("This should be low: %lu\n", timing);
    
    flush(address);
    timing = timed_load(address);
    printf("This should be high: %lu\n", timing);

    timing = timed_load(address);
    printf("This should be low: %lu\n", timing);

    return 0;
}