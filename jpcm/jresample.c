/*
 * jresample.c
 *
 * @chuanjiong
 */

#include "jresample.h"
#include "speex_resampler.h"

typedef struct jresample_context {
    SpeexResamplerState *resampler;
}jresample_context;

jhandle jresample_open(int channels, int src_samplerate, int dst_samplerate)
{
    jresample_context *ctx = (jresample_context *)jmalloc(sizeof(jresample_context));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(jresample_context));

    ctx->resampler = speex_resampler_init(channels, src_samplerate, dst_samplerate, 3, NULL);
    if (ctx->resampler == NULL)
    {
        jfree(ctx);
        return NULL;
    }

    speex_resampler_skip_zeros(ctx->resampler);

    return ctx;
}

int jresample_resample(jhandle h, const short *src, int src_sample, short *dst, int dst_sample)
{
    if ((h==NULL) || (src==NULL) || (src_sample<=0) || (dst==NULL) || (dst_sample<=0))
        return 0;

    jresample_context *ctx = (jresample_context *)h;

    int sample = src_sample;
    speex_resampler_process_interleaved_int(ctx->resampler, src, (spx_uint32_t *)(&src_sample), dst, (spx_uint32_t *)(&dst_sample));
    if (src_sample != sample)
        jerr("[jresample] output buffer is not enough\n");

    return dst_sample;
}

void jresample_close(jhandle h)
{
    if (h == NULL)
        return;

    jresample_context *ctx = (jresample_context *)h;

    speex_resampler_destroy(ctx->resampler);

    jfree(ctx);
}


