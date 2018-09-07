/*
 * jbufwriter.c
 *
 * @chuanjiong
 */

#include "jbufwriter.h"
#include "jvfs.h"

#define POS                     (ctx->pos)
#define SIZE                    (ctx->size)
#define BUF                     (ctx->buf)

typedef struct jbufwriter_context {
    jhandle h;

    int64_t pos;
    int64_t size;

    uint8_t *buf;
    int64_t buf_size;
}jbufwriter_context;

static int jbufwriter_flush(jbufwriter_context *ctx)
{
    if (ctx == NULL)
        return ERROR_FAIL;

    if (ctx->size >= ctx->buf_size)
        return SUCCESS;

    int64_t size = jvfs_write(ctx->h, ctx->buf, ctx->pos);
    if (size != ctx->pos)
        return ERROR_FAIL;

    ctx->pos = 0;
    ctx->size = ctx->buf_size;

    return SUCCESS;
}

jhandle jbufwriter_open(const char *url, int buf_size)
{
    if (url == NULL)
        return NULL;

    jbufwriter_context *ctx = (jbufwriter_context *)jmalloc(sizeof(jbufwriter_context));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(jbufwriter_context));

    ctx->h = jvfs_open(url, jtrue);
    if (ctx->h == NULL)
    {
        jfree(ctx);
        return NULL;
    }

    if (buf_size <= 0)
        buf_size = BUF_RW_BUF_SIZE;

    ctx->buf_size = buf_size;
    ctx->buf = (uint8_t *)jmalloc(ctx->buf_size);
    if (ctx->buf == NULL)
    {
        jvfs_close(ctx->h);
        jfree(ctx);
        return NULL;
    }
    memset(ctx->buf, 0, ctx->buf_size);

    ctx->size = ctx->buf_size;

    return ctx;
}

void jbufwriter_B32(jhandle h, uint32_t v)
{
    if (h == NULL)
        return;

    jbufwriter_context *ctx = (jbufwriter_context *)h;

    if (ctx->size < 4)
        if (jbufwriter_flush(ctx) != SUCCESS)
            return;

    BUF[POS++] = (v>>24) & 0xff;
    BUF[POS++] = (v>>16) & 0xff;
    BUF[POS++] = (v>>8) & 0xff;
    BUF[POS++] = v & 0xff;

    SIZE -= 4;
}

void jbufwriter_L32(jhandle h, uint32_t v)
{
    if (h == NULL)
        return;

    jbufwriter_context *ctx = (jbufwriter_context *)h;

    if (ctx->size < 4)
        if (jbufwriter_flush(ctx) != SUCCESS)
            return;

    BUF[POS++] = v & 0xff;
    BUF[POS++] = (v>>8) & 0xff;
    BUF[POS++] = (v>>16) & 0xff;
    BUF[POS++] = (v>>24) & 0xff;

    SIZE -= 4;
}

void jbufwriter_B24(jhandle h, uint32_t v)
{
    if (h == NULL)
        return;

    jbufwriter_context *ctx = (jbufwriter_context *)h;

    if (ctx->size < 3)
        if (jbufwriter_flush(ctx) != SUCCESS)
            return;

    BUF[POS++] = (v>>16) & 0xff;
    BUF[POS++] = (v>>8) & 0xff;
    BUF[POS++] = v & 0xff;

    SIZE -= 3;
}

void jbufwriter_L24(jhandle h, uint32_t v)
{
    if (h == NULL)
        return;

    jbufwriter_context *ctx = (jbufwriter_context *)h;

    if (ctx->size < 3)
        if (jbufwriter_flush(ctx) != SUCCESS)
            return;

    BUF[POS++] = v & 0xff;
    BUF[POS++] = (v>>8) & 0xff;
    BUF[POS++] = (v>>16) & 0xff;

    SIZE -= 3;
}

void jbufwriter_B16(jhandle h, uint32_t v)
{
    if (h == NULL)
        return;

    jbufwriter_context *ctx = (jbufwriter_context *)h;

    if (ctx->size < 2)
        if (jbufwriter_flush(ctx) != SUCCESS)
            return;

    BUF[POS++] = (v>>8) & 0xff;
    BUF[POS++] = v & 0xff;

    SIZE -= 2;
}

void jbufwriter_L16(jhandle h, uint32_t v)
{
    if (h == NULL)
        return;

    jbufwriter_context *ctx = (jbufwriter_context *)h;

    if (ctx->size < 2)
        if (jbufwriter_flush(ctx) != SUCCESS)
            return;

    BUF[POS++] = v & 0xff;
    BUF[POS++] = (v>>8) & 0xff;

    SIZE -= 2;
}

void jbufwriter_8(jhandle h, uint32_t v)
{
    if (h == NULL)
        return;

    jbufwriter_context *ctx = (jbufwriter_context *)h;

    if (ctx->size < 1)
        if (jbufwriter_flush(ctx) != SUCCESS)
            return;

    BUF[POS++] = v & 0xff;

    SIZE--;
}

int jbufwriter_dump(jhandle h, int size, uint8_t v)
{
    if ((h==NULL) || (size<=0))
        return ERROR_FAIL;

    uint8_t *buf = (uint8_t *)jmalloc(size);
    if (buf == NULL)
        return ERROR_FAIL;
    memset(buf, v, size);

    jbufwriter_write(h, buf, size);

    jfree(buf);

    return SUCCESS;
}

int jbufwriter_seek(jhandle h, int64_t pos, int base)
{
    if (h == NULL)
        return ERROR_FAIL;

    jbufwriter_context *ctx = (jbufwriter_context *)h;

    if (jbufwriter_flush(ctx) != SUCCESS)
        return ERROR_FAIL;

    return jvfs_seek(ctx->h, pos, base);
}

int64_t jbufwriter_tell(jhandle h)
{
    if (h == NULL)
        return -1;

    jbufwriter_context *ctx = (jbufwriter_context *)h;

    return jvfs_tell(ctx->h)+ctx->pos;
}

int jbufwriter_write(jhandle h, uint8_t *buf, int size)
{
    if ((h==NULL) || (buf==NULL) || (size<=0))
        return 0;

    jbufwriter_context *ctx = (jbufwriter_context *)h;

    int wsize, total=0;

rewrite:
    if (ctx->size == 0)
        jbufwriter_flush(ctx);

    wsize = jmin(size, ctx->size);
    memcpy(&(BUF[POS]), buf, wsize);
    ctx->size -= wsize;
    ctx->pos += wsize;
    size -= wsize;
    buf += wsize;
    total += wsize;

    if (size > 0)
        goto rewrite;

    return total;
}

void jbufwriter_close(jhandle h)
{
    if (h == NULL)
        return;

    jbufwriter_context *ctx = (jbufwriter_context *)h;

    jbufwriter_flush(ctx);

    jvfs_close(ctx->h);

    jfree(ctx->buf);

    jfree(ctx);
}


