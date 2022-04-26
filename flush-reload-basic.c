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
}

// ---------------------------------------------------------------------------
static inline void maccess(void *p)
{
    uint64_t val;
    asm volatile("ld %0, %1\n"
                 : "=r"(val)
                 : "m"(p)
                 :);
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

void main()
{
    // No pthreads on user level riscv so we do a simple poc
    void *victim_arr[2];
    victim_arr[0] = maccess;
    victim_arr[1] = flush;

    // char victim_arr[2] = {'a', 'b'};

    uint64_t timings[6] = {0, 0, 0, 0, 0, 0};

    int ctr = 0;
    void* buf;
    // char buf;
    while (1)
    {
        /*
         Victim
         Get victim idx values into the cache
        */
        ctr = (ctr + 1) % 6;
        flush();
        buf = maccess(victim_arr[0]);
        // buf = victim_arr[0];
        asm volatile("fence" ::: "memory");
        printf("1: %lu\n", timed_load(main));
        asm volatile("fence" ::: "memory");
        printf("2: %lu\n", timed_load(main));
        asm volatile("fence" ::: "memory");
        printf("3: %lu\n", timed_load(main));
        asm volatile("fence" ::: "memory");

        /*
         Attacker
         Time both array indices and pick the one with the smaller time i.e
         the one that is in cache
        */
        // printf("%p\n", victim_arr[0]);
        // printf("%p\n", victim_arr[1]);
        asm volatile("fence" ::: "memory");
        timings[0] = timed_load(victim_arr[0]);
        asm volatile("fence" ::: "memory");
        timings[1] = timed_load(victim_arr[0]);
        asm volatile("fence" ::: "memory");
        timings[2] = timed_load(victim_arr[0]);
        asm volatile("fence" ::: "memory");
        timings[3] = timed_load(victim_arr[1]);
        asm volatile("fence" ::: "memory");
        timings[4] = timed_load(victim_arr[1]);
        asm volatile("fence" ::: "memory");
        timings[5] = timed_load(victim_arr[1]);
        asm volatile("fence" ::: "memory");
        printf("%ld %ld %ld;%ld %ld %ld: ", timings[0], timings[1], timings[2], timings[3], timings[4], timings[5]);
        if (timings[0] < timings[1]) {
            printf("0\n");
        } else {
            printf("1\n");
        }
    }
}