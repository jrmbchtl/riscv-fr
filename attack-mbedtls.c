#include "polarssl/config.h"
#include "polarssl/platform.h"

#include "polarssl/error.h"
#include "polarssl/pk.h"
#include "polarssl/entropy.h"
#include "polarssl/ctr_drbg.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define SAMPLE_SIZE     10000
#define RUNS            100

typedef struct {
    uint64_t start;
    uint64_t duration;
} sample_t;

// crypto init
#define KEYFILE "keyfile.key"
#define CIPHERFILE "result-enc.txt"

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

static inline sample_t timed_call(pk_context *ctx, const unsigned char *input, size_t ilen, unsigned char *output, size_t *olen, size_t osize, int (*f_rng)(void *, unsigned char *, size_t), void *p_rng)
{
    uint64_t start, end;
    start = rdtsc();
    pk_decrypt(ctx, input, ilen, output, olen, osize,
                            f_rng, p_rng);
    end = rdtsc();
    assert(output[0] == 'H');
    assert(output[1] == 'e');
    assert(output[2] == 'l');
    assert(output[3] == 'l');
    assert(output[4] == 'o');
    return (sample_t) {start, end - start};
}

static inline sample_t timed_call_n_flush(pk_context *ctx, const unsigned char *input, size_t ilen, unsigned char *output, size_t *olen, size_t osize, int (*f_rng)(void *, unsigned char *, size_t), void *p_rng)
{
    uint64_t start, end;
    start = rdtsc();
    pk_decrypt(ctx, input, ilen, output, olen, osize,
                            f_rng, p_rng);
    end = rdtsc();
    assert(output[0] == 'H');
    assert(output[1] == 'e');
    assert(output[2] == 'l');
    assert(output[3] == 'l');
    assert(output[4] == 'o');
    flush();
    return (sample_t) {start, end - start};
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
    uint64_t chached_timings[SAMPLE_SIZE] = {0};
    uint64_t unchached_timings[SAMPLE_SIZE] = {0};
    uint64_t threshold = 0;
    pthread_t spam;

    FILE *f;
    int c;
    size_t ilen, olen = 0;
    pk_context pk;
    entropy_context entropy;
    ctr_drbg_context ctr_drbg;
    unsigned char result[1024];
    unsigned char buf[512] = {
        0x6F, 0x47, 0x3A, 0xFD, 0x23, 0x66, 0xAB, 0x6F, 0xB1, 0x83, 0xA2, 0x43, 0x53, 0x21, 0xC7, 0x6A,
        0x0D, 0xD8, 0x6B, 0x35, 0x65, 0xB4, 0x9A, 0x6C, 0x9D, 0x86, 0x8E, 0x5D, 0xA2, 0x76, 0x14, 0x1B,
        0x66, 0x67, 0x7C, 0x90, 0x5F, 0xB8, 0x0F, 0x3F, 0xA7, 0x5F, 0x9F, 0x95, 0x82, 0x2D, 0xEA, 0x45,
        0x25, 0xFD, 0x1A, 0x97, 0x65, 0xEA, 0x61, 0x33, 0xD1, 0x87, 0xC6, 0xD1, 0x96, 0x3F, 0xA9, 0x97,
        0x7C, 0xD3, 0x24, 0x60, 0x4C, 0x6A, 0x57, 0x9A, 0xF8, 0x7A, 0x50, 0x9F, 0xAE, 0x53, 0xA5, 0x38,
        0xD1, 0xF3, 0x55, 0x39, 0x9C, 0xD3, 0x5A, 0xFD, 0x05, 0x90, 0x1C, 0xE0, 0x30, 0x3E, 0x77, 0x0E,
        0x0B, 0x14, 0x0A, 0x7A, 0x15, 0x7F, 0x62, 0xBE, 0xFB, 0x49, 0x7B, 0x95, 0xE9, 0x01, 0xB0, 0x6C,
        0xC7, 0x46, 0xD5, 0x98, 0x2C, 0xC6, 0x9F, 0x4D, 0x7C, 0x72, 0xA1, 0x75, 0xD9, 0xB0, 0x8D, 0x18
    };
    const char *pers = "pk_decrypt";

    memset(result, 0, sizeof( result ) );

    entropy_init( &entropy );
    ctr_drbg_init( &ctr_drbg, entropy_func, &entropy,
                              (const unsigned char *) pers,
                               strlen( pers ) );

    pk_init( &pk );
    pk_parse_keyfile( &pk, KEYFILE, "" );
    ilen = 128;

    // get threshold for cached and uncached multiply access
    pk_decrypt( &pk, buf, ilen, result, &olen, sizeof(result),
                            ctr_drbg_random, &ctr_drbg );
    for (size_t i=0; i<SAMPLE_SIZE; i++) {
        for (size_t j=0; j<1024; j++) {
            result[j] = 0;
        }
        chached_timings[i] = timed_call(&pk, buf, ilen, result, &olen, sizeof(result),
                            ctr_drbg_random, &ctr_drbg).duration;
    }
    flush();
    for (size_t i=0; i<SAMPLE_SIZE; i++) {
        for (size_t j=0; j<1024; j++) {
            result[j] = 0;
        }
        unchached_timings[i] = timed_call_n_flush(&pk, buf, ilen, result, &olen, sizeof(result),
                            ctr_drbg_random, &ctr_drbg).duration;
    }
    printf("cached median = %lu\n", median(chached_timings, SAMPLE_SIZE));
    printf("uncached median = %lu\n", median(unchached_timings, SAMPLE_SIZE));
    threshold = (median(chached_timings, SAMPLE_SIZE) + median(unchached_timings, SAMPLE_SIZE))/2;
    printf("threshold: %lu\n", threshold);


    // printf("Observing square...\n");
    // FILE* sq = fopen("square.csv", "w");
    // for(size_t i=0; i<RUNS; i++) {
    //     size_t done = 0;
    //     pthread_create(&spam, NULL, calculate, &done);
    //     uint64_t start = rdtsc();
    //     flush();

    //     while(done == 0)
    //     {   

    //         sample_t sq_timing = timed_call_1(square);
    //         flush();
            
    //         if (sq_timing.duration < threshold_1)
    //         {
    //             fprintf(sq, "%lu\n", sq_timing.start - start);
    //         }
    //     }
    //     pthread_join(spam, NULL);
    // }
    // fclose(sq);

    return 0;
}