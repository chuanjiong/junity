/*
 * jmm_demuxer.h
 *
 * @chuanjiong
 */

#ifndef _JMM_DEMUXER_H_
#define _JMM_DEMUXER_H_

#include "jmm_module.h"

#ifdef __cplusplus
extern "C"
{
#endif

jhandle jmm_demuxer_open(const char *url);

void jmm_demuxer_close(jhandle h);

jmm_packet *jmm_demuxer_extradata(jhandle h, jmm_codec_type type);

jmm_packet *jmm_demuxer_read(jhandle h);

int jmm_demuxer_seek(jhandle h, int64_t ts);

int jmm_demuxer_finfo(jhandle h, jmm_file_info *info);

#ifdef __cplusplus
}
#endif

#endif //_JMM_DEMUXER_H_


