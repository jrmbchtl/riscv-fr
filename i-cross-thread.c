#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

typedef struct {
    uint64_t start;
    uint64_t done;
} thread_args;

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


void dummy() {
    return;
}

void* thread_2(void* t) {
    thread_args* args = (thread_args*)t;
    // open thread2.csv
    FILE* fp = fopen("thread2.csv", "w");
    uint64_t before, after;
    printf("Thread 2 started\n");
    args->start = 1;
    printf("value of done: %d\n", args->done);
    while (!args->done) {
        before = rdtsc();
        // dummy();
        // after = rdtsc();
        fprintf(fp, "%lu\n", before);
    }
    fprintf(fp, "\n");
    fclose(fp);
}

int main() {
    pthread_t spam;

    thread_args args;
    args.start = 0;
    args.done = 0;

    printf("Main started\n");
    pthread_create(&spam, NULL, thread_2, &args);
    while(!args.start) {
    }

    uint64_t before, middle, after;
    FILE* fp = fopen("thread1.csv", "w");

    flush();
    for (int i = 0; i < 100000; i++) {
        before = rdtsc();
        // dummy();
        // middle = rdtsc();
        // // flush();
        // after = rdtsc();
        fprintf(fp, "%lu\n", before);
    }
    fclose(fp);
    args.done = 1;
    printf("Waiting for thread 2 to finish\n");
    pthread_join(spam, NULL);


    return 0;
}