#include <stdio.h>
#include <stdint.h>

int main() {
    register int i asm("0x7C0");
    // print i has hexEA
    printf("%x\n", i);

    return 0;
}