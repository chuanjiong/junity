/*
 * jmm_util.h
 *
 * @chuanjiong
 */

#ifndef _JMM_UTIL_H_
#define _JMM_UTIL_H_

#include "jmm_type.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define RB32            (jbufreader_B32(ctx->rd))
#define RL32            (jbufreader_L32(ctx->rd))
#define RB24            (jbufreader_B24(ctx->rd))
#define RL24            (jbufreader_L24(ctx->rd))
#define RB16            (jbufreader_B16(ctx->rd))
#define RL16            (jbufreader_L16(ctx->rd))
#define R8              (jbufreader_8(ctx->rd))

#define WB32(v)         (jbufwriter_B32(ctx->wt,(v)))
#define WL32(v)         (jbufwriter_L32(ctx->wt,(v)))
#define WB24(v)         (jbufwriter_B24(ctx->wt,(v)))
#define WL24(v)         (jbufwriter_L24(ctx->wt,(v)))
#define WB16(v)         (jbufwriter_B16(ctx->wt,(v)))
#define WL16(v)         (jbufwriter_L16(ctx->wt,(v)))
#define W8(v)           (jbufwriter_8(ctx->wt,(v)))

#define BL32(v)         ((((v)&0xff)<<24) | (((v)&0xff00)<<8) | (((v)&0xff0000)>>8) | (((v)&0xff000000)>>24))
#define BL64(v)         ((((v)&0xff)<<56) | (((v)&0xff00)<<40) | (((v)&0xff0000)<<24) | (((v)&0xff000000)<<8) | (((v)&0xff00000000)>>8) | (((v)&0xff0000000000)>>24) |(((v)&0xff000000000000)>>40) | (((v)&0xff00000000000000)>>56))

// packet

jmm_packet *jmm_packet_alloc(int size);

void jmm_packet_free(jmm_packet *packet);

jmm_packet *jmm_packet_clone(jmm_packet *packet);

// aac

int jmm_aac_asc_parse(jmm_packet *packet, jmm_asc_info *info);

jmm_packet *jmm_aac_adts2es(jmm_packet *packet, jbool copy);

jmm_packet *jmm_aac_es2adts(jmm_packet *packet, jmm_asc_info *info, jbool copy);

// avc

int jmm_avc_avcc_parse(jmm_packet *packet, jmm_avcc_info *info);

jmm_packet *jmm_avc_avcc2annexb(jmm_packet *packet, jbool copy);

jmm_packet *jmm_avc_annexb2avcc(jmm_packet *packet, jbool copy);

jmm_packet *jmm_avc_mp42annexb(jmm_packet *packet, jbool copy);

jmm_packet *jmm_avc_annexb2mp4(jmm_packet *packet, jbool copy);

// flv

jmm_packet *jmm_packet_flv_tag(jmm_packet *packet);

jmm_packet *fetch_sps_pps(uint8_t *buf);

jmm_packet *jmm_packet_merge(jmm_packet *pkt1, jmm_packet *pkt2);

#ifdef __cplusplus
}
#endif

#endif //_JMM_UTIL_H_


