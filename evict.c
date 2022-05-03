#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define THRESHOLD 100
#define TEST_CYCLES  50
#define START_SIZE 4096

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

uint64_t test_eviction_set(void* victim, struct Set *eviction_set) {
    for (int counter=0; counter<TEST_CYCLES; counter++) {
        maccess(victim);
        for (uint64_t i = 0; i < (*eviction_set).size; i++) {
            maccess((*eviction_set).list[i]);
        }
        uint64_t timing = timed_load(victim);
        if (timing < THRESHOLD) {
            return 0;
        }
    }
    return 1;
}

struct Set reduce(void* victim, struct Set *eviction_set) {
    uint64_t index = 0;
    while (index < (*eviction_set).size) {
        printf("%lu\n", index);
        assert(test_eviction_set(victim, eviction_set));
        struct Set new_set;
        new_set.size = (*eviction_set).size - 1;
        for (uint64_t i = 0; i < index; i++) {
            new_set.list[i] = (*eviction_set).list[i];
        }
        for (uint64_t i = index + 1; i < (*eviction_set).size; i++) {
            new_set.list[i-1] = (*eviction_set).list[i];
        }
        if (test_eviction_set(victim, &new_set)) {
            *eviction_set = new_set;
        } else {
            index++;
            // printf("can't remove %lu\n", index);
        }
        assert(test_eviction_set(victim, eviction_set));
    }
    return *eviction_set;

    // void* first_element = (*eviction_set).list[0];
    // uint8_t first_element_set = 0;
    // assert(test_eviction_set(victim, eviction_set));
    // void* tmp = list_pop((*eviction_set).list, (*eviction_set).size);
    // (*eviction_set).size--;
    // while(1) {
    //     if (!test_eviction_set(victim, eviction_set)) {
    //         if (!first_element_set) {
    //             first_element_set = 1;
    //             first_element = tmp;
    //         }
    //         list_append((*eviction_set).list, (*eviction_set).size, tmp);
    //         (*eviction_set).size++;
    //         assert(test_eviction_set(victim, eviction_set));
    //     } else {
    //         assert(test_eviction_set(victim, eviction_set));
    //         // printf("new size: %lu\n", eviction_set.size);
    //     }
    //     // assert test_eviction_set(first_element, eviction_set.list, eviction_set.size);
    //     assert(test_eviction_set(victim, eviction_set));
    //     tmp = list_pop((*eviction_set).list, (*eviction_set).size);
    //     if (tmp == first_element) {
    //         list_append((*eviction_set).list, (*eviction_set).size, tmp);
    //         assert(test_eviction_set(victim, eviction_set));
    //         break;
    //     }
    //     (*eviction_set).size--;
    // }
    // assert(test_eviction_set(victim, eviction_set));
}

int main() {

    // initialize eviction set
    struct Set eviction_set;
    eviction_set.size = START_SIZE;

    for (int i = 0; i < eviction_set.size; i++) {
        eviction_set.list[i] = dummy + (i+1) * 0x1000;
    }

    uint64_t cached_timings[1024] = {0};

    maccess(dummy);
    for (int i = 0; i < 1024; i++) {
        cached_timings[i] = timed_load(dummy);
    }
    uint64_t cached_timing = median(cached_timings, 1024);
    printf("Cached timing: %lu\n", cached_timing);

    if (test_eviction_set(dummy, &eviction_set)) {
        printf("Eviction set is working\n");
    } else {
        printf("Eviction set is not working... Exiting!\n");
        return 0;
    }

    eviction_set = reduce(dummy, &eviction_set);
    assert(test_eviction_set(dummy, &eviction_set));

    // make sure that eviction set is working
    if (test_eviction_set(dummy, &eviction_set)) {
        printf("Eviction set is working\n");
    } else {
        printf("Eviction set is not working... Exiting!\n");
        return 0;
    }

    // for (int i=0; i<eviction_set.size; i++) {
    //     printf("%p ", eviction_set.list[i]);
    // }
    printf("\n");

}