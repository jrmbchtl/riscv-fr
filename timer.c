#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void main() {
    for (int i = 0; i < 100; i++) {
        uint64_t val;
        asm volatile ("rdcycle %0\n":"=r" (val) ::);
        printf("%lu\n", val);
    }
}