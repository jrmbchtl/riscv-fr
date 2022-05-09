#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

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

void* thread_2(void* d) {
    int *done = (int*)d;
    // open thread2.csv
    FILE* fp = fopen("thread2.csv", "w");
    uint64_t before, after;
    printf("Thread 2 started\n");
    printf("value of done: %d\n", *done);
    while (!*done) {
        before = rdtsc();
        dummy();
        after = rdtsc();
        fprintf(fp, "%lu,%lu\n", before, after);
    }
    fclose(fp);    
}

int main() {
    int done = 0;
    pthread_t spam;
    printf("Main started\n");
    pthread_create(&spam, NULL, thread_2, &done);
    printf("value of done in main: %d\n", done);
    usleep(100000);

    uint64_t before, middle, after;
    FILE* fp = fopen("thread1.csv", "w");

    flush();
    for (int i = 0; i < 100; i++) {
        before = rdtsc();
        dummy();
        middle = rdtsc();
        flush();
        after = rdtsc();
        fprintf(fp, "%lu,%lu,%lu\n", before, middle, after);
    }
    fclose(fp);
    done = 1;
    printf("Waiting for thread 2 to finish\n");
    pthread_join(spam, NULL);


    return 0;
}