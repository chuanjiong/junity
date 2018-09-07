/*
 * jmm_muxer.c
 *
 * @chuanjiong
 */

#include "jmm_muxer.h"

typedef struct jmm_muxer_ctx {
    jhandle h;
    const jmm_muxer *muxer;
}jmm_muxer_ctx;

jhandle jmm_muxer_open(const char *url)
{
    if (url == NULL)
        return NULL;

    jmm_muxer_ctx *ctx = (jmm_muxer_ctx *)jmalloc(sizeof(jmm_muxer_ctx));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(jmm_muxer_ctx));

    jstring protocol = jstring_protocol(url);
    if (!jstring_compare(protocol, "file://"))
    {
        jstring ext = jstring_file_ext(url);
        if (!jstring_compare(ext, "flv"))
        {
            ctx->muxer = &jmm_flv_muxer;
        }
        else if (!jstring_compare(ext, "mp4"))
        {
            ctx->muxer = &jmm_mp4_muxer;
        }
        else if (!jstring_compare(ext, "ts"))
        {
            ctx->muxer = &jmm_ts_muxer;
        }
        else
        {
            jerr("[jmm_muxer] unknown file ext\n");
            jstring_free(ext);
            jstring_free(protocol);
            jfree(ctx);
            return NULL;
        }
        jstring_free(ext);
    }
    else if (!jstring_compare(protocol, "rtmp://"))
    {
        ctx->muxer = &jmm_rtmp_muxer;
    }
    else if (!jstring_compare(protocol, "udp://"))
    {
        ctx->muxer = &jmm_ts_muxer;
    }
    else
    {
        jerr("[jmm_muxer] unknown protocol\n");
        jstring_free(protocol);
        jfree(ctx);
        return NULL;
    }
    jstring_free(protocol);

    ctx->h = ctx->muxer->open(url);
    if (ctx->h == NULL)
    {
        jfree(ctx);
        return NULL;
    }

    return ctx;
}

void jmm_muxer_close(jhandle h)
{
    if (h == NULL)
        return;

    jmm_muxer_ctx *ctx = (jmm_muxer_ctx *)h;

    ctx->muxer->close(ctx->h);

    jfree(ctx);
}

int jmm_muxer_write(jhandle h, jmm_packet *packet)
{
    if ((h==NULL) || (packet==NULL))
        return ERROR_FAIL;

    jmm_muxer_ctx *ctx = (jmm_muxer_ctx *)h;

    return ctx->muxer->write(ctx->h, packet);
}


