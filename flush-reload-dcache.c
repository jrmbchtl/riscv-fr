#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SIZE     32768
#define OFFSET   64
char __attribute__((aligned(4096))) data[4096 * 4];
void* max_addr = &data[SIZE-1];
void* min_addr = &data[0];

char __attribute__((aligned(4096))) tmp[4096 * 4];

// funtcion equivalent to rdtsc on x86, but implemented on RISC-V
static inline uint64_t rdtsc()
{
    uint64_t val;
    asm volatile("rdcycle %0\n" : "=r"(val)::);
    return val;
}

static inline void flush(void *p) {
    uint64_t val;
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

void shuffle_list(uint64_t* list, size_t size) {
    // randomize the list
    for (int i = 0; i < size; i++) {
        int j = rand() % size;
        uint64_t tmp = list[i];
        list[i] = list[j];
        list[j] = tmp;
    }
}

int main() {
    srand(0);
    uint64_t timings[SIZE] = {0};
    void* address = &data[0];

    char tmp = data[0];
    uint64_t timing = timed_load(address);
    printf("This should be low: %lu\n", timing);
    timing = timed_load(address);
    printf("This should be low: %lu\n", timing);
    flush(address);
    timing = timed_load(address);
    printf("This should be high: %lu\n", timing);
    flush(address);
    timing = timed_load(address);
    printf("This should be high: %lu\n", timing);
    timing = timed_load(address);
    printf("This should be low: %lu\n", timing);

    return 0;
}