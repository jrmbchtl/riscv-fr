#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define THRESHOLD 100
#define TEST_CYCLES  5
#define START_SIZE 2

struct Set {
    void* list[16384];
    uint64_t size;
} set;

static inline uint64_t rdtsc() {
  uint64_t val;
  asm volatile ("rdcycle %0\n":"=r" (val) ::);
  return val;
}

static inline void maccess(void *p) {
    uint64_t val;
    asm volatile("ld %0, %1\n" :"=r" (val) : "m"(p):);
}

static inline uint64_t timed_load(void *p){
    uint64_t start, end;
    start = rdtsc();
    maccess(p);
    end = rdtsc();
    return end-start;
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

void dummy_entry() {
    return;
}

void dummy() {
    return;
}


void* list_pop(void* list[], uint64_t size) {
    void* tmp = list[0];
    for (uint64_t i = 0; i < size - 1; i++) {
        list[i] = list[i+1];
    }
    return tmp;
}

void list_remove(void* list[], uint64_t size, uint64_t index) {
    for(int i=index+1; i<size-1; i++){
        list[i-1] = list[i];
    }
}

void* list_append(void* list[], uint64_t size, void* item) {
    list[size] = item;
    return list[size];
}

uint64_t test_eviction_set(struct Set eviction_set) {
    for (int counter=0; counter<TEST_CYCLES; counter++) {
        for (uint64_t i = 0; i < eviction_set.size; i++) {
            timed_load(eviction_set.list[i]);
        }
        
        uint64_t time;
        for (uint64_t i = 0; i < eviction_set.size; i++) {
            usleep(100);
            time = timed_load(eviction_set.list[i]);
            if (time < THRESHOLD && i == 0) {
                printf("index %lu failed with time %lu and data %p\n", i, time, eviction_set.list[i]);
                return 0;
            } else {
                // printf("index %lu passed with time %lu\n", i, time);
            }
        }
    }
    return 1;
}

struct Set reduce(struct Set eviction_set) {
    uint64_t index = eviction_set.size - 1;
    while (index > 0) { // don't allow first element to be removed
        struct Set new_set;
        new_set.size = eviction_set.size - 1;
        for (uint64_t i = 0; i < index; i++) {
            new_set.list[i] = eviction_set.list[i];
        }
        for (uint64_t i = index + 1; i < eviction_set.size; i++) {
            new_set.list[i-1] = eviction_set.list[i];
        }
        if (test_eviction_set(new_set)) {
            printf("can remove %lu\n", index);
            eviction_set = new_set;
        } else {
            printf("can't remove %lu\n", index);
        }
        index--;
    }

    assert(test_eviction_set(eviction_set));
    return eviction_set;
}

int main() {

    // initialize eviction set
    struct Set eviction_set;
    eviction_set.size = START_SIZE;
    printf("%p\n", dummy);

    for (int i = 0; i < eviction_set.size; i++) {
        eviction_set.list[i] = dummy + i * 0x1000;
    }

    uint64_t cached_timings[1024] = {0};

    maccess(eviction_set.list[0]);
    for (int i = 0; i < 1024; i++) {
        cached_timings[i] = timed_load(eviction_set.list[0]);
    }
    uint64_t cached_timing = median(cached_timings, 1024);
    printf("Cached timing: %lu\n", cached_timing);

    if (test_eviction_set(eviction_set)) {
        printf("Eviction set is working\n");
    } else {
        printf("Eviction set is not working... Exiting!\n");
        return 0;
    }

    uint64_t size = eviction_set.size;
    eviction_set = reduce(eviction_set);
    assert(test_eviction_set(eviction_set));
    

    // eviction_set = *reduce(dummy, &eviction_set);
    // assert(test_eviction_set(dummy, &eviction_set));

    // make sure that eviction set is working
    printf("%lu\n", eviction_set.size);
    for (uint64_t i = 0; i < eviction_set.size; i++) {
        printf("%p\n", eviction_set.list[i]);
    }
    if (test_eviction_set(eviction_set)) {
        printf("Eviction set is still working\n");
    } else {
        printf("Eviction set is now broken... Exiting!\n");
        return 0;
    }

    // for (int i=0; i<eviction_set.size; i++) {
    //     printf("%p ", eviction_set.list[i]);
    // }
    printf("\n");

}