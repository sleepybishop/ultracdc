#ifndef ULTRACDC_H
#define ULTRACDC_H

#include "chunker.h"

chunker_cfg ultracdc_init(uint32_t mi, uint32_t av, uint32_t ma);
size_t ultracdc_cut(chunker_cfg *cfg, uint8_t *data, size_t len);

#endif
