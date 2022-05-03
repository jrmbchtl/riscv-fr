#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

THRESHOLD = 100;
TEST_CYCLES = 50;

static inline uint64_t rdtsc() {
  uint64_t val;
  asm volatile ("rdcycle %0\n":"=r" (val) ::);
  return val;
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

void dummy() {
    return;
}

uint64_t test_eviction_set(void* victim, void* eviction_set[], uint64_t size) {
    for (int counter=0; counter<TEST_CYCLES; counter++) {
        maccess(victim);
        for (uint64_t i = 0; i < size; i++) {
            maccess(eviction_set[i]);
        }
        uint64_t timing = timed_load(victim);
        if (timing < THRESHOLD) {
            return 0;
        }
    }
    return 1;
}

int main() {
    void* eviction_set[16384] = {0};

    for (int i = 0; i < 16384; i++) {
        eviction_set[i] = dummy + i * 0x1000;
    }

    uint64_t cached_timings[1024] = {0};

    maccess(dummy);
    for (int i = 0; i < 1024; i++) {
        cached_timings[i] = timed_load(dummy);
    }
    uint64_t cached_timing = median(cached_timings, 1024);
    printf("Cached timing: %lu\n", cached_timing);

    if (test_eviction_set(dummy, eviction_set, 16384)) {
        printf("Eviction set is working\n");
    } else {
        printf("Eviction set is not working\n");
    }

}