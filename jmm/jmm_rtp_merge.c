/*
 * jmm_rtp_merge.c
 *
 * @chuanjiong
 */

#include "jmm_rtp.h"

typedef struct jmm_rtp_merge_ctx {
    jmm_codec_type type;
    jhandle pkt_q;

    uint8_t packet_buf[JMM_RTP_MERGE_BUF_SIZE];
    int packet_size;

    jbool sps_flag;
    jbool start_flag;

    int seq;
}jmm_rtp_merge_ctx;

jhandle jmm_rtp_merge_open(jmm_codec_type type)
{
    jmm_rtp_merge_ctx *ctx = (jmm_rtp_merge_ctx *)jmalloc(sizeof(jmm_rtp_merge_ctx));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(jmm_rtp_merge_ctx));

    ctx->type = type;

    ctx->pkt_q = jqueue_alloc();

    return ctx;
}

void jmm_rtp_merge_close(jhandle h)
{
    if (h == NULL)
        return;

    jmm_rtp_merge_ctx *ctx = (jmm_rtp_merge_ctx *)h;

    jmm_packet *pkt;
    while (jqueue_pop(ctx->pkt_q, (uint8_t **)&pkt, NULL) == SUCCESS)
        jmm_packet_free(pkt);

    jqueue_free(ctx->pkt_q);

    jfree(ctx);
}

/*
     0                   1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |V=2|P|X|  CC   |M|     PT      |       sequence number         |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                           timestamp                           |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |           synchronization source (SSRC) identifier            |
    +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
    |            contributing source (CSRC) identifiers             |
    |                             ....                              |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
int jmm_rtp_merge_write(jhandle h, uint8_t *buf, int size)
{
    if ((h==NULL) || (buf==NULL) || (size<=12))
        return ERROR_FAIL;

    jmm_rtp_merge_ctx *ctx = (jmm_rtp_merge_ctx *)h;

    uint8_t *payload = buf;
    int payload_size = size;

    int padding = (payload[0]&0x20) >> 5;
    int ext_header = (payload[0]&0x10) >> 4;
    int csrc_count = payload[0] & 0x0f;

    int64_t timestamp = (unsigned)((payload[4]<<24) | (payload[5]<<16) | (payload[6]<<8) | payload[7]);

    int seq = (payload[2]<<8) | payload[3];
    if (seq != (ctx->seq+1))
    {
        //jerr("[jmm_rtp_merge] seq uncontinue %d -> %d\n", ctx->seq, seq);
    }
    ctx->seq = seq;

    payload += 12;
    payload_size -= 12;

    payload += (csrc_count * 4);
    payload_size -= (csrc_count * 4);

    if (ext_header > 0)
    {
        int skip = ((payload[2]<<8)|payload[3]) * 4;
        payload += (skip + 4);
        payload_size -= (skip + 4);
    }

    if (padding > 0)
        payload_size -= buf[size-1];

    if (payload_size <= 0)
        return ERROR_FAIL;

    if (ctx->type == JMM_CODEC_TYPE_AAC)
    {
        int au_headers_size = (((payload[0]<<8)|payload[1])+7) / 8;
        jmm_packet *pkt = jmm_packet_alloc(payload_size-2-au_headers_size);
        pkt->type = JMM_CODEC_TYPE_AAC;
        pkt->fmt = JMM_BS_FMT_AAC_ES;
        pkt->key = jtrue;
        pkt->dts = timestamp;
        pkt->pts = timestamp;
        memcpy(pkt->data, &(payload[2+au_headers_size]), pkt->size);
        jqueue_push(ctx->pkt_q, (uint8_t *)pkt, sizeof(jmm_packet));
    }
    else
    {
        /*
            0:    undefined
            1~23: nalu
            24:   stap-a
            25:   stap-b
            26:   mtap16
            27:   mtap24
            28:   fu-a
            29:   fu-b
            30~31:undefined
        */
        int type = payload[0] & 0x1f;
        if ((type>=1) && (type<=23))
        {
            ctx->start_flag = jfalse;
            if (type == 7) //sps
            {
                ctx->packet_buf[0] = 0;
                ctx->packet_buf[1] = 0;
                ctx->packet_buf[2] = 0;
                ctx->packet_buf[3] = 1;
                memcpy(&(ctx->packet_buf[4]), payload, payload_size);
                ctx->packet_size = payload_size + 4;
                ctx->sps_flag = jtrue;
            }
            else if (type == 8) //pps
            {
                if (ctx->sps_flag && (ctx->packet_size > 0))
                {
                    jmm_packet *pkt = jmm_packet_alloc(ctx->packet_size + payload_size + 4);
                    pkt->type = JMM_CODEC_TYPE_AVC;
                    pkt->fmt = JMM_BS_FMT_AVC_ANNEXB;
                    pkt->key = jtrue;
                    pkt->dts = timestamp;
                    pkt->pts = timestamp;
                    memcpy(pkt->data, ctx->packet_buf, ctx->packet_size);
                    pkt->data[ctx->packet_size+0] = 0;
                    pkt->data[ctx->packet_size+1] = 0;
                    pkt->data[ctx->packet_size+2] = 0;
                    pkt->data[ctx->packet_size+3] = 1;
                    memcpy(&(pkt->data[ctx->packet_size+4]), payload, payload_size);
                    jmm_packet *tmp = pkt;
                    pkt = jmm_avc_annexb2avcc(pkt, jfalse);
                    if (pkt)
                        jqueue_push(ctx->pkt_q, (uint8_t *)pkt, sizeof(jmm_packet));
                    else
                        jmm_packet_free(tmp);
                }
                ctx->sps_flag = jfalse;
                ctx->packet_size = 0;
            }
            else
            {
                jmm_packet *pkt = jmm_packet_alloc(payload_size + 4);
                pkt->type = JMM_CODEC_TYPE_AVC;
                pkt->fmt = JMM_BS_FMT_AVC_ANNEXB;
                if (type == 5)
                    pkt->key = jtrue;
                else
                    pkt->key = jfalse;
                pkt->dts = timestamp;
                pkt->pts = timestamp;
                //0 0 0 1
                pkt->data[3] = 0x1;
                memcpy(&(pkt->data[4]), payload, payload_size);
                jqueue_push(ctx->pkt_q, (uint8_t *)pkt, sizeof(jmm_packet));
                ctx->sps_flag = jfalse;
                ctx->packet_size = 0;
            }
        }
        else if (type == 28)
        {
            ctx->sps_flag = jfalse;
            int start = (payload[1]&0x80) >> 7;
            int end = (payload[1]&0x40) >> 6;
            if (start > 0)
            {
                payload[1] = (payload[0]&0xe0) | (payload[1]&0x1f);
                ctx->packet_buf[0] = 0;
                ctx->packet_buf[1] = 0;
                ctx->packet_buf[2] = 0;
                ctx->packet_buf[3] = 1;
                memcpy(&(ctx->packet_buf[4]), &(payload[1]), payload_size-1);
                ctx->packet_size = payload_size + 4 - 1;
                ctx->start_flag = jtrue;
            }
            else if (end > 0)
            {
                if (ctx->start_flag)
                {
                    jmm_packet *pkt = jmm_packet_alloc(ctx->packet_size + payload_size - 2);
                    pkt->type = JMM_CODEC_TYPE_AVC;
                    pkt->fmt = JMM_BS_FMT_AVC_ANNEXB;
                    if (((ctx->packet_buf[4]&0x1f) == 5) || ((ctx->packet_buf[4]&0x1f) == 7))
                        pkt->key = jtrue;
                    else
                        pkt->key = jfalse;
                    pkt->dts = timestamp;
                    pkt->pts = timestamp;
                    memcpy(pkt->data, ctx->packet_buf, ctx->packet_size);
                    memcpy(&(pkt->data[ctx->packet_size]), &(payload[2]), payload_size-2);
                    jqueue_push(ctx->pkt_q, (uint8_t *)pkt, sizeof(jmm_packet));
                }
                ctx->start_flag = jfalse;
                ctx->packet_size = 0;
            }
            else
            {
                if (ctx->start_flag)
                {
                    if ((ctx->packet_size+payload_size-2) <= JMM_RTP_MERGE_BUF_SIZE)
                    {
                        memcpy(&(ctx->packet_buf[ctx->packet_size]), &(payload[2]), payload_size-2);
                        ctx->packet_size += (payload_size - 2);
                    }
                    else
                    {
                        jerr("[jmm_rtp_merge] buf size %d is not enough\n", JMM_RTP_MERGE_BUF_SIZE);
                    }
                }
                else
                    ctx->packet_size = 0;
            }
        }
        else
        {
            ctx->sps_flag = jfalse;
            ctx->start_flag = jfalse;
            ctx->packet_size = 0;
            jerr("[jmm_rtp_merge] unsupported avc rtp packet type: %d\n", type);
        }
    }

    return SUCCESS;
}

jmm_packet *jmm_rtp_merge_read(jhandle h)
{
    if (h == NULL)
        return NULL;

    jmm_rtp_merge_ctx *ctx = (jmm_rtp_merge_ctx *)h;

    jmm_packet *pkt;
    if (jqueue_pop(ctx->pkt_q, (uint8_t **)&pkt, NULL) == SUCCESS)
        return pkt;

    return NULL;
}

int jmm_rtp_merge_size(jhandle h)
{
    if (h == NULL)
        return -1;

    jmm_rtp_merge_ctx *ctx = (jmm_rtp_merge_ctx *)h;

    return jqueue_size(ctx->pkt_q);
}


