/*
 * jbitreader.h
 *
 * @chuanjiong
 */

#ifndef _J_BITREADER_H_
#define _J_BITREADER_H_

#include "jcommon.h"

#ifdef __cplusplus
extern "C"
{
#endif

jhandle jbitreader_alloc(uint8_t *buf, int size);

void jbitreader_free(jhandle h);

uint32_t jbitreader_read(jhandle h, int bits);

#ifdef __cplusplus
}
#endif

#endif //_J_BITREADER_H_


