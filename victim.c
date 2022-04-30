#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "attack-me.h"

int main() {

    sleep(2);
    multiply(1, 2);
    sleep(2);
    printf("Done\n");

    return 0;
}