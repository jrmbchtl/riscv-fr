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
    uint64_t val;
    asm volatile("ld %0, %1\n" : "=r"(val) : "m"(p) :);
    end = rdtsc();
    return end - start;
}

// static inline uint64_t timed_call(uint64_t (*p)(uint64_t, uint64_t))
// {
//     uint64_t start, end;
//     start = rdtsc();
//     p(0, 0);
//     end = rdtsc();
//     return end - start;
// }

uint64_t dummy_function(uint64_t x, uint64_t y)
{
    return 0;
}

// uint64_t square(uint64_t x, uint64_t y)
// {
//     return x * x;
// }

uint64_t multiply(uint64_t x, uint64_t y)
{
    return x * y;
}

void multiply_at_any_point()
{
    sleep(5);
    printf("multiply(12, 14) = %lu\n", multiply(12, 14));
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

uint64_t min(uint64_t* list, uint64_t size)
{
    uint64_t min = list[0];
    for (uint64_t i = 1; i < size; i++)
    {
        if (list[i] < min)
        {
            min = list[i];
        }
    }
    return min;
}

int main()
{
    // No pthreads on user level riscv so we do a simple poc

    // char victim_arr[1024] = {'a'};

    uint64_t timings[2] = {0, 0};
    uint64_t chached_timings[1000] = {0};
    uint64_t unchached_timings[1000] = {0};
    uint64_t threshold = 0;

    // get thresholds for cached victim_arr access
    // multiply(0, 0);
    for (int i = 0; i < 1000; i++)
    {
        // timed_call(dummy_function);
        multiply(0, 0);
        // timed_load(dummy_function);
        chached_timings[i] = timed_load(multiply);
        // printf("chached_timings[%d] = %lu\n", i, chached_timings[i]);
    }
    for (int i = 0; i < 1000; i++)
    {
        asm volatile("fence.i" ::: "memory");
        asm volatile("fence" ::: "memory");
        // timed_load(dummy_function);
        unchached_timings[i] = timed_load(multiply);
        usleep(100000;
        // printf("unchached_timings[%d] = %lu\n", i, unchached_timings[i]);
    }
    printf("cached median = %lu\n", median(chached_timings, 1000));
    printf("uncached median = %lu\n", median(unchached_timings, 1000));
    threshold = (median(chached_timings, 1000) + median(unchached_timings, 1000))/2;

    printf("threshold: %lu\n", threshold);

    pthread_t id;
    pthread_create(&id, NULL, (void*)multiply_at_any_point, NULL);

    asm volatile("fence.i" ::: "memory");
    asm volatile("fence" ::: "memory");
    usleep(10000);
    asm volatile("fence.i" ::: "memory");
    asm volatile("fence" ::: "memory");
    while(1)
    {
        // timings[1] = timed_load(dummy_function);
        timings[0] = timed_load(multiply);
        asm volatile("fence.i" ::: "memory");
        asm volatile("fence" ::: "memory");
        printf("timing of square is %lu\n", timings[0]);

        if (timings[0] < threshold)
        {
            break;
        }
        usleep(10000);
    }
    printf("someone just multiplied!\n");
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