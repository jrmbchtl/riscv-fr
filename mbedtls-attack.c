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


#define KEYFILE "keyfile.key"
#define CIPHERFILE "result-enc.txt"

#define SAMPLE_SIZE     10000
#define RUNS            100

#define biL    (ciL << 3)               /* bits  in limb  */
#define ciL    (sizeof(t_uint))         /* chars in limb  */

static void mpi_montg_init( t_uint *mm, const mpi *N )
{
    t_uint x, m0 = N->p[0];
    unsigned int i;

    x  = m0;
    x += ( ( m0 + 2 ) & 4 ) << 1;

    for( i = biL; i >= 8; i /= 2 )
        x *= ( 2 - ( m0 * x ) );

    *mm = ~x + 1;
}

typedef struct {
    uint64_t start;
    uint64_t duration;
} sample_t;

static inline uint64_t rdtsc()
{
    uint64_t val;
    asm volatile("rdcycle %0\n" : "=r"(val)::);
    return val;
}

static inline void flush()
{
    asm volatile("fence.i" ::: "memory");
    asm volatile("fence" ::: "memory");
}

static inline sample_t timed_call(void (*p)(mpi*, const mpi*, const mpi*, t_uint, const mpi*), mpi *a, mpi *b, mpi *c, t_uint d, mpi *e)
{
    uint64_t start, end;
    start = rdtsc();
    p(a, b, c, d, e);
    end = rdtsc();
    return (sample_t) {start, end - start};
}


static inline sample_t timed_call_n_flush(void (*p)(mpi*, const mpi*, const mpi*, t_uint, const mpi*), mpi *a, mpi *b, mpi *c, t_uint d, mpi *e)
{
    uint64_t start, end;
    start = rdtsc();
    p(a, b, c, d, e);
    end = rdtsc();
    flush();
    return (sample_t) {start, end - start};
}

void* calculate(void* d)
{
    size_t* done = (size_t*)d;
    usleep(1000);
    FILE *f;
    int c;
    size_t i, olen = 0;
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

    /*
     * Extract the RSA encrypted value from the text file
     */
    i = 128;

    /*
     * Decrypt the encrypted RSA data and print the result.
     */

    pk_decrypt( &pk, buf, i, result, &olen, sizeof(result),
                            ctr_drbg_random, &ctr_drbg );
    printf( "The decrypted result is: '%s'\n\n", result );

    assert(result[0] == 'H');
    assert(result[1] == 'e');
    assert(result[2] == 'l');
    assert(result[3] == 'l');
    assert(result[4] == 'o');

    ctr_drbg_free( &ctr_drbg );
    entropy_free( &entropy );
    usleep(1000);
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
    uint64_t timing = 0;
    uint64_t chached_timings[SAMPLE_SIZE] = {0};
    uint64_t unchached_timings[SAMPLE_SIZE] = {0};
    uint64_t threshold = 0;
    pthread_t spam;

    mpi X, N, T;
    mpi_init(&X);
    mpi_init(&N);
    mpi_lset(&X, 2);
    mpi_lset(&N, 16);
    t_uint mm;
    mpi_montg_init( &mm, &N );
    mpi_init(&T);
    mpi_lset(&T, 2);
    int (*fun)(mpi*, const mpi*, const mpi*, const mpi*, mpi*) = mpi_exp_mod;
    void (*fun2)(mpi*, const mpi*, const mpi*, t_uint, const mpi*);
    fun2 = (void (*)(mpi*, const mpi*, const mpi*, t_uint, const mpi*)) fun - 0x1676;

    (*fun2)(&X, &X, &N, mm, &T);
    for (size_t i=0; i<SAMPLE_SIZE; i++) {
        chached_timings[i] = timed_call(fun2, &X, &X, &N, mm, &T).duration;
    }
    for (size_t i=0; i<SAMPLE_SIZE; i++) {
        unchached_timings[i] = timed_call_n_flush(fun2, &X, &X, &N, mm, &T).duration;
    }

    printf("cached median 1 = %lu\n", median(chached_timings, SAMPLE_SIZE));
    printf("uncached median 1 = %lu\n", median(unchached_timings, SAMPLE_SIZE));
    threshold = (median(chached_timings, SAMPLE_SIZE) + median(unchached_timings, SAMPLE_SIZE))/2;
    printf("threshold 1: %lu\n", threshold);

    return 0;
}