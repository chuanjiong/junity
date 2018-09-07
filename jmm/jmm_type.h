/*
 * jmm_type.h
 *
 * @chuanjiong
 */

#ifndef _JMM_TYPE_H_
#define _JMM_TYPE_H_

#include "jlib.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
    JMM_CODEC_TYPE_AAC,
    JMM_CODEC_TYPE_AVC,
}jmm_codec_type;

typedef enum {
    JMM_BS_FMT_AAC_ASC,
    JMM_BS_FMT_AAC_ES,
    JMM_BS_FMT_AAC_ADTS,

    JMM_BS_FMT_AVC_AVCC,
    JMM_BS_FMT_AVC_ANNEXB,
    JMM_BS_FMT_AVC_MP4,
}jmm_bitstream_fmt;

typedef struct jmm_packet {
    jmm_codec_type type;
    jmm_bitstream_fmt fmt;

    jbool key;

    int64_t dts;    //us
    int64_t pts;    //us

    int size;
    uint8_t *data;
}jmm_packet;

typedef struct jmm_asc_info {
    int profile;                // 1:main, 2:lc, 3:ssr, 4:ltp
    int samplerate;
    int channels;
}jmm_asc_info;

typedef struct jmm_avcc_info {
    int width;
    int height;
}jmm_avcc_info;

typedef struct jmm_file_info {
    int64_t total_us;
}jmm_file_info;

typedef struct jmm_seek_entry {
    int64_t ts;     //us
    int64_t pos;
}jmm_seek_entry;

#ifdef __cplusplus
}
#endif

#endif //_JMM_TYPE_H_


