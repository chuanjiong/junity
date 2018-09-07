/*
 * jopus.c
 *
 * @chuanjiong
 */

#include "jopus.h"
#include "opus.h"

static jopus_frame *jopus_frame_alloc(int size)
{
    jopus_frame *frame = (jopus_frame *)jmalloc(sizeof(jopus_frame));
    if (frame == NULL)
        return NULL;
    memset(frame, 0, sizeof(jopus_frame));

    if (size > 0)
    {
        frame->data = (uint8_t *)jmalloc(size);
        if (frame->data == NULL)
        {
            jfree(frame);
            return NULL;
        }
        memset(frame->data, 0, size);
        frame->size = size;
    }

    return frame;
}

#define JOPUS_ENCODER_BUF_SIZE      (1024)

typedef struct jopus_encoder_context {
    OpusEncoder *enc;
    uint8_t buf[JOPUS_ENCODER_BUF_SIZE];
}jopus_encoder_context;

jhandle jopus_encoder_open(int samplerate, int channels, int bitrate)
{
    jopus_encoder_context *ctx = (jopus_encoder_context *)jmalloc(sizeof(jopus_encoder_context));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(jopus_encoder_context));

    int err;
    ctx->enc = opus_encoder_create(samplerate, channels, OPUS_APPLICATION_AUDIO, &err);
    if (err < 0)
    {
        jfree(ctx);
        return NULL;
    }

    opus_encoder_ctl(ctx->enc, OPUS_SET_BITRATE(bitrate));
    return ctx;
}

jopus_frame *jopus_encoder_enc(jhandle h, uint8_t *buf, int samplesize)
{
    if ((h==NULL) || (buf==NULL) || (samplesize<0))
        return NULL;

    jopus_encoder_context *ctx = (jopus_encoder_context *)h;

    int size = opus_encode(ctx->enc, (const opus_int16 *)buf, samplesize, ctx->buf, JOPUS_ENCODER_BUF_SIZE);
    if (size < 0)
        return NULL;

    jopus_frame *frame = jopus_frame_alloc(size);
    if (frame == NULL)
        return NULL;

    memcpy(frame->data, ctx->buf, size);
    return frame;
}

void jopus_encoder_close(jhandle h)
{
    if (h == NULL)
        return;

    jopus_encoder_context *ctx = (jopus_encoder_context *)h;

    opus_encoder_destroy(ctx->enc);
    jfree(ctx);
}

#define JOPUS_DECODER_BUF_SIZE      (16*1024)

typedef struct jopus_decoder_context {
    OpusDecoder *dec;
    int channels;
    int bpp;
    uint8_t buf[JOPUS_DECODER_BUF_SIZE];
}jopus_decoder_context;

jhandle jopus_decoder_open(int samplerate, int channels, int bpp)
{
    jopus_decoder_context *ctx = (jopus_decoder_context *)jmalloc(sizeof(jopus_decoder_context));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(jopus_decoder_context));

    int err;
    ctx->dec = opus_decoder_create(samplerate, channels, &err);
    if (err < 0)
    {
        jfree(ctx);
        return NULL;
    }

    ctx->channels = channels;
    ctx->bpp = bpp;

    return ctx;
}

jopus_frame *jopus_decoder_dec(jhandle h, uint8_t *buf, int size)
{
    if ((h==NULL) || (buf==NULL) || (size<0))
        return NULL;

    jopus_decoder_context *ctx = (jopus_decoder_context *)h;

    int samplesize = opus_decode(ctx->dec, buf, size, (opus_int16 *)(ctx->buf), JOPUS_DECODER_BUF_SIZE, 0);
    if (samplesize < 0)
        return NULL;

    jopus_frame *frame = jopus_frame_alloc(samplesize*ctx->channels*ctx->bpp/8);
    if (frame == NULL)
        return NULL;

    memcpy(frame->data, ctx->buf, samplesize*ctx->channels*ctx->bpp/8);
    return frame;
}

void jopus_decoder_close(jhandle h)
{
    if (h == NULL)
        return;

    jopus_decoder_context *ctx = (jopus_decoder_context *)h;

    opus_decoder_destroy(ctx->dec);
    jfree(ctx);
}

void jopus_frame_free(jopus_frame *frame)
{
    if (frame == NULL)
        return;

    if (frame->size > 0)
        jfree(frame->data);

    jfree(frame);
}


