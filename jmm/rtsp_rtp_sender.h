/*
 * rtsp_rtp_sender.h
 *
 * @chuanjiong
 */

#ifndef _RTSP_RTP_SENDER_H_
#define _RTSP_RTP_SENDER_H_

#include "jmm.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum rtsp_rtp_sender_type {
    RTSP_RTP_SENDER_TYPE_TCP,
    RTSP_RTP_SENDER_TYPE_UDP,
}rtsp_rtp_sender_type;

typedef struct rtsp_rtp_sender_cfg {
    rtsp_rtp_sender_type type;

    union {
        struct {
            jsocket sock;
            int a_rtp_chn;
            int a_rtcp_chn;
            int v_rtp_chn;
            int v_rtcp_chn;
        }tcp;
        struct {
            jsocket a_rtp_sock;
            jsocket a_rtcp_sock;
            jsocket v_rtp_sock;
            jsocket v_rtcp_sock;
        }udp;
    };

    int a_seq;
    int a_timebase;

    int v_seq;
    int v_timebase;
}rtsp_rtp_sender_cfg;

jhandle rtsp_rtp_sender_alloc(rtsp_rtp_sender_cfg *cfg);

void rtsp_rtp_sender_free(jhandle h);

int rtsp_rtp_sender_send(jhandle h, jmm_packet *packet);

#ifdef __cplusplus
}
#endif

#endif //_RTSP_RTP_SENDER_H_


