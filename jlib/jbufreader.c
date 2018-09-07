/*
 * jbufreader.c
 *
 * @chuanjiong
 */

#include "jbufreader.h"
#include "jvfs.h"

#define POS                     (ctx->pos)
#define SIZE                    (ctx->size)
#define BUF                     (ctx->buf)

typedef struct jbufreader_context {
    jhandle h;
    int64_t pos;
    int64_t size;
    uint8_t buf[BUF_RW_BUF_SIZE];
}jbufreader_context;

static int jbufreader_fill(jbufreader_context *ctx)
{
    if (ctx == NULL)
        return -1;

    if (ctx->size > 0)
    {
        memmove(ctx->buf, &(ctx->buf[ctx->pos]), ctx->size);
        ctx->pos = 0;
    }

    int size = jvfs_read(ctx->h, &(ctx->buf[ctx->size]), BUF_RW_BUF_SIZE-ctx->size);
    if (size <= 0)
        return ctx->size;

    ctx->pos = 0;
    ctx->size += size;

    return ctx->size;
}

jhandle jbufreader_open(const char *url)
{
    if (url == NULL)
        return NULL;

    jbufreader_context *ctx = (jbufreader_context *)jmalloc(sizeof(jbufreader_context));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(jbufreader_context));

    ctx->h = jvfs_open(url, jfalse);
    if (ctx->h == NULL)
    {
        jfree(ctx);
        return NULL;
    }

    jbufreader_fill(ctx);

    return ctx;
}

uint32_t jbufreader_B32(jhandle h)
{
    if (h == NULL)
        return 0;

    jbufreader_context *ctx = (jbufreader_context *)h;

    if (ctx->size < 4)
        if (jbufreader_fill(ctx) < 4)
            return 0;

    uint32_t v = (BUF[POS]<<24) | (BUF[POS+1]<<16) | (BUF[POS+2]<<8) | (BUF[POS+3]);

    POS += 4;
    SIZE -= 4;

    return v;
}

uint32_t jbufreader_L32(jhandle h)
{
    if (h == NULL)
        return 0;

    jbufreader_context *ctx = (jbufreader_context *)h;

    if (ctx->size < 4)
        if (jbufreader_fill(ctx) < 4)
            return 0;

    uint32_t v = (BUF[POS+3]<<24) | (BUF[POS+2]<<16) | (BUF[POS+1]<<8) | (BUF[POS]);

    POS += 4;
    SIZE -= 4;

    return v;
}

uint32_t jbufreader_B24(jhandle h)
{
    if (h == NULL)
        return 0;

    jbufreader_context *ctx = (jbufreader_context *)h;

    if (ctx->size < 3)
        if (jbufreader_fill(ctx) < 3)
            return 0;

    uint32_t v = (BUF[POS]<<16) | (BUF[POS+1]<<8) | (BUF[POS+2]);

    POS += 3;
    SIZE -= 3;

    return v;
}

uint32_t jbufreader_L24(jhandle h)
{
    if (h == NULL)
        return 0;

    jbufreader_context *ctx = (jbufreader_context *)h;

    if (ctx->size < 3)
        if (jbufreader_fill(ctx) < 3)
            return 0;

    uint32_t v = (BUF[POS+2]<<16) | (BUF[POS+1]<<8) | (BUF[POS]);

    POS += 3;
    SIZE -= 3;

    return v;
}

uint32_t jbufreader_B16(jhandle h)
{
    if (h == NULL)
        return 0;

    jbufreader_context *ctx = (jbufreader_context *)h;

    if (ctx->size < 2)
        if (jbufreader_fill(ctx) < 2)
            return 0;

    uint32_t v = (BUF[POS]<<8) | (BUF[POS+1]);

    POS += 2;
    SIZE -= 2;

    return v;
}

uint32_t jbufreader_L16(jhandle h)
{
    if (h == NULL)
        return 0;

    jbufreader_context *ctx = (jbufreader_context *)h;

    if (ctx->size < 2)
        if (jbufreader_fill(ctx) < 2)
            return 0;

    uint32_t v = (BUF[POS+1]<<8) | (BUF[POS]);

    POS += 2;
    SIZE -= 2;

    return v;
}

uint32_t jbufreader_8(jhandle h)
{
    if (h == NULL)
        return 0;

    jbufreader_context *ctx = (jbufreader_context *)h;

    if (ctx->size < 1)
        if (jbufreader_fill(ctx) < 1)
            return 0;

    uint32_t v = BUF[POS];

    POS += 1;
    SIZE -= 1;

    return v;
}

jbool jbufreader_eof(jhandle h)
{
    if (h == NULL)
        return jtrue;

    jbufreader_context *ctx = (jbufreader_context *)h;

    if (ctx->size <= 0)
        if (jbufreader_fill(ctx) <= 0)
            return jtrue;

    return jfalse;
}

int jbufreader_skip(jhandle h, int64_t size)
{
    if ((h==NULL) || (size<=0))
        return ERROR_FAIL;

    jbufreader_context *ctx = (jbufreader_context *)h;

    int64_t skip;

    skip = jmin(size, ctx->size);
    ctx->size -= skip;
    ctx->pos += skip;
    size -= skip;

    if (size > 0)
        return jvfs_seek(ctx->h, size, SEEK_CUR);

    return SUCCESS;
}

int jbufreader_seek(jhandle h, int64_t pos, int base)
{
    if (h == NULL)
        return ERROR_FAIL;

    jbufreader_context *ctx = (jbufreader_context *)h;

    ctx->pos = 0;
    ctx->size = 0;

    return jvfs_seek(ctx->h, pos, base);
}

int64_t jbufreader_tell(jhandle h)
{
    if (h == NULL)
        return -1;

    jbufreader_context *ctx = (jbufreader_context *)h;

    return jvfs_tell(ctx->h)-ctx->size;
}

int64_t jbufreader_read(jhandle h, uint8_t *buf, int64_t size)
{
    if ((h==NULL) || (buf==NULL) || (size<=0))
        return 0;

    jbufreader_context *ctx = (jbufreader_context *)h;

    int64_t rsize = jmin(size, ctx->size);
    memcpy(buf, &(BUF[POS]), rsize);
    ctx->size -= rsize;
    ctx->pos += rsize;
    size -= rsize;

    if (size > 0)
    {
        int r = jvfs_read(ctx->h, &buf[rsize], size);
        if (r > 0)
            rsize += r;
    }

    return rsize;
}

void jbufreader_close(jhandle h)
{
    if (h == NULL)
        return;

    jbufreader_context *ctx = (jbufreader_context *)h;

    jvfs_close(ctx->h);

    jfree(ctx);
}


