#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SIZE            16384
char __attribute__((aligned(4096))) data[4096 * 4];
char __attribute__((aligned(4096))) evict[4096 * 64];

typedef struct {
    uint64_t start;
    uint64_t duration;
} sample_t;

// funtcion equivalent to rdtsc on x86, but implemented on RISC-V
uint64_t rdtsc() { 
    uint64_t val; 
    asm volatile("rdcycle %0\n" : "=r"(val)::); 
    return val; 
}

void maccess(void* p) {
    *(volatile char*)p; 
}

sample_t timed_load(void* p) { 
    uint64_t start, end; 
    start = rdtsc(); 
    maccess(p); 
    end = rdtsc(); 
    return (sample_t) {start, end - start};
}

int main() {
    memset(data, 0, sizeof(data));
    memset(evict, 0, sizeof(evict));
    void* addresses_data[SIZE];
    void* addresses_evict[4096];

    for (int i = 0; i < SIZE; i++) {
        addresses_data[i] = &data[i];
    }

    for (int i = 0; i < 4096; i++) {
        addresses_evict[i] = &evict[i * 64];
    }

    void* target = &data[0];
    maccess(target);
    uint64_t timing = timed_load(target).duration;

    for (int i =0; i < 4096; i++) {
        maccess(addresses_evict[i]);
    }
    uint64_t new_timing = timed_load(target).duration;
    printf("%lu\n", timing);
    printf("%lu\n", new_timing);

    return 0;
}