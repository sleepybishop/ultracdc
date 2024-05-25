#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define XXH_STATIC_LINKING_ONLY
#define XXH_IMPLEMENTATION

#include "xxhash.h"
#include "mmapring.h"

#include "ultracdc.h"

typedef struct {
    chunker_cfg *self;
    size_t (*cut)(chunker_cfg *, uint8_t *data, size_t len);
    void (*on_chunk)(uint8_t *data, size_t offset, size_t len);
} chunker;

void print_chunk(uint8_t *data, size_t offset, size_t len)
{
    static int hdr_set;
    if (!hdr_set) {
        printf("%16s|%8s|%8s\n", "chunk_id", "offset", "length");
        hdr_set = 1;
    }
    printf("%016lx|%08zu|%08zu\n", XXH3_64bits(data, len), offset, len);
}

int main(int argc, char *argv[])
{
    FILE *s = NULL;

    if (argc == 2) {
        if (memcmp(argv[1], "-", 1) == 0)
            s = stdin;
        else
            s = fopen(argv[1], "r");
    }
    if (!s)
        exit(-1);

    struct timespec tp1, tp2;
    const size_t min_chunk = 2048, max_chunk = 65536, read_chunk = 16384;
    mmapring_t rb = mmapring_create("ring.tmp", 2 * max_chunk);

    if (rb.err) {
        fprintf(stderr, "ring error: %s\n", rb.err);
        exit(1);
    }

    chunker_cfg ckrcfg = ultracdc_init(min_chunk, 0, max_chunk);
    chunker ckr = {.self = &ckrcfg, .cut = ultracdc_cut, .on_chunk = print_chunk};

    size_t offset = 0;
    double elapsed = 0.0;
    uint8_t data[read_chunk];
    while (!feof(s)) {
        size_t ar = fread(data, 1, sizeof(data), s);
        mmapring_append(&rb, data, ar);
        while ((rb.written - offset) > ckr.self->ma) {
            uint8_t *ptr = rb.base + (offset % rb.size);
            clock_gettime(CLOCK_MONOTONIC, &tp1);
            size_t cp = ckr.cut(ckr.self, ptr, rb.written - offset);
            clock_gettime(CLOCK_MONOTONIC, &tp2);
            elapsed += (tp2.tv_sec - tp1.tv_sec) * 1000 + (tp2.tv_nsec - tp1.tv_nsec) / 1000000.0;
            ckr.on_chunk(ptr, offset, cp);
            offset += cp;
        }
    }
    while (offset < rb.written) {
        uint8_t *ptr = rb.base + (offset % rb.size);
        size_t cp = ckr.cut(ckr.self, ptr, rb.written - offset);
        ckr.on_chunk(ptr, offset, cp);
        offset += cp;
    }

    fprintf(stderr, "\n======\n%.2fmb in %.2fms (%.3f mbps)\n", 1.0 * offset / (1024 * 1024), elapsed,
            1000.0 * offset / (elapsed * 1024 * 1024));

    fclose(s);
    mmapring_destroy(&rb);

    return 0;
}
