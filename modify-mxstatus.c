#include <stdio.h>
#include <stdint.h>

int main() {
    register int i asm("mxstatus");
    // print i has hex
    printf("%x\n", i);

    return 0;
}