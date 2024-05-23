#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "ultracdc.h"

void print_chunk(void *arg, size_t offset, size_t len)
{
    printf("%08zu|%08zu\n", offset, len);
}

int main(int argc, char *argv[])
{
    FILE *s = fopen(argv[1], "r");
    if (!s)
        exit(-1);

    struct timespec tp1, tp2;

    clock_gettime(CLOCK_MONOTONIC, &tp1);
    printf("%8s|%8s\n", "offset", "length");
    size_t bytes = ultracdc_stream(s, 2048, 0, 65536, print_chunk, NULL);
    clock_gettime(CLOCK_MONOTONIC, &tp2);
    double elapsed = (tp2.tv_sec - tp1.tv_sec) * 1000 + (tp2.tv_nsec - tp1.tv_nsec) / 1000000.0;
    fprintf(stderr, "\n======\n%.2fmb in %.2fms (%.3f mbps)\n", 1.0 * bytes / (1024 * 1024), elapsed,
           1000.0 * bytes / (elapsed * 1024 * 1024));
    fclose(s);
    return 0;
}
