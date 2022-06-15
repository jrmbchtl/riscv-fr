#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SIZE     16384
#define OFFSET   2048
char __attribute__((aligned(4096))) data[4096 * 4];
void* max_addr = &data[SIZE-1];
void* min_addr = &data[0];

uint64_t access_pattern[SIZE] = {0};

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

void evict() {
    for (int i = 0; i < SIZE; i+=64) {
        if (addresses[i] % OFFSET == -1) {
            printf("You never should have come here\n");
            break;
        }
    }
}

int main() {
    srand(0);
    uint64_t timings[SIZE] = {0};
    void* addresses[SIZE] = {0};

    for (uint64_t i = 0; i < SIZE; i++) {
        access_pattern[i] = i;
    }

    shuffle_list(access_pattern, SIZE);

    memset(data, 0, 4096 * 4);

    for (int i = 0; i < SIZE; i++) {
        addresses[i] = &data[i];
    }

    for (int i = 0; i < SIZE; i+=1) {
        // if (((uint64_t) addresses[access_pattern[i]]) % OFFSET != 0) {
        //     continue;
        // }
        evict();
        // flush(addresses[access_pattern[i]]);
        timings[access_pattern[i]] = timed_load(addresses[access_pattern[i]]);
    }

    int counter = 0;
    for (int i = 0; i < SIZE; i++)
    {
        if (timings[i] > 30) {
            counter++;
            printf("%d, %lu, %p\n", i, timings[i], addresses[i]);
        }
    }
    printf("Cache misses: %d\n", counter);

    return 0;
}