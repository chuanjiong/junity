/*
 * jmm_flv_muxer.c
 *
 * @chuanjiong
 */

#include "jmm_module.h"

/*
  F L V 1 a&v start

  tag-0-size

  tag-1

  tag-1-size

  ...

  tag-n

  tag-n-size
*/

typedef struct jmm_flv_muxer_ctx {
    jhandle wt;

    jbool asc_flg;
    jbool avcc_flg;
}jmm_flv_muxer_ctx;

static jhandle jmm_flv_muxer_open(const char *url)
{
    if (url == NULL)
        return NULL;

    jmm_flv_muxer_ctx *ctx = (jmm_flv_muxer_ctx *)jmalloc(sizeof(jmm_flv_muxer_ctx));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(jmm_flv_muxer_ctx));

    ctx->wt = jbufwriter_open(url, 0);
    if (ctx->wt == NULL)
    {
        jfree(ctx);
        return NULL;
    }

    uint8_t buf[16] = {'F', 'L', 'V', 1, 5, 0, 0, 0, 9, 0, 0, 0, 0, 0, 0, 0};

    jbufwriter_write(ctx->wt, buf, 13);

    return ctx;
}

static void jmm_flv_muxer_close(jhandle h)
{
    if (h == NULL)
        return;

    jmm_flv_muxer_ctx *ctx = (jmm_flv_muxer_ctx *)h;

    jbufwriter_close(ctx->wt);

    jfree(ctx);
}

static int jmm_flv_muxer_write(jhandle h, jmm_packet *packet)
{
    if ((h==NULL) || (packet==NULL))
        return ERROR_FAIL;

    jmm_flv_muxer_ctx *ctx = (jmm_flv_muxer_ctx *)h;

    //wait asc
    if (!ctx->asc_flg)
        if ((packet->type==JMM_CODEC_TYPE_AAC) && (packet->fmt!=JMM_BS_FMT_AAC_ASC))
        {
            //jwarn("[jmm_flv_muxer] drop aac pkt: %d\n", packet->fmt);
            return ERROR_FAIL;
        }

    //wait avcc
    if (!ctx->avcc_flg)
        if ((packet->type==JMM_CODEC_TYPE_AVC) && (packet->fmt!=JMM_BS_FMT_AVC_AVCC))
        {
            //jwarn("[jmm_flv_muxer] drop avc pkt: %d\n", packet->fmt);
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

    jbufwriter_write(ctx->wt, tag->data, tag->size);

    jmm_packet_free(tag);

    if (copy)
        jmm_packet_free(pkt);

    return SUCCESS;
}

const jmm_muxer jmm_flv_muxer = {
    jmm_flv_muxer_open,
    jmm_flv_muxer_close,
    jmm_flv_muxer_write
};


