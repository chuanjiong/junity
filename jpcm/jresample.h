/*
 * jresample.h
 *
 * @chuanjiong
 */

#ifndef _J_RESAMPLE_H_
#define _J_RESAMPLE_H_

#include "jlib.h"

#ifdef __cplusplus
extern "C"
{
#endif

jhandle jresample_open(int channels, int src_samplerate, int dst_samplerate);

int jresample_resample(jhandle h, const short *src, int src_sample, short *dst, int dst_sample);

void jresample_close(jhandle h);

#ifdef __cplusplus
}
#endif

#endif //_J_RESAMPLE_H_


