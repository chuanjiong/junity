/*
 * jwav.h
 *
 * @chuanjiong
 */

#ifndef _J_WAV_H_
#define _J_WAV_H_

#include "jlib.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct jwav_format {
    int samplerate;
    int channels;
    int bpp;    //bits per sample
}jwav_format;

//reader

jhandle jwav_reader_open(const char *url, jwav_format *fmt);

int jwav_reader_read(jhandle h, uint8_t *buf, int size);

void jwav_reader_close(jhandle h);

//writer

jhandle jwav_writer_open(const char *url, jwav_format *fmt);

int jwav_writer_write(jhandle h, uint8_t *buf, int size);

void jwav_writer_close(jhandle h);

#ifdef __cplusplus
}
#endif

#endif //_J_WAV_H_


