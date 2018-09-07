/*
 * jmm_source.h
 *
 * @chuanjiong
 */

#ifndef _JMM_SOURCE_H_
#define _JMM_SOURCE_H_

#include "jmm_demuxer.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef int (*jmm_source_callback)(void *arg, jmm_packet *packet);

jhandle jmm_source_setup(const char *url, jbool net, jbool loop, jmm_source_callback cb, void *arg);

void jmm_source_shutdown(jhandle h);

#ifdef __cplusplus
}
#endif

#endif //_JMM_SOURCE_H_


