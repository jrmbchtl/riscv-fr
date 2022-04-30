#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "vulnerable.h"

int main() {

    while (1) {
        multiply(0, 0);
    }

    // sleep(1);
    // multiply(1, 2);
    // printf("multiplied successfully\n");
    // sleep(1);
    // printf("Done\n");

    return 0;
}