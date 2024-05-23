#include "ultracdc.h"

#define ULTRACDC_CLAMP(x, a, b) ((x < (a)) ? (a) : ((x > b) ? b : x))

#define MIN_CHUNK_SIZE (1 << 11) // 2 KiB
#define MAX_CHUNK_SIZE (1 << 16) // 64 KiB

static const uint64_t PATTERN = 0xAAAAAAAAAAAAAAAAULL;
static const uint64_t MASK_S = 0x2F;
static const uint64_t MASK_L = 0x2C;
static const uint64_t LEST = 64;

static uint32_t normal_size(const uint32_t mi, uint32_t av, const uint32_t ma)
{
    uint32_t ns = mi + (1 << 13);
    if (ns > av)
        return ns;
    if (av > ma)
        return ma;
    return av;
}

static uint32_t cut(const uint8_t *src, const uint32_t len, const uint32_t mi, const uint32_t ma, uint32_t ns)
{
    uint32_t n = len, cnt = 0;

    if (n <= mi)
        return n;
    if (n >= ma)
        n = ma;
    else if (n <= ns)
        ns = n;

    uint64_t *owin = (uint64_t *)(src + mi);
    uint64_t dist = __builtin_popcountll(*owin ^ PATTERN);
    for (uint32_t i = mi + 8; i + 8 < n; i += 8) {
        uint64_t *iwin = (uint64_t *)(src + i);
        if ((*owin ^ *iwin) == 0) {
            cnt++;
            if (cnt == LEST)
                return i + 8;
        } else {
            cnt = 0;
            for (uint32_t j = 0; j < 8; j++) {
                if ((dist & ((i < ns) ? MASK_S : MASK_L)) == 0)
                    return i + 8;
                //SLIDE(dist);
                dist -= __builtin_popcount(src[i + j - 8] ^ 0xAA);
                dist += __builtin_popcount(src[i + j] ^ 0xAA);
            }
            owin = iwin;
        }
    }
    return n;
}

cdc_ctx ultracdc_init(uint32_t mi, uint32_t av, uint32_t ma)
{
    mi = ULTRACDC_CLAMP(mi, MIN_CHUNK_SIZE, av);
    ma = ULTRACDC_CLAMP(ma, av, MAX_CHUNK_SIZE);
    uint32_t ns = normal_size(mi, av, ma);
    cdc_ctx ctx = {.mi = mi, .ma = ma, .ns = ns, .pos = 0};
    return ctx;
}

size_t ultracdc_update(cdc_ctx *ctx, uint8_t *data, size_t len, int end, on_chunk cb, void *arg)
{
    size_t offset = 0;
    while (((len - offset) >= ctx->ma) || (end && (offset < len))) {
        uint32_t cp = cut(data + offset, len - offset, ctx->mi, ctx->ma, ctx->ns);
        cb(arg, ctx->pos + offset, cp);
        offset += cp;
    }
    ctx->pos += offset;
    return offset;
}

size_t ultracdc_stream(FILE *stream, uint32_t mi, uint32_t av, uint32_t ma, on_chunk cb, void *arg)
{
    size_t offset = 0;
    int end = 0;
    cdc_ctx cdc = ultracdc_init(mi, av, ma), *ctx = &cdc;
    size_t rs = ctx->ma * 4;
    rs = ULTRACDC_CLAMP(rs, 0, UINT32_MAX);
    uint8_t *data = malloc(rs);
    while (!end) {
        size_t ar = fread(data, 1, rs, stream);
        end = feof(stream);
        offset += ultracdc_update(ctx, data, ar, end, cb, arg);
        fseek(stream, offset, SEEK_SET);
    }
    free(data);
    return offset;
}
