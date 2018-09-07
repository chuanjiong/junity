/*
 * jopus.h
 *
 * @chuanjiong
 */

#ifndef _J_OPUS_H_
#define _J_OPUS_H_

#include "jlib.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct jopus_frame {
    int size;
    uint8_t *data;
}jopus_frame;

//encoder

jhandle jopus_encoder_open(int samplerate, int channels, int bitrate);

jopus_frame *jopus_encoder_enc(jhandle h, uint8_t *buf, int samplesize);

void jopus_encoder_close(jhandle h);

//decoder

jhandle jopus_decoder_open(int samplerate, int channels, int bpp);

jopus_frame *jopus_decoder_dec(jhandle h, uint8_t *buf, int size);

void jopus_decoder_close(jhandle h);

void jopus_frame_free(jopus_frame *frame);

#ifdef __cplusplus
}
#endif

#endif //_J_OPUS_H_


