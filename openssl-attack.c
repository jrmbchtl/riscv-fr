#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "openssl/rsa.h"

#define SAMPLE_SIZE     10000
#define RUNS            100

typedef struct {
    uint64_t start;
    uint64_t duration;
} sample_t;

typedef struct {
    RSA* rsa;
    unsigned char* cipher;
    int cipher_len;
    unsigned char* plain;
    size_t done;
} calculate_t;


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

static inline sample_t timed_call(BIGNUM* a, BIGNUM* b, BN_CTX* ctx)
{
    uint64_t start, end;
    start = rdtsc();
    BN_sqr(a, b, ctx);
    end = rdtsc();
    return (sample_t) {start, end - start};
}

static inline sample_t timed_call_n_flush(BIGNUM* a, BIGNUM* b, BN_CTX* ctx)
{
    uint64_t start, end;
    start = rdtsc();
    BN_sqr(a, b, ctx);
    end = rdtsc();
    flush();
    return (sample_t) {start, end - start};
}

void* calculate(void* args) 
{
    calculate_t* calc = (calculate_t*) args;
    usleep(1000);
    RSA_private_decrypt(calc->cipher_len, calc->cipher, calc->plain, calc->rsa, RSA_PKCS1_PADDING);
    usleep(1000);
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

int main() {
    uint64_t timing = 0;
    uint64_t chached_timings[SAMPLE_SIZE] = {0};
    uint64_t unchached_timings[SAMPLE_SIZE] = {0};
    uint64_t threshold = 0;
    pthread_t spam;

    BIGNUM* a = BN_new();
    BIGNUM* b = BN_new();
    BN_CTX* ctx = BN_CTX_new();
    BN_zero(a);
    BN_one(b);

    BN_sqr(a, b, ctx);
    for (size_t i=0; i<SAMPLE_SIZE; i++) {
        chached_timings[i] = timed_call(a, b, ctx).duration;
    }
    flush();
    for (size_t i=0; i<SAMPLE_SIZE; i++) {
        unchached_timings[i] = timed_call_n_flush(a, b, ctx).duration;
    }

    printf("cached median = %lu\n", median(chached_timings, SAMPLE_SIZE));
    printf("uncached median = %lu\n", median(unchached_timings, SAMPLE_SIZE));
    threshold = (median(chached_timings, SAMPLE_SIZE) + median(unchached_timings, SAMPLE_SIZE))/2;
    printf("threshold: %lu\n", threshold);


    calculate_t calc;
    calc.rsa = RSA_generate_key(512, 65537, NULL, NULL);
    unsigned char* input = "Ciphertext";
	int input_len = strlen(input);
	calc.cipher_len = RSA_size(calc.rsa);
    calc.cipher = malloc(calc.cipher_len);
    int ret = RSA_public_encrypt(input_len, input, calc.cipher, calc.rsa, RSA_PKCS1_PADDING);

    calc.plain = malloc(calc.cipher_len);
    ret = RSA_private_decrypt(calc.cipher_len, calc.cipher, calc.plain, calc.rsa, RSA_PKCS1_PADDING);
    printf("Decrypted: %s\n", calc.plain);

    RSA_free(calc.rsa);
    free(calc.plain);
    free(calc.cipher);
    return 0;
}