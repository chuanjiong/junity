/*
 * jmm_rtmp_muxer.c
 *
 * @chuanjiong
 */

#include "jmm_module.h"
#include "rtmp.h"

typedef struct jmm_rtmp_muxer_ctx {
    RTMP r;

    jbool asc_flg;
    jbool avcc_flg;
}jmm_rtmp_muxer_ctx;

static int jmm_rtmp_muxer_set_chunksize(RTMP *rtmp, int size)
{
    int ret = 0;
    RTMPPacket pack;
    RTMPPacket_Alloc(&pack, 4);
    pack.m_packetType = 1;//RTMP_PACKET_TYPE_CHUNK_SIZE;
    pack.m_nChannel = 0x02;
    pack.m_headerType = RTMP_PACKET_SIZE_LARGE;
    pack.m_nTimeStamp = 0;
    pack.m_nInfoField2 = 0;
    pack.m_nBodySize = 4;
    int nVal = size;
    pack.m_body[3] = nVal & 0xff;
    pack.m_body[2] = nVal >> 8;
    pack.m_body[1] = nVal >> 16;
    pack.m_body[0] = nVal >> 24;
    rtmp->m_outChunkSize = nVal;
    ret = RTMP_SendPacket(rtmp, &pack, 1);
    RTMPPacket_Free(&pack);
    return ret;
}

static jhandle jmm_rtmp_muxer_open(const char *url)
{
    if (url == NULL)
        return NULL;

    jmm_rtmp_muxer_ctx *ctx = (jmm_rtmp_muxer_ctx *)jmalloc(sizeof(jmm_rtmp_muxer_ctx));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(jmm_rtmp_muxer_ctx));

    RTMP_Init(&(ctx->r));

    if (!RTMP_SetupURL(&(ctx->r), (char *)url))
    {
        jfree(ctx);
        return NULL;
    }

    RTMP_EnableWrite(&(ctx->r));

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

    jmm_rtmp_muxer_set_chunksize(&(ctx->r), 1024*1024);

    return ctx;
}

static void jmm_rtmp_muxer_close(jhandle h)
{
    if (h == NULL)
        return;

    jmm_rtmp_muxer_ctx *ctx = (jmm_rtmp_muxer_ctx *)h;

    RTMP_Close(&(ctx->r));

    jfree(ctx);
}

static int jmm_rtmp_muxer_write(jhandle h, jmm_packet *packet)
{
    if ((h==NULL) || (packet==NULL))
        return ERROR_FAIL;

    jmm_rtmp_muxer_ctx *ctx = (jmm_rtmp_muxer_ctx *)h;

    //wait asc
    if (!ctx->asc_flg)
        if ((packet->type==JMM_CODEC_TYPE_AAC) && (packet->fmt!=JMM_BS_FMT_AAC_ASC))
        {
            //jwarn("[jmm_rtmp_muxer] drop aac pkt: %d\n", packet->fmt);
            return ERROR_FAIL;
        }

    //wait avcc
    if (!ctx->avcc_flg)
        if ((packet->type==JMM_CODEC_TYPE_AVC) && (packet->fmt!=JMM_BS_FMT_AVC_AVCC))
        {
            //jwarn("[jmm_rtmp_muxer] drop avc pkt: %d\n", packet->fmt);
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

    if (pkt == NULL)
        return ERROR_FAIL;

    if ((pkt->type==JMM_CODEC_TYPE_AAC) && (pkt->fmt==JMM_BS_FMT_AAC_ASC))
        ctx->asc_flg = jtrue;
    else if ((pkt->type==JMM_CODEC_TYPE_AVC) && (pkt->fmt==JMM_BS_FMT_AVC_AVCC))
        ctx->avcc_flg = jtrue;

    jmm_packet *tag = jmm_packet_flv_tag(pkt);
    if (tag == NULL)
    {
        if (copy)
            jmm_packet_free(pkt);
        return ERROR_FAIL;
    }

    RTMP_Write(&(ctx->r), (const char *)(tag->data), tag->size);

    jmm_packet_free(tag);

    if (copy)
        jmm_packet_free(pkt);

    return SUCCESS;
}

const jmm_muxer jmm_rtmp_muxer = {
    jmm_rtmp_muxer_open,
    jmm_rtmp_muxer_close,
    jmm_rtmp_muxer_write
};


