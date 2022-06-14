#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SIZE     16384
#define MAX_SIZE 20
char __attribute__((aligned(4096))) data[4096 * 4];

// funtcion equivalent to rdtsc on x86, but implemented on RISC-V
static inline uint64_t rdtsc()
{
    uint64_t val;
    asm volatile("rdcycle %0\n" : "=r"(val)::);
    return val;
}

static inline void flush(void *p) {
    uint64_t val;
    // load p into a5 and flush the dcache line with this address
    // asm volatile("ld a5, %0\n;.word 0x0277800b\n" :: "m"(p):);
    asm volatile("mv a5, %0; .word 0x0277800b\n" : : "r"(p) :"a5","memory");
}

static inline void flush_offset(void *p, uint64_t offset) {
    uint64_t val;
    uint64_t p1 = (uint64_t)p % 0x800;
    uint64_t p2 = (uint64_t)p - p1 + offset;
    printf("p: %lx, p2: %lx\n", p, p2);
    // load p into a5 and flush the dcache line with this address
    // asm volatile("ld a5, %0\n;.word 0x0277800b\n" :: "m"(p):);
    asm volatile("mv a5, %0; .word 0x0277800b\n" : : "r"(p) :"a5","memory");
}

static inline void flush_all(void** list, size_t size) {
    for (int i = 0; i < size; i++) {
        flush(list[i]);
    }
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

uint64_t vote(uint64_t* list, size_t size) {
    int numbers[size];
    uint64_t options[size];
    uint64_t list_tmp[size];
    int option_count = 0;

    for (int i = 0; i < size; i++) {
        list_tmp[i] = list[i] % 2048;
    }

    for (int i = 0; i < size; i++) {
        int found = 0;
        for (int j = 0; j < option_count; j++) {
            if (options[j] == list_tmp[i]) {
                numbers[j]++;
                found = 1;
                break;
            }
        }
        if (found) {
            continue;
        }
        options[option_count] = list_tmp[i];
        numbers[option_count] = 1;
        option_count++;
    }

    int max = 0;
    int max_index = 0;
    for (int i = 0; i < option_count; i++) {
        if (numbers[i] > max) {
            max = numbers[i];
            max_index = i;
        }
    }
    return options[max_index];
}

uint64_t calibrate_offset()
{
    
    // return offset;
}

int main() {
    uint64_t timings[SIZE] = {0};
    void* addresses[SIZE] = {0};

    uint64_t relevant_addresses[MAX_SIZE] = {0};
    size_t size = 0;

    memset(data, 0, 4096 * 4);

    for (int i = 0; i < SIZE; i++) {
        addresses[i] = &data[i];
    }

    for (int i = 0; i < SIZE; i++) {
        flush(addresses[i]);
        timings[i] = timed_load(addresses[i]);
    }

    for (int i = 0; i < SIZE; i++)
    {
        if (timings[i] > 30) {
            relevant_addresses[size++] = (uint64_t) addresses[i];
        }
    }

    for (int i = 0; i < size; i++)
    {
        printf("%p\n", relevant_addresses[i]);
    }

    uint64_t offset = vote(relevant_addresses, size);
    printf("offset: %lx\n", offset);

    for (int i = 0; i < SIZE; i++)
    {
        flush_offset(addresses[i], offset);
        timings[i] = timed_load(addresses[i]);
    }

    return 0;
}