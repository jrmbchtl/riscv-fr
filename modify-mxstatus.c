#include <stdio.h>
#include <stdint.h>

int main() {
    uint64_t val;
    asm volatile("csrrs %0, 0xC00, x0": "=r"(val)::);
    // print as hex
    printf("%lx\n", val);

    // register int i asm("a2");
    // // print i has hexEA
    // printf("%x\n", i);

    return 0;
}