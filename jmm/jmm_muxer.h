/*
 * jmm_muxer.h
 *
 * @chuanjiong
 */

#ifndef _JMM_MUXER_H_
#define _JMM_MUXER_H_

#include "jmm_module.h"

#ifdef __cplusplus
extern "C"
{
#endif

jhandle jmm_muxer_open(const char *url);

void jmm_muxer_close(jhandle h);

int jmm_muxer_write(jhandle h, jmm_packet *packet);

#ifdef __cplusplus
}
#endif

#endif //_JMM_MUXER_H_


