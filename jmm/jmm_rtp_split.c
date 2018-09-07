/*
 * jmm_rtp_split.c
 *
 * @chuanjiong
 */

#include "jmm_rtp.h"

typedef struct jmm_rtp_split_ctx {
    jmm_rtp_split_cfg cfg;
    jhandle rtp_q;
}jmm_rtp_split_ctx;

jhandle jmm_rtp_split_open(jmm_rtp_split_cfg *cfg)
{
    if (cfg == NULL)
        return NULL;

    jmm_rtp_split_ctx *ctx = (jmm_rtp_split_ctx *)jmalloc(sizeof(jmm_rtp_split_ctx));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(jmm_rtp_split_ctx));

    ctx->cfg = *cfg;

    ctx->rtp_q = jqueue_alloc();

    return ctx;
}

void jmm_rtp_split_close(jhandle h)
{
    if (h == NULL)
        return;

    jmm_rtp_split_ctx *ctx = (jmm_rtp_split_ctx *)h;

    uint8_t *buf;
    while (jqueue_pop(ctx->rtp_q, &buf, NULL) == SUCCESS)
        jfree(buf);

    jqueue_free(ctx->rtp_q);

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
int jmm_rtp_split_write(jhandle h, jmm_packet *packet)
{
    if ((h==NULL) || (packet==NULL))
        return ERROR_FAIL;

    jmm_rtp_split_ctx *ctx = (jmm_rtp_split_ctx *)h;

    if (ctx->cfg.type != packet->type)
    {
        jerr("[jmm_rtp_split] codec type mismatch\n");
        return ERROR_FAIL;
    }

    if (packet->fmt == JMM_BS_FMT_AAC_ASC)
    {
        jwarn("[jmm_rtp_split] aac-asc invalid\n");
        return ERROR_FAIL;
    }

    jbool copy = jfalse;
    jmm_packet *pkt = packet;
    if ((packet->type==JMM_CODEC_TYPE_AAC) && (packet->fmt==JMM_BS_FMT_AAC_ADTS))
    {
        pkt = jmm_aac_adts2es(packet, jtrue);
        copy = jtrue;
    }
    else if ((packet->type==JMM_CODEC_TYPE_AVC) && (packet->fmt==JMM_BS_FMT_AVC_ANNEXB))
    {
        pkt = jmm_avc_annexb2mp4(packet, jtrue);
        copy = jtrue;
    }
    else if ((packet->type==JMM_CODEC_TYPE_AVC) && (packet->fmt==JMM_BS_FMT_AVC_AVCC))
    {
        pkt = jmm_avc_avcc2annexb(packet, jtrue);
        pkt = jmm_avc_annexb2mp4(pkt, jfalse);
        copy = jtrue;
    }

    if (pkt == NULL)
        return ERROR_FAIL;

    int left_size = 0;
    uint8_t *left = NULL;

    int payload_size = pkt->size;
    uint8_t *payload = pkt->data;

    if (pkt->type == JMM_CODEC_TYPE_AVC)
    {
        uint32_t one_nalu_size = (*payload<<24) | (*(payload+1)<<16) | (*(payload+2)<<8) | (*(payload+3));
        left_size = payload_size - one_nalu_size - 4;
        left = payload + one_nalu_size + 4;

        payload_size = one_nalu_size;
        payload += 4;
    }

    if (pkt->type == JMM_CODEC_TYPE_AAC)
    {
        uint8_t *data = (uint8_t *)jmalloc(payload_size+16);
        memset(data, 0, payload_size+16);

        uint8_t *header = data;
        header[0] = 0x80;
        header[1] = 0x80 | ctx->cfg.pt;
        header[2] = (ctx->cfg.seq>>8) & 0xff;
        header[3] = ctx->cfg.seq & 0xff;
        ctx->cfg.seq++;
        if (ctx->cfg.seq >= 65536)
            ctx->cfg.seq = 0;
        uint32_t ts = pkt->dts*ctx->cfg.timebase/1000000;
        header[4] = (ts>>24) & 0xff;
        header[5] = (ts>>16) & 0xff;
        header[6] = (ts>>8) & 0xff;
        header[7] = ts & 0xff;

        uint8_t *au = &(data[12]);
        au[0] = 0;
        au[1] = 16;
        au[2] = payload_size >> 5;
        au[3] = (payload_size&0x1f) << 3;

        memcpy(&(data[16]), payload, payload_size);

        jqueue_push(ctx->rtp_q, data, payload_size+16);
    }
    else
    {
        if (payload_size <= JMM_RTP_PAYLOAD_SIZE_MAX)
        {
            uint8_t *data = (uint8_t *)jmalloc(payload_size+12);
            memset(data, 0, payload_size+12);

            uint8_t *header = data;
            header[0] = 0x80;
            header[1] = 0x80 | ctx->cfg.pt;
            header[2] = (ctx->cfg.seq>>8) & 0xff;
            header[3] = ctx->cfg.seq & 0xff;
            ctx->cfg.seq++;
            if (ctx->cfg.seq >= 65536)
                ctx->cfg.seq = 0;
            uint32_t ts = pkt->dts*ctx->cfg.timebase/1000000;
            header[4] = (ts>>24) & 0xff;
            header[5] = (ts>>16) & 0xff;
            header[6] = (ts>>8) & 0xff;
            header[7] = ts & 0xff;

            memcpy(&(data[12]), payload, payload_size);

            jqueue_push(ctx->rtp_q, data, payload_size+12);
        }
        else
        {
            uint8_t fu[2] = {(*payload&0xe0)|28, (*payload&0x1f)|0x80};
            payload_size--;
            payload++;
            while (payload_size > 0)
            {
                int size = jmin(payload_size, JMM_RTP_PAYLOAD_SIZE_MAX-1);
                uint8_t *data = (uint8_t *)jmalloc(size+14);
                memset(data, 0, size+14);

                uint8_t *header = data;
                header[0] = 0x80;
                header[1] = 0x80 | ctx->cfg.pt;
                header[2] = (ctx->cfg.seq>>8) & 0xff;
                header[3] = ctx->cfg.seq & 0xff;
                ctx->cfg.seq++;
                if (ctx->cfg.seq >= 65536)
                    ctx->cfg.seq = 0;
                uint32_t ts = pkt->dts*ctx->cfg.timebase/1000000;
                header[4] = (ts>>24) & 0xff;
                header[5] = (ts>>16) & 0xff;
                header[6] = (ts>>8) & 0xff;
                header[7] = ts & 0xff;

                if (size == payload_size)
                    fu[1] |= 0x40;
                memcpy(&(data[12]), fu, 2);
                memcpy(&(data[14]), payload, size);

                jqueue_push(ctx->rtp_q, data, size+14);

                fu[1] &= 0x7f;
                payload_size -= size;
                payload += size;
            }
        }
    }

    if (left_size > 0)
    {
        jmm_packet l;
        l = *pkt;
        l.data = left;
        l.size = left_size;
        jmm_rtp_split_write(h, &l);
    }

    if (copy)
        jmm_packet_free(pkt);

    return SUCCESS;
}

int jmm_rtp_split_read(jhandle h, uint8_t **buf, int *size)
{
    if ((h==NULL) || (buf==NULL))
        return ERROR_FAIL;

    jmm_rtp_split_ctx *ctx = (jmm_rtp_split_ctx *)h;

    return jqueue_pop(ctx->rtp_q, buf, size);
}


