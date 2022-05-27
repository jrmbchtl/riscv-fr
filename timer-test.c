#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define RUNS 1000

int main()
{
    // open rdcycle.csv
    FILE *fp = fopen("rdcycle.csv", "w");
    if (fp == NULL) {
        printf("Error opening file!\n");
        return 1;
    }
    for (int i = 0; i < RUNS; i++) {
        uint64_t start, end;
        asm volatile("rdcycle %0\n" : "=r"(start)::);
        usleep(100);
        asm volatile("rdcycle %0\n" : "=r"(end)::);
        fprintf(fp, "%lu\n", end - start);
    }
    fclose(fp);

    // open rdtime.csv
    fp = fopen("rdtime.csv", "w");
    if (fp == NULL) {
        printf("Error opening file!\n");
        return 1;
    }
    for (int i = 0; i < RUNS; i++) {
        uint64_t start, end;
        asm volatile("rdtime %0\n" : "=r"(start)::);
        usleep(100);
        asm volatile("rdtime %0\n" : "=r"(end)::);
        fprintf(fp, "%lu\n", end - start);
    }
    fclose(fp);

    return 0;
}