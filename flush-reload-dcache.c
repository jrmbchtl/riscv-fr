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
    asm volatile("mv a5, %0; .word 0x0277800b;fence\n"
                 :
                 : "r"(p)
                 : "a5", "memory");
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
    start = rdtsc();
    maccess(p);
    end = rdtsc();
    return end - start;
}

// compare function for qsort
int compare_uint64_t (const void * a, const void * b) 
{
   return ( *(int*)a - *(int*)b );
}

uint64_t median(uint64_t* list, uint64_t size)
{
    uint64_t* sorted = malloc(size * sizeof(uint64_t));
    memcpy(sorted, list, size * sizeof(uint64_t));
    qsort(sorted, size, sizeof(uint64_t), compare_uint64_t);
    uint64_t median = sorted[size / 2];
    free(sorted);
    return median;
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
        data[i] = i;
    }
    void *address = &data[0];
    uint64_t timings[3] = {0};

    timed_load(address);
    for (int i = 0; i < 3; i++) {
        timings[i] = timed_load(address);
        printf("This should be low: %lu\n", timings[i]);
    }
    for (int i = 0; i < 3; i++) {
        flush(address);
        timings[i] = timed_load(address);
        printf("This should be high: %lu\n", timings[i]);
    }
    return 0;
}