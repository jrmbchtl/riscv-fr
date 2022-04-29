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
    asm volatile("rdcycle %0\n"
                 : "=r"(val)::);
    return val;
}

// ---------------------------------------------------------------------------
// On a simple risc-v implementation this might flush the cache
// On our processor this is not sufficient -> eviction sets are necessary
static inline void flush()
{
    asm volatile("fence.i" ::: "memory");
    asm volatile("fence" ::: "memory");
    // asm volatile(".word 0x0020000b" ::: "memory");
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
    end = rdtsc();
    return end - start;
}

uint64_t dummy_function(uint64_t x, uint64_t y)
{
    return 0;
}

uint64_t square(uint64_t x, uint64_t y)
{
    return x * x;
}

uint64_t multiply(uint64_t x, uint64_t y)
{
    return x * y;
}

void main()
{
    // No pthreads on user level riscv so we do a simple poc
    void *victim_arr[2];
    victim_arr[0] = square;
    victim_arr[1] = multiply;

    // char victim_arr[1024] = {'a'};

    uint64_t timings[2] = {0, 0};
    uint64_t thresholds[4] = {0};

    int ctr = 0;
    void* buf;
    // char buf;

    // get thresholds for cached victim_arr access
    square(0, 0);
    for (int i = 0; i < 1000; i++)
    {
        thresholds[0] += timed_load(victim_arr[0]);
    }
    multiply(0, 0);
    for (int i = 0; i < 1000; i++)
    {
        thresholds[1] += timed_load(victim_arr[1]);
    }
    for (int i = 0; i < 1000; i++)
    {
        flush();
        thresholds[2] += timed_call(victim_arr[0]);
    }
    for (int i = 0; i < 1000; i++)
    {
        flush();
        thresholds[3] += timed_call(victim_arr[1]);
    }
    for (int i = 0; i < 4; i++)
    {
        thresholds[i] /= 1000;
    }

    printf("thresholds: %lu %lu %lu %lu\n", thresholds[0], thresholds[1], thresholds[2], thresholds[3]);

    // while (1)
    // {
    //     /*
    //      Victim
    //      Get victim idx values into the cache
    //     */
    //     ctr = (ctr + 1) % 6;
    //     flush();
    //     timed_call(dummy_function);
    //     square(0, 0);
    //     // maccess(victim_arr[1]);
    //     // maccess(timed_load);

    //     /*
    //      Attacker
    //      Time both array indices and pick the one with the smaller time i.e
    //      the one that is in cache
    //     */
    //     // square(0, 0);
    //     timings[1] = timed_call(multiply);
    //     timings[0] = timed_call(square);
        
        
    //     printf("%ld %ld: ", timings[0], timings[1]);
    //     if (timings[0] < timings[1]) {
    //         printf("0\n");
    //     } 
    //     else {
    //         printf("1\n");
    //     }
    // }
}