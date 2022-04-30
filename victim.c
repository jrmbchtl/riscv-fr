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
    return 0;
}