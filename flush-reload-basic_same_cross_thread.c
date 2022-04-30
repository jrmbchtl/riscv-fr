#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


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
static inline uint64_t timed_load(void *p)
{
    uint64_t start, end;
    start = rdtsc();
    maccess(p);
    end = rdtsc();
    return end - start;
}

static inline uint64_t timed_call(uint64_t (*p)(uint64_t, uint64_t))
{
    uint64_t start, end;
    start = rdtsc();
    p(0, 0);
    end = rdtsc();
    return end - start;
}

static inline uint64_t timed_call_n_flush(uint64_t (*p)(uint64_t, uint64_t))
{
    uint64_t start, end;
    start = rdtsc();
    p(0, 0);
    end = rdtsc();
    asm volatile("fence.i" ::: "memory");
    asm volatile("fence" ::: "memory");
    return end - start;
}

uint64_t multiply(uint64_t x, uint64_t y)
{
    return x * y;
}

void multiply_for_some_time(size_t* done)
{
    for (uint64_t i=0; i<100000000; i++)
    {
        multiply(0, 0);
    }
    printf("Done\n");
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

    // show that same-thread side channels work
    uint64_t seq[16] = {0,0,1,1,1,0,1,0,0,1,0,1,1,0,1,0};

    uint64_t timing = 0;
    uint64_t chached_timings[10000] = {0};
    uint64_t unchached_timings[10000] = {0};
    uint64_t threshold = 0;
    pthread_t id;

    // get threshold for cached and uncached multiply access
    multiply(0, 0);
    for (int i = 0; i < 10000; i++)
    {
        chached_timings[i] = timed_call(multiply);
    }
    for (int i = 0; i < 10000; i++)
    {
        flush();
        unchached_timings[i] = timed_call(multiply);
    }
    printf("cached median = %lu\n", median(chached_timings, 10000));
    printf("uncached median = %lu\n", median(unchached_timings, 10000));
    threshold = (median(chached_timings, 10000) + median(unchached_timings, 10000))/2;

    printf("threshold: %lu\n", threshold);

    asm volatile("fence.i" ::: "memory");
    asm volatile("fence" ::: "memory");

    for (int i=0; i<16; i++)
    {
        if (seq[i] == 0)
        {
            asm volatile("fence.i" ::: "memory");
            asm volatile("fence" ::: "memory");
        } else {
            multiply(0, 0);
        }
        timing = timed_call(multiply);
        if (timing < threshold) 
        {
            printf("1");
        } else {
            printf("0");
        }
    }
    printf("\n");

    size_t done = 0;

    pthread_create(&id, NULL, (void*)multiply_for_some_time, &done);

    uint64_t counter = 0;

    asm volatile("fence.i" ::: "memory");
    asm volatile("fence" ::: "memory");


    while(done == 0)
    {
        timing = timed_call(multiply);
        asm volatile("fence.i" ::: "memory");
        asm volatile("fence" ::: "memory");
        if (timing < threshold)
        {
            counter++;
        }
        // usleep(10000);
    }
    printf("counter: %lu\n", counter);

    return 0;
}