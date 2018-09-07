/*
 * jbufwriter.h
 *
 * @chuanjiong
 */

#ifndef _J_BUFWRITER_H_
#define _J_BUFWRITER_H_

#include "jcommon.h"

#ifdef __cplusplus
extern "C"
{
#endif

jhandle jbufwriter_open(const char *url, int buf_size);

void jbufwriter_B32(jhandle h, uint32_t v);

void jbufwriter_L32(jhandle h, uint32_t v);

void jbufwriter_B24(jhandle h, uint32_t v);

void jbufwriter_L24(jhandle h, uint32_t v);

void jbufwriter_B16(jhandle h, uint32_t v);

void jbufwriter_L16(jhandle h, uint32_t v);

void jbufwriter_8(jhandle h, uint32_t v);

int jbufwriter_dump(jhandle h, int size, uint8_t v);

int jbufwriter_seek(jhandle h, int64_t pos, int base);

int64_t jbufwriter_tell(jhandle h);

int jbufwriter_write(jhandle h, uint8_t *buf, int size);

void jbufwriter_close(jhandle h);

#ifdef __cplusplus
}
#endif

#endif //_J_BUFWRITER_H_


