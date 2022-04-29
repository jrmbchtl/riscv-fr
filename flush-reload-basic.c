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

void square_at_any_point()
{
    sleep(5);
    printf("square(12, 0) = %lu\n", square(12, 0));
    sleep(5);
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
    void *victim_arr[2];
    victim_arr[0] = square;
    victim_arr[1] = multiply;

    // char victim_arr[1024] = {'a'};

    uint64_t timings[2] = {0, 0};
    uint64_t chached_timings_0[1000] = {0};
    uint64_t chached_timings_1[1000] = {0};
    uint64_t unchached_timings_0[1000] = {0};
    uint64_t unchached_timings_1[1000] = {0};
    uint64_t threshold_tmp[4] = {0};
    uint64_t thresholds[2] = {0, 0};

    int ctr = 0;
    void* buf;

    // get thresholds for cached victim_arr access
    square(0, 0);
    for (int i = 0; i < 1000; i++)
    {
        chached_timings_0[i] = timed_call(victim_arr[0]);
    }
    multiply(0, 0);
    for (int i = 0; i < 1000; i++)
    {
        chached_timings_1[i] = timed_call(victim_arr[1]);
    }
    for (int i = 0; i < 1000; i++)
    {
        flush();
        unchached_timings_0[i] = timed_call(victim_arr[0]);
    }
    for (int i = 0; i < 1000; i++)
    {
        flush();
        unchached_timings_1[i] = timed_call(victim_arr[1]);
    }
    printf("threshold_tmp[0] = %lu\n", median(chached_timings_0, 1000));
    printf("threshold_tmp[1] = %lu\n", median(chached_timings_1, 1000));
    printf("threshold_tmp[2] = %lu\n", median(unchached_timings_0, 1000));
    printf("threshold_tmp[3] = %lu\n", median(unchached_timings_1, 1000));
    threshold_tmp[0] = median(chached_timings_0, 1000);
    threshold_tmp[1] = median(chached_timings_1, 1000);
    threshold_tmp[2] = median(unchached_timings_0, 1000);
    threshold_tmp[3] = median(unchached_timings_1, 1000);

    thresholds[0] = (threshold_tmp[0] + threshold_tmp[2]) / 2;
    thresholds[1] = (threshold_tmp[1] + threshold_tmp[3]) / 2;

    printf("thresholds: %lu %lu\n", thresholds[0], thresholds[1]);

    pthread_t id;
    pthread_create(&id, NULL, (void*)square_at_any_point, NULL);

    flush();
    while(1)
    {
        timings[0] = timed_call(victim_arr[0]);
        flush();
        printf("timing of square is %lu\n", timings[0]);
        if (timings[0] < thresholds[0])
        {
            break;
        }
        usleep(100000);
    }
    printf("someone just squared!\n");
    pthread_join(id, NULL);
    printf("exit\n");

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

    return 0;
}