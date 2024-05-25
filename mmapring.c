#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#include "mmapring.h"

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

mmapring_t mmapring_create(const char *path, off_t size)
{
    mmapring_t rng = {};

    rng.size = sysconf(_SC_PAGE_SIZE);
    while (rng.size < size) {
        rng.size = rng.size << 1;
    }

    unlink(path);
    int fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0600);
    if (fd < 0) {
        return rng;
    }

    if (ftruncate(fd, rng.size) < 0) {
        rng.err = "path fill error";
        return rng;
    }

    rng.base = mmap(0, rng.size << 1, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (rng.base == MAP_FAILED) {
        close(fd);
        rng.err = "ring map failure";
        return rng;
    }

    uint8_t *base = mmap(rng.base, rng.size, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fd, 0);
    if (base != rng.base) {
        munmap(rng.base, rng.size << 1);
        close(fd);
        rng.err = "invalid ring base";
        return rng;
    }

    base = mmap(rng.base + rng.size, rng.size, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fd, 0);
    if (base != (rng.base + rng.size)) {
        munmap(rng.base, rng.size << 1);
        close(fd);
        rng.err = "invalid ring loop";
        return rng;
    }

    close(fd);

    return rng;
}

void mmapring_destroy(mmapring_t *rng)
{
    if (rng) {
        munmap(rng->base, rng->size * 2);
    }
}

off_t mmapring_append(mmapring_t *rng, const uint8_t *p, off_t len)
{
    off_t wlen = (rng->size > len ? len : rng->size);

    memcpy(rng->base + rng->write_offset, p, wlen);
    rng->written += wlen;
    rng->write_offset = (rng->write_offset + wlen) % rng->size;

    return wlen;
}

void mmapring_reset(mmapring_t *rng)
{
    rng->write_offset = 0;
    rng->written = 0;
}
