/*
 * jmm_module.h
 *
 * @chuanjiong
 */

#ifndef _JMM_MODULE_H_
#define _JMM_MODULE_H_

#include "jmm_util.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct jmm_muxer {
    jhandle (*open)(const char *url);
    void (*close)(jhandle h);
    int (*write)(jhandle h, jmm_packet *packet);
}jmm_muxer;

typedef struct jmm_demuxer {
    jhandle (*open)(const char *url);
    void (*close)(jhandle h);
    jmm_packet *(*extradata)(jhandle h, jmm_codec_type type);
    jmm_packet *(*read)(jhandle h);
    int (*seek)(jhandle h, int64_t ts);
    int (*finfo)(jhandle h, jmm_file_info *info);
}jmm_demuxer;

extern const jmm_muxer jmm_flv_muxer;
extern const jmm_muxer jmm_mp4_muxer;
extern const jmm_muxer jmm_rtmp_muxer;
extern const jmm_muxer jmm_ts_muxer;

extern const jmm_demuxer jmm_flv_demuxer;
extern const jmm_demuxer jmm_mp4_demuxer;
extern const jmm_demuxer jmm_rtmp_demuxer;
extern const jmm_demuxer jmm_rtsp_demuxer;

#ifdef __cplusplus
}
#endif

#endif //_JMM_MODULE_H_


