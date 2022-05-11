#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define SAMPLE_SIZE 10000

typedef struct {
    uint64_t start;
    uint64_t duration;
} sample_t;

/*
 Other processors may implement the fence.i instruction differently and flush the
 cache. Our processor does not do this. This PoC can still be used to test for the
 specific fence implementation, if it works the implementation is flush based.

 For deatails see:
    - Flush instruction in RISC-V Instruction set manual
    - https://elixir.bootlin.com/linux/v4.15.5/source/arch/riscv/include/asm/cacheflush.h
*/

// ---------------------------------------------------------------------------
static inline uint64_t rdtsc()
{
    uint64_t val;
    asm volatile("rdcycle %0\n" : "=r"(val)::);
    return val;
}

// ---------------------------------------------------------------------------
// On a simple risc-v implementation this might flush the cache
// On our processor this is not sufficient -> eviction sets are necessary
static inline void flush()
{
    asm volatile("fence.i" ::: "memory");
    asm volatile("fence" ::: "memory");
}

// ---------------------------------------------------------------------------
static inline void maccess(void *p)
{
    uint64_t val;
    asm volatile("ld %0, %1\n" : "=r"(val) : "m"(p) :);
}

// ---------------------------------------------------------------------------
static inline sample_t timed_load(void *p)
{
    uint64_t start, end;
    start = rdtsc();
    maccess(p);
    end = rdtsc();
    return (sample_t) {start, end - start};
}

static inline sample_t timed_call_1(uint64_t (*p)(uint64_t))
{
    uint64_t start, end;
    start = rdtsc();
    p(0);
    end = rdtsc();
    return (sample_t) {start, end - start};
}

static inline sample_t timed_call_2(uint64_t (*p)(uint64_t, uint64_t))
{
    uint64_t start, end;
    start = rdtsc();
    p(0, 0);
    end = rdtsc();
    return (sample_t) {start, end - start};
}

static inline sample_t timed_call_n_flush_1(uint64_t (*p)(uint64_t))
{
    uint64_t start, end;
    start = rdtsc();
    p(0);
    end = rdtsc();
    flush();
    return (sample_t) {start, end - start};
}

static inline sample_t timed_call_n_flush_2(uint64_t (*p)(uint64_t, uint64_t))
{
    uint64_t start, end;
    start = rdtsc();
    p(0, 0);
    end = rdtsc();
    flush();
    return (sample_t) {start, end - start};
}

uint64_t multiply(uint64_t x, uint64_t y)
{
    return x * y;
}

uint64_t square(uint64_t x)
{
    return x * x;
}

void* calculate(void* d)
{
    size_t* done = (size_t*)d;

    for (size_t i=0; i<100; i++) {
        usleep(10000);
        multiply(0, 0);
        square(0);
    }
    usleep(10000);
    *done = 1;
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

int main()
{
    // No pthreads on user level riscv so we do a simple poc
    uint64_t timing = 0;
    uint64_t chached_timings_1[SAMPLE_SIZE] = {0};
    uint64_t chached_timings_2[SAMPLE_SIZE] = {0};
    uint64_t unchached_timings_1[SAMPLE_SIZE] = {0};
    uint64_t unchached_timings_2[SAMPLE_SIZE] = {0};
    uint64_t threshold_1 = 0;
    uint64_t threshold_2 = 0;
    pthread_t spam;

    // get threshold for cached and uncached multiply access
    square(0);
    for (size_t i=0; i<SAMPLE_SIZE; i++) {
        chached_timings_1[i] = timed_call_1(square).duration;
    }
    for (size_t i=0; i<SAMPLE_SIZE; i++) {
        unchached_timings_1[i] = timed_call_n_flush_1(square).duration;
    }
    printf("cached median 1 = %lu\n", median(chached_timings_1, SAMPLE_SIZE));
    printf("uncached median 1 = %lu\n", median(unchached_timings_1, SAMPLE_SIZE));
    threshold_1 = (median(chached_timings_1, SAMPLE_SIZE) + median(unchached_timings_1, SAMPLE_SIZE))/2;
    printf("threshold 1: %lu\n", threshold_1);

    multiply(0, 0);
    for (int i = 0; i < 10000; i++)
    {
        chached_timings_2[i] = timed_call_2(multiply).duration;
    }
    for (int i = 0; i < 10000; i++)
    {
        flush();
        unchached_timings_2[i] = timed_call_2(multiply).duration;
    }
    printf("cached median 2 = %lu\n", median(chached_timings_2, 10000));
    printf("uncached median 2 = %lu\n", median(unchached_timings_2, 10000));
    threshold_2 = (median(chached_timings_2, 10000) + median(unchached_timings_2, 10000))/2;
    printf("threshold 2: %lu\n", threshold_2);

    for(size_t i=0; i<100; i++) {
        size_t done = 0;
        pthread_create(&spam, NULL, calculate, &done);
        uint64_t mul_counter = 0;
        uint64_t sq_counter = 0;
        flush();

        while(done == 0)
        {
            
            sample_t mul_timing = timed_call_2(multiply);
            sample_t sq_timing = timed_call_1(square);
            flush();
            
            if (mul_timing.duration < threshold_2)
            {
                mul_counter++;
            }
            if (sq_timing.duration < threshold_1)
            {
                sq_counter++;
            }
        }
        printf("sq_counter at run %d: %lu\n", i, sq_counter);
        printf("mul_counter at run %d: %lu\n", i, mul_counter);
    }

    return 0;
}
