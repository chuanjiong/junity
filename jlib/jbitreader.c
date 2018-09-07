/*
 * jbitreader.c
 *
 * @chuanjiong
 */

#include "jbitreader.h"

typedef struct jbitreader_context {
    int total_bits;
    int read_bits;
    uint8_t *data;
}jbitreader_context;

jhandle jbitreader_alloc(uint8_t *buf, int size)
{
    if ((buf==NULL) || (size<=0))
        return NULL;

    jbitreader_context *ctx = (jbitreader_context *)jmalloc(sizeof(jbitreader_context));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(jbitreader_context));

    ctx->total_bits = size * 8;
    ctx->data = buf;

    return ctx;
}

void jbitreader_free(jhandle h)
{
    if (h == NULL)
        return;

    jbitreader_context *ctx = (jbitreader_context *)h;

    jfree(ctx);
}

static uint32_t jbitreader_read_1bit(jbitreader_context *ctx)
{
    if (ctx == NULL)
        return 0;

    if (ctx->read_bits >= ctx->total_bits)
        return 0;

    int byte_idx = ctx->read_bits / 8;
    int bit_idx = ctx->read_bits % 8;

    ctx->read_bits++;

    return (ctx->data[byte_idx]>>(7-bit_idx)) & 0x1;
}

uint32_t jbitreader_read(jhandle h, int bits)
{
    if ((h==NULL) || (bits<=0) || (bits>32))
        return 0;

    jbitreader_context *ctx = (jbitreader_context *)h;

    int i, n=bits;
    uint32_t v=0;

    for (i=0; i<bits; i++,n--)
        v |= (jbitreader_read_1bit(ctx) << (n-1));

    return v;
}


