#ifndef MMAPRING_H_
#define MMAPRING_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t *base;
    off_t size;
    off_t write_offset;
    off_t written;
    char *err;
} mmapring_t;

mmapring_t mmapring_create(const char *path, off_t size);
void mmapring_destroy(mmapring_t *rng);
off_t mmapring_append(mmapring_t *rng, const uint8_t *p, off_t len);
void mmapring_reset(mmapring_t *rng);

#ifdef __cplusplus
}
#endif

#endif // MMAPRING_H_
