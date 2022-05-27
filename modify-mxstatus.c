
#include <stdint.h>

int main() {
    // get current value of mxstatus register with inline asm
    uint64_t mxstatus;
    asm volatile("mrs %0, mxstatus" : "=r"(mxstatus));
    mxstatus |= 0x1 << 22; // set bit 22 to 1
    // write new value to mxstatus register with inline asm
    asm volatile("msr mxstatus, %0" :: "r"(mxstatus));


    return 0;
}