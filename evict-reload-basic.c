#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SIZE            16384
#define THRESHOLD       100
char __attribute__((aligned(4096))) data[4096 * 4];
char __attribute__((aligned(4096))) eviction_data[4096 * 64];

typedef struct {
    uint64_t start;
    uint64_t duration;
} sample_t;

// funtcion equivalent to rdtsc on x86, but implemented on RISC-V
uint64_t rdtsc() { 
    uint64_t val; 
    asm volatile("rdcycle %0\n" : "=r"(val)::); 
    return val; 
}

void maccess(void* p) {
    *(volatile char*)p; 
}

sample_t timed_load(void* p) { 
    uint64_t start, end; 
    start = rdtsc(); 
    maccess(p); 
    end = rdtsc(); 
    return (sample_t) {start, end - start};
}

void* pop(void** list, size_t len, size_t index) {
    // pop item at index from list
    void* item = list[index];
    for (size_t i = index; i < len - 1; i++) {
        list[i] = list[i + 1];
    }
    list[len - 1] = NULL;
    return item;
}

void append(void** list, size_t len, void* item) {
    // append item to list
    list[len - 1] = item;
}

char eviction_test(void** list, size_t len, void* target) {
    printf("2\n");
    maccess(target);
    uint64_t cached_timing = timed_load(target).duration;
    printf("3\n");
    if (cached_timing > THRESHOLD) {
        printf("no cache hit after maccess\n");
        return 0;
    }
    printf("4\n");

    for (int i = 0; i < len; i++) {
        maccess(list[i]);
    }
    printf("5\n");
    uint64_t uncached_timing = timed_load(target).duration;
    if (uncached_timing < THRESHOLD) {
        printf("cache hit after eviction\n");
        return 0;
    }
    printf("6\n");

    return 1;
}

int main() {
    memset(data, 0, sizeof(data));
    memset(eviction_data, 0, sizeof(eviction_data));
    void* addresses_data[SIZE];
    void* addresses_evict[4096];

    for (int i = 0; i < SIZE; i++) {
        addresses_data[i] = &data[i];
    }

    for (int i = 0; i < 4096; i++) {
        addresses_evict[i] = &eviction_data[i * 64];
    }

    void* target = &data[0];
    maccess(target);
    uint64_t timing = timed_load(target).duration;

    for (int i =0; i < 4096; i++) {
        maccess(addresses_evict[i]);
    }
    uint64_t new_timing = timed_load(target).duration;
    printf("%lu\n", timing);
    printf("%lu\n", new_timing);
    assert(timing < new_timing);
    assert(new_timing > 100);
    assert(timing < 100);

    size_t len = 4096;
    size_t index = 0;
    while (index < len) {
        void* tmp = pop(addresses_evict, len, index);
        len--;
        char test = eviction_test(addresses_evict, len, target);
        if (!test) {
            append(addresses_evict, len, tmp);
            len++;
            index++;
        }
        printf("1\n");
        printf("test is working: %d\n", eviction_test(addresses_evict, len, target));
        printf("new len: %lu\n", len);
        printf("current index: %lu\n", index);
    }

    return 0;
}