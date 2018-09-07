/*
 * jmm_demuxer.c
 *
 * @chuanjiong
 */

#include "jmm_demuxer.h"

typedef struct jmm_demuxer_ctx {
    jhandle h;
    const jmm_demuxer *demuxer;
}jmm_demuxer_ctx;

jhandle jmm_demuxer_open(const char *url)
{
    if (url == NULL)
        return NULL;

    jmm_demuxer_ctx *ctx = (jmm_demuxer_ctx *)jmalloc(sizeof(jmm_demuxer_ctx));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(jmm_demuxer_ctx));

    jstring protocol = jstring_protocol(url);
    if (!jstring_compare(protocol, "file://"))
    {
        jstring ext = jstring_file_ext(url);
        if (!jstring_compare(ext, "flv"))
        {
            ctx->demuxer = &jmm_flv_demuxer;
        }
        else if (!jstring_compare(ext, "mp4"))
        {
            ctx->demuxer = &jmm_mp4_demuxer;
        }
        else
        {
            jerr("[jmm_demuxer] unknown file ext\n");
            jstring_free(ext);
            jstring_free(protocol);
            jfree(ctx);
            return NULL;
        }
        jstring_free(ext);
    }
    else if (!jstring_compare(protocol, "rtmp://"))
    {
        ctx->demuxer = &jmm_rtmp_demuxer;
    }
    else if (!jstring_compare(protocol, "rtsp://"))
    {
        ctx->demuxer = &jmm_rtsp_demuxer;
    }
    else
    {
        jerr("[jmm_demuxer] unknown protocol\n");
        jstring_free(protocol);
        jfree(ctx);
        return NULL;
    }
    jstring_free(protocol);

    ctx->h = ctx->demuxer->open(url);
    if (ctx->h == NULL)
    {
        jfree(ctx);
        return NULL;
    }

    return ctx;
}

void jmm_demuxer_close(jhandle h)
{
    if (h == NULL)
        return;

    jmm_demuxer_ctx *ctx = (jmm_demuxer_ctx *)h;

    ctx->demuxer->close(ctx->h);

    jfree(ctx);
}

jmm_packet *jmm_demuxer_extradata(jhandle h, jmm_codec_type type)
{
    if (h == NULL)
        return NULL;

    jmm_demuxer_ctx *ctx = (jmm_demuxer_ctx *)h;

    return ctx->demuxer->extradata(ctx->h, type);
}

jmm_packet *jmm_demuxer_read(jhandle h)
{
    if (h == NULL)
        return NULL;

    jmm_demuxer_ctx *ctx = (jmm_demuxer_ctx *)h;

    return ctx->demuxer->read(ctx->h);
}

int jmm_demuxer_seek(jhandle h, int64_t ts)
{
    if ((h==NULL) || (ts<0))
        return ERROR_FAIL;

    jmm_demuxer_ctx *ctx = (jmm_demuxer_ctx *)h;

    return ctx->demuxer->seek(ctx->h, ts);
}

int jmm_demuxer_finfo(jhandle h, jmm_file_info *info)
{
    if ((h==NULL) || (info==NULL))
        return ERROR_FAIL;

    jmm_demuxer_ctx *ctx = (jmm_demuxer_ctx *)h;

    return ctx->demuxer->finfo(ctx->h, info);
}


