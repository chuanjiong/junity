/*
 * jmm_rtp.h
 *
 * @chuanjiong
 */

#ifndef _JMM_RTP_H_
#define _JMM_RTP_H_

#include "jmm_util.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define JMM_RTP_PAYLOAD_SIZE_MAX        (1400)

#define JMM_RTP_MERGE_BUF_SIZE          (512*1024)  //max i-frame size

//split

typedef struct jmm_rtp_split_cfg {
    jmm_codec_type type;
    int pt;
    int seq;
    int timebase;
}jmm_rtp_split_cfg;

jhandle jmm_rtp_split_open(jmm_rtp_split_cfg *cfg);

void jmm_rtp_split_close(jhandle h);

int jmm_rtp_split_write(jhandle h, jmm_packet *packet);

int jmm_rtp_split_read(jhandle h, uint8_t **buf, int *size);

//merge

jhandle jmm_rtp_merge_open(jmm_codec_type type);

void jmm_rtp_merge_close(jhandle h);

int jmm_rtp_merge_write(jhandle h, uint8_t *buf, int size);

jmm_packet *jmm_rtp_merge_read(jhandle h);

int jmm_rtp_merge_size(jhandle h);

#ifdef __cplusplus
}
#endif

#endif //_JMM_RTP_H_


