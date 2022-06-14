#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SIZE     16384
#define CALIBRATE  16384
char __attribute__((aligned(4096))) data[4096 * 4];

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
    // asm volatile("ld a5, %0\n;.word 0x0277800b\n" :: "m"(p):);
    asm volatile("mv a5, %0; .word 0x0277800b\n" : : "r"(p) :"a5","memory");
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

uint64_t calibrate_offset()
{
    uint64_t timings[CALIBRATE];

    for (int i = 0; i < CALIBRATE; i++)
    {
        flush(&data[i]);
        timings[i] = timed_load(&data[i]);
    }

    for (int i = 0; i < CALIBRATE; i++)
    {
        if (timings[i] > 100) {
            printf("%d: %lu: %p\n", i, timings[i], &data[i]);
            
        }
    }
}

int main() {
    uint64_t timings[SIZE] = {0};
    void* addresses[SIZE] = {0};

    memset(data, 0, 4096 * 4);

    for (int i = 0; i < SIZE; i++) {
        addresses[i] = &data[i];
    }

    for (int i = 0; i < SIZE; i++) {
        flush(addresses[i]);
        timings[i] = timed_load(addresses[i]);
    }
    for (int i = 0; i < SIZE; i++) {
        if (timings[i] > 30) {
            printf("%d: %lu: %p\n", i, timings[i], addresses[i]);
        }
    }
    return 0;
}