#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*

 Other processors may implement the fence.i instruction differently and flush the 
 cache. Our processor does not do this. This PoC can still be used to test for the 
 specific fence implementation, if it works the implementation is flush based.

 For deatails see:
    - Flush instruction in RISC-V Instruction set manual
    - https://elixir.bootlin.com/linux/v4.15.5/source/arch/riscv/include/asm/cacheflush.h
*/

// ---------------------------------------------------------------------------
static inline uint64_t rdtsc() {
  uint64_t val;
  asm volatile ("rdcycle %0\n":"=r" (val) ::);
  return val;
}


// ---------------------------------------------------------------------------
// On a simple risc-v implementation this might flush the cache
// On our processor this is not sufficient -> eviction sets are necessary
static inline void flush(){
    asm volatile ("fence.i" ::: "memory");
    asm volatile ("fence" ::: "memory");
}

// ---------------------------------------------------------------------------
static inline void maccess(void *p){
    uint64_t val;
    asm volatile("ld %0, %1\n" :"=r" (val) : "m"(p):);
}

// ---------------------------------------------------------------------------
static inline uint64_t timed_load(void *p){
    uint64_t start, end;
    start = rdtsc();
    maccess(p);
    end = rdtsc();
    return end-start;
} 

void main(){
    // No pthreads on user level riscv so we do a simple poc
    // void *victim_arr[2];
    // victim_arr[0] = maccess;
    // victim_arr[1] = flush;

    char victim_arr[256];
    for (int i = 0; i < 256; i++) {
        victim_arr[i] = &'a';
    }
    
    uint64_t timings[2] = {0,0};

    int ctr = 0;
    // void* buf;
    char buf;
    while(1){
        /*
         Victim
         Get victim idx values into the cache
        */
        ctr = (ctr+1)%6;
        flush();
        buf = victim_arr[255];

        /*
         Attacker
         Time both array indices and pick the one with the smaller time i.e 
         the one that is in cache
        */
        timings[0] = timed_load(victim_arr[0]);
        timings[1] = timed_load(victim_arr[255]);
        printf("%ld %ld: ",timings[0],timings[1]);
        if(timings[0] < timings[1]){
            printf("0\n");
        }else{
            printf("1\n");
        }
    }
}