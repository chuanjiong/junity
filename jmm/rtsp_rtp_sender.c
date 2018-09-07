/*
 * rtsp_rtp_sender.c
 *
 * @chuanjiong
 */

#include "rtsp_rtp_sender.h"

typedef struct rtsp_rtp_sender_context {
    rtsp_rtp_sender_cfg cfg;
    jhandle a_spliter;
    jhandle v_spliter;
}rtsp_rtp_sender_context;

jhandle rtsp_rtp_sender_alloc(rtsp_rtp_sender_cfg *cfg)
{
    if (cfg == NULL)
        return NULL;

    rtsp_rtp_sender_context *ctx = (rtsp_rtp_sender_context *)jmalloc(sizeof(rtsp_rtp_sender_context));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(rtsp_rtp_sender_context));

    ctx->cfg = *cfg;

    jmm_rtp_split_cfg a_split_cfg = {JMM_CODEC_TYPE_AAC, 97, cfg->a_seq, cfg->a_timebase};
    jmm_rtp_split_cfg v_split_cfg = {JMM_CODEC_TYPE_AVC, 96, cfg->v_seq, cfg->v_timebase};

    ctx->a_spliter = jmm_rtp_split_open(&a_split_cfg);
    ctx->v_spliter = jmm_rtp_split_open(&v_split_cfg);

    return ctx;
}

void rtsp_rtp_sender_free(jhandle h)
{
    if (h == NULL)
        return;

    rtsp_rtp_sender_context *ctx = (rtsp_rtp_sender_context *)h;

    jfree(ctx);
}

int rtsp_rtp_sender_send(jhandle h, jmm_packet *packet)
{
    if ((h==NULL) || (packet==NULL))
        return ERROR_FAIL;

    rtsp_rtp_sender_context *ctx = (rtsp_rtp_sender_context *)h;

    uint8_t *payload = NULL;
    int payload_size = 0;

    if (packet->type == JMM_CODEC_TYPE_AAC)
    {
        if (jmm_rtp_split_write(ctx->a_spliter, packet) != SUCCESS)
            return ERROR_FAIL;
        while (jmm_rtp_split_read(ctx->a_spliter, &payload, &payload_size) == SUCCESS)
        {
            if (ctx->cfg.type == RTSP_RTP_SENDER_TYPE_TCP)
            {
                //1Byte:'$', 1Byte:'chn', 2Byte:'size'
                uint8_t sync[4] = {'$'};
                sync[1] = ctx->cfg.tcp.a_rtp_chn;
                sync[2] = (payload_size>>8) & 0xff;
                sync[3] = (payload_size) & 0xff;
                if (tcp_write(ctx->cfg.tcp.sock, sync, 4) != 4)
                {
                    jerr("[rtsp_rtp_sender] tcp write a-sync fail\n");
                    jfree(payload);
                    return ERROR_FAIL;
                }
                //send payload
                if (tcp_write(ctx->cfg.tcp.sock, payload, payload_size) != payload_size)
                {
                    jerr("[rtsp_rtp_sender] tcp write a-payload fail\n");
                    jfree(payload);
                    return ERROR_FAIL;
                }
            }
            else
            {
                //send payload
                if (udp_write(ctx->cfg.udp.a_rtp_sock, payload, payload_size) != payload_size)
                {
                    jerr("[rtsp_rtp_sender] udp write a-payload fail\n");
                    jfree(payload);
                    return ERROR_FAIL;
                }
            }
            jfree(payload);
        }
    }
    else
    {
        if (jmm_rtp_split_write(ctx->v_spliter, packet) != SUCCESS)
            return ERROR_FAIL;
        while (jmm_rtp_split_read(ctx->v_spliter, &payload, &payload_size) == SUCCESS)
        {
            if (ctx->cfg.type == RTSP_RTP_SENDER_TYPE_TCP)
            {
                //1Byte:'$', 1Byte:'chn', 2Byte:'size'
                uint8_t sync[4] = {'$'};
                sync[1] = ctx->cfg.tcp.v_rtp_chn;
                sync[2] = (payload_size>>8) & 0xff;
                sync[3] = (payload_size) & 0xff;
                if (tcp_write(ctx->cfg.tcp.sock, sync, 4) != 4)
                {
                    jerr("[rtsp_rtp_sender] tcp write v-sync fail\n");
                    jfree(payload);
                    return ERROR_FAIL;
                }
                //send payload
                if (tcp_write(ctx->cfg.tcp.sock, payload, payload_size) != payload_size)
                {
                    jerr("[rtsp_rtp_sender] tcp write v-payload fail\n");
                    jfree(payload);
                    return ERROR_FAIL;
                }
            }
            else
            {
                //send payload
                if (udp_write(ctx->cfg.udp.v_rtp_sock, payload, payload_size) != payload_size)
                {
                    jerr("[rtsp_rtp_sender] udp write v-payload fail\n");
                    jfree(payload);
                    return ERROR_FAIL;
                }
            }
            jfree(payload);
        }
    }

    return SUCCESS;
}


