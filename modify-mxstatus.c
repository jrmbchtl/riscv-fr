#include <stdio.h>
#include <stdint.h>

int main() {
    uint64_t fallback = 0;
    uint64_t val;
    asm volatile("csrrs %0, cycle, x0": "=r"(val)::);
    printf("%llu\n", val);

    // register int i asm("a2");
    // // print i has hexEA
    // printf("%x\n", i);

    return 0;
}