#ifndef ULTRACDC_H
#define ULTRACDC_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  uint32_t mi;
  uint32_t ma;
  uint32_t ns;
  size_t pos;
} cdc_ctx;

typedef void(*on_chunk)(void* arg, size_t offset, size_t len);

cdc_ctx ultracdc_init(uint32_t mi, uint32_t av, uint32_t ma);
size_t ultracdc_update(cdc_ctx *ctx, uint8_t *data, size_t len, int end,
                      on_chunk cb, void *arg);
size_t ultracdc_stream(FILE *stream, uint32_t mi, uint32_t ns, uint32_t ma,
                      on_chunk cb, void *arg);
#endif
