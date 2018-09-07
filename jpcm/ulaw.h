/*
 * ulaw.h
 *
 * @chuanjiong
 */

#ifndef _ULAW_H_
#define _ULAW_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

int pcm16_to_ulaw(const unsigned short *src, int sample, unsigned char *dst);

int ulaw_to_pcm16(const unsigned char *src, int sample, unsigned short *dst);

#ifdef __cplusplus
}
#endif

#endif //_ULAW_H_


