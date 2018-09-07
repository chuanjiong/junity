/*
 * jbufreader.h
 *
 * @chuanjiong
 */

#ifndef _J_BUFREADER_H_
#define _J_BUFREADER_H_

#include "jcommon.h"

#ifdef __cplusplus
extern "C"
{
#endif

jhandle jbufreader_open(const char *url);

uint32_t jbufreader_B32(jhandle h);

uint32_t jbufreader_L32(jhandle h);

uint32_t jbufreader_B24(jhandle h);

uint32_t jbufreader_L24(jhandle h);

uint32_t jbufreader_B16(jhandle h);

uint32_t jbufreader_L16(jhandle h);

uint32_t jbufreader_8(jhandle h);

jbool jbufreader_eof(jhandle h);

int jbufreader_skip(jhandle h, int64_t size);

int jbufreader_seek(jhandle h, int64_t pos, int base);

int64_t jbufreader_tell(jhandle h);

int64_t jbufreader_read(jhandle h, uint8_t *buf, int64_t size);

void jbufreader_close(jhandle h);

#ifdef __cplusplus
}
#endif

#endif //_J_BUFREADER_H_


