#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "openssl/rsa.h"

#define SAMPLE_SIZE     10000
#define RUNS            16000

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

static inline sample_t timed_call_mul(BIGNUM* a, BIGNUM* b, BIGNUM* c, BN_CTX* ctx)
{
    uint64_t start, end;
    start = rdtsc();
    BN_mul(a, b, c, ctx);
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
    BIGNUM* a = BN_new();
    BIGNUM* b = BN_new();
    BN_CTX* ctx = BN_CTX_new();
    BN_zero(a);
    BN_one(b);
    usleep(1000);
    for(int i=0; i<10; i++) {
        BN_sqr(a, b, ctx);
        usleep(1000);
    }
    // // decrypt
	// calc->plain = malloc(RSA_size(calc->rsa));
	// int len = RSA_private_decrypt(RSA_size(calc->rsa), calc->cipher, calc->plain, calc->rsa, RSA_PKCS1_PADDING);
    // assert(len > 0);
    // free(calc->plain);
	
    // // check that decrypting ciphertext is same as input
    // // assert(strcmp((char*)input, (char*)calc->plain) == 0);
    calc->done = 1;
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
    BIGNUM* c = BN_new();
    BN_CTX* ctx = BN_CTX_new();
    BN_zero(a);
    BN_one(b);
    BN_one(c);

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
    // load key from file
	FILE* f = fopen("key.pem", "r");
	if (f == NULL) {
		printf("Error opening key file \"key.pem\" for reading\n");
		return 1;
	}
	calc.rsa = RSA_new();
	PEM_read_RSAPrivateKey(f, &calc.rsa, NULL, NULL);
	fclose(f);
	
	// encrypt
	unsigned char* input = "Ciphertext";
	calc.cipher = malloc(RSA_size(calc.rsa));
	int len = RSA_public_encrypt(strlen((char*)input), input, calc.cipher, calc.rsa, RSA_PKCS1_PADDING);
	
    // calculate
    printf("Observing square...\n");
    FILE* sq = fopen("ssl_square.csv", "w");
    for(size_t i=0; i<RUNS; i++) {
        calc.done = 0;
        pthread_create(&spam, NULL, calculate, &calc);
        uint64_t start = rdtsc();
        flush();

        while(calc.done == 0)
        {   

            sample_t sq_timing = timed_call(a, b, ctx);
            flush();
            
            if (sq_timing.duration < threshold)
            {
                fprintf(sq, "%lu\n", sq_timing.start - start);
            }
        }
        pthread_join(spam, NULL);
    }
    fclose(sq);

    printf("Done observing square\n");

    printf("Observing multiply...\n");
    FILE* sq = fopen("ssl_multiply.csv", "w");
    for(size_t i=0; i<RUNS; i++) {
        calc.done = 0;
        pthread_create(&spam, NULL, calculate, &calc);
        uint64_t start = rdtsc();
        flush();

        while(calc.done == 0)
        {   

            sample_t mul_timing = timed_call_mul(a, b, c, ctx);
            flush();
            
            if (mul_timing.duration < threshold)
            {
                fprintf(sq, "%lu\n", mul_timing.start - start);
            }
        }
        pthread_join(spam, NULL);
    }
    fclose(sq);

    printf("Done observing multiply\n");
	
	// cleanup
	free(calc.cipher);
	RSA_free(calc.rsa);
    return 0;
}
