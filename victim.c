#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "vulnerable.h"

int main() {

    uint64_t counter = 0;
    while (1) {
        // usleep(100);
        multiply(0, 0);
        printf("counter: %lu\n", counter);
        counter++;
    }

    // sleep(1);
    // multiply(1, 2);
    // printf("multiplied successfully\n");
    // sleep(1);
    // printf("Done\n");

    return 0;
}