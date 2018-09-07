/*
 * jpcm_mix.h
 *
 * @chuanjiong
 */

#ifndef _J_PCM_MIX_H_
#define _J_PCM_MIX_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

int jpcm_mix(const short *src, short *dst, int sample);

#ifdef __cplusplus
}
#endif

#endif //_J_PCM_MIX_H_


