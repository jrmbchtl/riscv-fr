#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

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

int main() { 
    uint64_t victim = 0;

    struct timespec remaining, request = {0, 1};

    uint64_t start = rdtsc();
    // nanosleep(&request, &remaining);
    usleep(1);
    uint64_t end = rdtsc();
    printf("been sleeping for %lu cycles\n", end - start);

    uint64_t timings[8];
    maccess(&victim);
    timings[0] = timed_load(&victim);
    nanosleep(&request, &remaining);
    timings[1] = timed_load(&victim);
    nanosleep(&request, &remaining);
    timings[2] = timed_load(&victim);
    timings[3] = timed_load(&victim);
    timings[4] = timed_load(&victim);
    usleep(1);
    timings[5] = timed_load(&victim);
    usleep(1);
    timings[6] = timed_load(&victim);
    timings[7] = timed_load(&victim);

    for (int i = 0; i < 8; i++) {
        printf("timing %lu: %lu\n", i, timings[i]);
    }

    return 0;
}