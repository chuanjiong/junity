/*
 * jmm_rtmp_demuxer.c
 *
 * @chuanjiong
 */

#include "jmm_module.h"
#include "rtmp.h"

typedef struct jmm_rtmp_demuxer_ctx {
    RTMP r;

    jmm_packet *asc;
    jmm_packet *avcc;

    jmm_asc_info ainfo;
}jmm_rtmp_demuxer_ctx;

static jhandle jmm_rtmp_demuxer_open(const char *url)
{
    if (url == NULL)
        return NULL;

    jmm_rtmp_demuxer_ctx *ctx = (jmm_rtmp_demuxer_ctx *)jmalloc(sizeof(jmm_rtmp_demuxer_ctx));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(jmm_rtmp_demuxer_ctx));

    RTMP_Init(&(ctx->r));

    if (!RTMP_SetupURL(&(ctx->r), (char *)url))
    {
        jfree(ctx);
        return NULL;
    }

    if (!RTMP_Connect(&(ctx->r), NULL))
    {
        jfree(ctx);
        return NULL;
    }

    if (!RTMP_ConnectStream(&(ctx->r), 0))
    {
        RTMP_Close(&(ctx->r));
        jfree(ctx);
        return NULL;
    }

    return ctx;
}

static void jmm_rtmp_demuxer_close(jhandle h)
{
    if (h == NULL)
        return;

    jmm_rtmp_demuxer_ctx *ctx = (jmm_rtmp_demuxer_ctx *)h;

    RTMP_Close(&(ctx->r));

    if (ctx->asc)
        jmm_packet_free(ctx->asc);
    if (ctx->avcc)
        jmm_packet_free(ctx->avcc);

    jfree(ctx);
}

static jmm_packet *jmm_rtmp_demuxer_extradata(jhandle h, jmm_codec_type type)
{
    if (h == NULL)
        return NULL;

    jmm_rtmp_demuxer_ctx *ctx = (jmm_rtmp_demuxer_ctx *)h;

    if (type == JMM_CODEC_TYPE_AAC)
        return jmm_packet_clone(ctx->asc);
    else
        return jmm_packet_clone(ctx->avcc);
}

static jmm_packet *jmm_rtmp_demuxer_read(jhandle h)
{
    if (h == NULL)
        return NULL;

    jmm_rtmp_demuxer_ctx *ctx = (jmm_rtmp_demuxer_ctx *)h;

    RTMPPacket packet = {0};
    jmm_packet *pkt = NULL;

    while (RTMP_IsConnected(&(ctx->r)))
    {
        if (RTMP_ReadPacket(&(ctx->r), &packet) && RTMPPacket_IsReady(&packet))
        {
            RTMP_ClientPacket(&(ctx->r), &packet);
            if (packet.m_packetType == RTMP_PACKET_TYPE_AUDIO)
            {
                uint32_t fmt = packet.m_body[0];
                if ((fmt&0xf0) != 0xa0)
                {
                    jwarn("[jmm_rtmp_demuxer] audio is not aac format\n");
                    RTMPPacket_Free(&packet);
                    return NULL;
                }
                uint32_t aac_pt = packet.m_body[1];
                if (aac_pt == 0) //asc
                {
                    pkt = jmm_packet_alloc(packet.m_nBodySize-2);
                    if (pkt == NULL)
                    {
                        RTMPPacket_Free(&packet);
                        return NULL;
                    }
                    pkt->type = JMM_CODEC_TYPE_AAC;
                    pkt->fmt = JMM_BS_FMT_AAC_ASC;
                    pkt->key = jtrue;
                    pkt->dts = packet.m_nTimeStamp * 1000;
                    pkt->pts = packet.m_nTimeStamp * 1000;
                    memcpy(pkt->data, &(packet.m_body[2]), pkt->size);
                    jmm_aac_asc_parse(pkt, &(ctx->ainfo));
                    if (ctx->asc)
                        jmm_packet_free(ctx->asc);
                    ctx->asc = jmm_packet_clone(pkt);
                }
                else
                {
                    pkt = jmm_packet_alloc(packet.m_nBodySize-2);
                    if (pkt == NULL)
                    {
                        RTMPPacket_Free(&packet);
                        return NULL;
                    }
                    pkt->type = JMM_CODEC_TYPE_AAC;
                    pkt->fmt = JMM_BS_FMT_AAC_ES;
                    pkt->key = jtrue;
                    pkt->dts = packet.m_nTimeStamp * 1000;
                    pkt->pts = packet.m_nTimeStamp * 1000;
                    memcpy(pkt->data, &(packet.m_body[2]), pkt->size);
                    jmm_packet *tmp = pkt;
                    pkt = jmm_aac_es2adts(pkt, &(ctx->ainfo), jfalse);
                    if (pkt == NULL)
                        jmm_packet_free(tmp);
                }
                RTMPPacket_Free(&packet);
                return pkt;
            }
            else if (packet.m_packetType == RTMP_PACKET_TYPE_VIDEO)
            {
                uint32_t fmt = packet.m_body[0];
                if ((fmt&0x0f) != 0x07)
                {
                    jwarn("[jmm_rtmp_demuxer] video is not avc format\n");
                    RTMPPacket_Free(&packet);
                    return NULL;
                }
                uint32_t avc_pt = packet.m_body[1];
                uint32_t ct = (packet.m_body[2]<<16) | (packet.m_body[3]<<8) | packet.m_body[4];
                if (avc_pt == 0) //avcc
                {
                    pkt = jmm_packet_alloc(packet.m_nBodySize-5);
                    if (pkt == NULL)
                    {
                        RTMPPacket_Free(&packet);
                        return NULL;
                    }
                    pkt->type = JMM_CODEC_TYPE_AVC;
                    pkt->fmt = JMM_BS_FMT_AVC_AVCC;
                    pkt->key = jtrue;
                    pkt->dts = packet.m_nTimeStamp * 1000;
                    pkt->pts = pkt->dts + ct*1000;
                    memcpy(pkt->data, &(packet.m_body[5]), pkt->size);
                    if (ctx->avcc)
                        jmm_packet_free(ctx->avcc);
                    ctx->avcc = jmm_packet_clone(pkt);
                }
                else
                {
                    pkt = jmm_packet_alloc(packet.m_nBodySize-5);
                    if (pkt == NULL)
                    {
                        RTMPPacket_Free(&packet);
                        return NULL;
                    }
                    pkt->type = JMM_CODEC_TYPE_AVC;
                    pkt->fmt = JMM_BS_FMT_AVC_MP4;
                    pkt->key = ((fmt&0xf0)==0x10)?jtrue:jfalse;
                    pkt->dts = packet.m_nTimeStamp * 1000;
                    pkt->pts = pkt->dts + ct*1000;
                    memcpy(pkt->data, &(packet.m_body[5]), pkt->size);
                }
                RTMPPacket_Free(&packet);
                return pkt;
            }
            RTMPPacket_Free(&packet);
        }
    }

    return pkt;
}

static int jmm_rtmp_demuxer_seek(jhandle h, int64_t ts)
{
    return ERROR_FAIL;
}

static int jmm_rtmp_demuxer_finfo(jhandle h, jmm_file_info *info)
{
    return ERROR_FAIL;
}

const jmm_demuxer jmm_rtmp_demuxer = {
    jmm_rtmp_demuxer_open,
    jmm_rtmp_demuxer_close,
    jmm_rtmp_demuxer_extradata,
    jmm_rtmp_demuxer_read,
    jmm_rtmp_demuxer_seek,
    jmm_rtmp_demuxer_finfo
};


