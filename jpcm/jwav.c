/*
 * jwav.c
 *
 * @chuanjiong
 */

#include "jwav.h"

typedef struct jwav_reader_context {
    jhandle rd;
    jwav_format fmt;
}jwav_reader_context;

jhandle jwav_reader_open(const char *url, jwav_format *fmt)
{
    if ((url==NULL) || (fmt==NULL))
        return NULL;

    jwav_reader_context *ctx = (jwav_reader_context *)jmalloc(sizeof(jwav_reader_context));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(jwav_reader_context));

    ctx->rd = jbufreader_open(url);
    if (ctx->rd == NULL)
    {
        jfree(ctx);
        return NULL;
    }

    uint8_t buf[36] = {0};
    jbufreader_read(ctx->rd, buf, 36);
    if ((buf[0]!='R') || (buf[1]!='I') || (buf[2]!='F') || (buf[3]!='F')
        || (buf[8]!='W') || (buf[9]!='A') || (buf[10]!='V') || (buf[11]!='E')
        || (buf[12]!='f') || (buf[13]!='m') || (buf[14]!='t') || (buf[15]!=' ')
        || (buf[20]!=1) || (buf[21]!=0))
    {
        jbufreader_close(ctx->rd);
        jfree(ctx);
        return NULL;
    }

    (*fmt).channels = buf[22] | (buf[23]<<8);
    (*fmt).samplerate = buf[24] | (buf[25]<<8) | (buf[26]<<16) | (buf[27]<<24);
    (*fmt).bpp = buf[34] | (buf[35]<<8);

    while (1)
    {
        uint32_t id = jbufreader_B32(ctx->rd);
        uint32_t size = jbufreader_L32(ctx->rd);
        if (id == 0x64617461) //data
            break;
        jbufreader_skip(ctx->rd, size);
    }

    return ctx;
}

int jwav_reader_read(jhandle h, uint8_t *buf, int size)
{
    if ((h==NULL) || (buf==NULL) || (size<=0))
        return 0;

    jwav_reader_context *ctx = (jwav_reader_context *)h;

    return jbufreader_read(ctx->rd, buf, size);
}

void jwav_reader_close(jhandle h)
{
    if (h == NULL)
        return;

    jwav_reader_context *ctx = (jwav_reader_context *)h;

    jbufreader_close(ctx->rd);
    jfree(ctx);
}

typedef struct jwav_writer_context {
    jhandle wt;
    int size;
}jwav_writer_context;

jhandle jwav_writer_open(const char *url, jwav_format *fmt)
{
    if ((url==NULL) || (fmt==NULL))
        return NULL;

    jwav_writer_context *ctx = (jwav_writer_context *)jmalloc(sizeof(jwav_writer_context));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(jwav_writer_context));

    ctx->wt = jbufwriter_open(url, 0);
    if (ctx->wt == NULL)
    {
        jfree(ctx);
        return NULL;
    }

    uint8_t buf[22] = {'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ', 0x10, 0, 0, 0, 0x01, 0};
    jbufwriter_write(ctx->wt, buf, 22);
    jbufwriter_L16(ctx->wt, fmt->channels);
    jbufwriter_L32(ctx->wt, fmt->samplerate);
    jbufwriter_L32(ctx->wt, (fmt->samplerate*fmt->channels*fmt->bpp)/8);
    jbufwriter_L16(ctx->wt, 4);
    jbufwriter_L16(ctx->wt, fmt->bpp);
    jbufwriter_B32(ctx->wt, 0x64617461); //data
    jbufwriter_L32(ctx->wt, 0);          //size

    return ctx;
}

int jwav_writer_write(jhandle h, uint8_t *buf, int size)
{
    if ((h==NULL) || (buf==NULL) || (size<=0))
        return 0;

    jwav_writer_context *ctx = (jwav_writer_context *)h;

    int w = jbufwriter_write(ctx->wt, buf, size);
    ctx->size += w;

    return w;
}

void jwav_writer_close(jhandle h)
{
    if (h == NULL)
        return;

    jwav_writer_context *ctx = (jwav_writer_context *)h;

    jbufwriter_seek(ctx->wt, 4, SEEK_SET);
    jbufwriter_L32(ctx->wt, ctx->size+0x24);
    jbufwriter_seek(ctx->wt, 0x28, SEEK_SET);
    jbufwriter_L32(ctx->wt, ctx->size);

    jbufwriter_close(ctx->wt);
    jfree(ctx);
}


