/*
 * jmm_ts_muxer.c
 *
 * @chuanjiong
 */

#include "jmm_module.h"

#define PAT_PMT_GAP             (10)

typedef struct jmm_ts_muxer_ctx {
    jhandle wt;

    int v_count;

    int pat_cc;
    int pmt_cc;
    int aac_cc;
    int avc_cc;

    jmm_asc_info ainfo;

    jbool asc_flg;
    jbool avcc_flg;
}jmm_ts_muxer_ctx;

static jhandle jmm_ts_muxer_open(const char *url)
{
    if (url == NULL)
        return NULL;

    jmm_ts_muxer_ctx *ctx = (jmm_ts_muxer_ctx *)jmalloc(sizeof(jmm_ts_muxer_ctx));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(jmm_ts_muxer_ctx));

    if (strstr(url, "udp://"))
        ctx->wt = jbufwriter_open(url, 188*7);
    else
        ctx->wt = jbufwriter_open(url, 0);
    if (ctx->wt == NULL)
    {
        jfree(ctx);
        return NULL;
    }

    return ctx;
}

static void jmm_ts_muxer_close(jhandle h)
{
    if (h == NULL)
        return;

    jmm_ts_muxer_ctx *ctx = (jmm_ts_muxer_ctx *)h;

    jbufwriter_close(ctx->wt);

    jfree(ctx);
}

/*

bits
8       sync_byte
1       transport_error_indicator
1       payload_unit_start_indicator
1       transport_priority
13      PID
2       transport_scrambling_control
2       adaptation_field_control
4       continuity_counter
if (adaptation_field_control == '10' || adaptation_field_control == '11') {
        adaptation_field()
}
if (adaptation_field_control == '01' || adaptation_field_control == '11') {
    for (i=0; i<N; i++) {
8       data_byte
    }
}

*/

/*

bits
8       table_id
1       section_syntax_indicator
1       '0'
2       reserved
12      section_length
16      transport_stream_id
2       reserved
5       version_number
1       current_next_indicator
8       section_number
8       last_section_number

for (i=0; i<N; i++) {
16      program_number
3       reserved
    if (program_number == '0') {
13      network_PID
    }
    else {
13      program_map_PID
    }
}

32      CRC_32

*/

static int jmm_ts_muxer_write_pat(jmm_ts_muxer_ctx *ctx)
{
    if (ctx == NULL)
        return ERROR_FAIL;

    uint8_t pat[21] = {0};

    //sync
    pat[0] = 0x47;

    pat[1] = 0x40;
    pat[2] = 0x00;

    pat[3] = 0x10;

    //cc
    pat[3] |= ((ctx->pat_cc++)&0xf);

    //pointer_field
    pat[4] = 0;

    //table_id
    pat[5] = 0;

    pat[6] = 0xb0;
    pat[7] = 0x0d;

    //transport_stream_id
    pat[8] = 0;
    pat[9] = 0x01;

    pat[10] = 0xc1;

    //section_number
    pat[11] = 0;
    pat[12] = 0;

    //program_number
    pat[13] = 0;
    pat[14] = 0x01;

    //PID 0x1000
    pat[15] = 0xf0;
    pat[16] = 0;

    //crc
    pat[17] = 0x2a;
    pat[18] = 0xb1;
    pat[19] = 0x04;
    pat[20] = 0xb2;

    jbufwriter_write(ctx->wt, pat, 21);

    jbufwriter_dump(ctx->wt, 188-21, 0xff);

    return SUCCESS;
}

/*

bits
8       table_id
1       section_syntax_indicator
1       '0'
2       reserved
12      section_length
16      program_number
2       reserved
5       version_number
1       current_next_indicator
8       section_number
8       last_section_number
3       reserved
13      PCR_PID
4       reserved
12      program_info_length

for (i=0; i<N; i++) {
        descriptor()
}

for (i=0; i<N1; i++) {
8       stream_type
3       reserved
13      elementary_PID
4       reserved
12      ES_info_length

    for (i=0; i<N2; i++) {
        descriptor()
    }
}

32      CRC_32

*/

static int jmm_ts_muxer_write_pmt(jmm_ts_muxer_ctx *ctx)
{
    if (ctx == NULL)
        return ERROR_FAIL;

    uint8_t pmt[37] = {0};

    //sync
    pmt[0] = 0x47;

    pmt[1] = 0x50;
    pmt[2] = 0x00;

    pmt[3] = 0x10;

    //cc
    pmt[3] |= ((ctx->pmt_cc++)&0xf);

    //pointer_field
    pmt[4] = 0;

    //table_id
    pmt[5] = 0x02;

    pmt[6] = 0xb0;
    pmt[7] = 0x1d;

    //program_number
    pmt[8] = 0;
    pmt[9] = 0x01;

    pmt[10] = 0xc1;

    //section_number
    pmt[11] = 0;
    pmt[12] = 0;

    //PCR_PID
    pmt[13] = 0xe1;
    pmt[14] = 0x00;

    pmt[15] = 0xf0;

    pmt[16] = 0;

    //avc
    //stream_type
    pmt[17] = 0x1b;

    //PID 0x100
    pmt[18] = 0xe1;
    pmt[19] = 0;

    pmt[20] = 0xf0;
    pmt[21] = 0;

    //aac
    //stream_type
    pmt[22] = 0x0f;

    //PID 0x101
    pmt[23] = 0xe1;
    pmt[24] = 0x01;

    pmt[25] = 0xf0;
    pmt[26] = 0x06;

    //descriptor
    pmt[27] = 0x0a;
    pmt[28] = 0x04;

    pmt[29] = 0x75;
    pmt[30] = 0x6e;
    pmt[31] = 0x64;

    pmt[32] = 0;

    //crc
    pmt[33] = 0x08;
    pmt[34] = 0x7d;
    pmt[35] = 0xe8;
    pmt[36] = 0x77;

    jbufwriter_write(ctx->wt, pmt, 37);

    jbufwriter_dump(ctx->wt, 188-37, 0xff);

    return SUCCESS;
}

static int jmm_ts_muxer_write_avc(jmm_ts_muxer_ctx *ctx, jmm_packet *packet)
{
    if ((ctx==NULL) || (packet==NULL))
        return ERROR_FAIL;

    //add pes header
    uint8_t pes[19] = {0};

    //packet_start_code_prefix
    pes[0] = 0;
    pes[1] = 0;
    pes[2] = 0x01;

    //stream_id
    pes[3] = 0xe0;

    //PES_packet_length
    if (packet->size > (65535-13))
    {
        pes[4] = 0;
        pes[5] = 0;
    }
    else
    {
        pes[4] = ((packet->size+13)>>8)&0xff;
        pes[5] = (packet->size+13) & 0xff;
    }

    pes[6] = 0x80;

    //pts & dts flag
    pes[7] = 0xc0;
    //pts & dts size
    pes[8] = 0x0a;

    //pts
    pes[9] = 0x31 | ((((packet->pts*9/100)>>30)&0x7)<<1);
    pes[10] = ((packet->pts*9/100)>>22) & 0xff;
    pes[11] = 0x01 | ((((packet->pts*9/100)>>15)&0x7f)<<1);
    pes[12] = ((packet->pts*9/100)>>7) & 0xff;
    pes[13] = 0x01 | (((packet->pts*9/100)&0x7f)<<1);

    //dts
    pes[14] = 0x11 | ((((packet->dts*9/100)>>30)&0x7)<<1);
    pes[15] = ((packet->dts*9/100)>>22) & 0xff;
    pes[16] = 0x01 | ((((packet->dts*9/100)>>15)&0x7f)<<1);
    pes[17] = ((packet->dts*9/100)>>7) & 0xff;
    pes[18] = 0x01 | (((packet->dts*9/100)&0x7f)<<1);

#if 0
    //add one nalu(0x9)
    pes[19] = 0;
    pes[20] = 0;
    pes[21] = 0;
    pes[22] = 0x01;
    pes[23] = 0x09;
    pes[24] = 0xf0;
#endif

    if (packet->key)
    {
        //convert to ts packet
        int total = 19 + packet->size;
        int wsize = 0;

        //first packet include PCR
        {
            uint8_t ts[12] = {0};
            int stuff_size;

            //sync
            ts[0] = 0x47;
            ts[1] = 0x40;

            //PID
            ts[1] |= 0x1;
            ts[2] = 0;

            ts[3] = 0x30 | ((ctx->avc_cc++)&0xf);

            if (total >= (188-4-8))
            {
                //=0 stuff
                stuff_size = 0;
            }
            else
            {
                //>0 stuff
                stuff_size = 176 - total;
            }

            //adaptation_field_length
            ts[4] = 0x07+stuff_size;
            ts[5] = 0x10;
            //PCR
            ts[6] = ((packet->dts*9/100)>>25) & 0xff;
            ts[7] = ((packet->dts*9/100)>>17) & 0xff;
            ts[8] = ((packet->dts*9/100)>>9) & 0xff;
            ts[9] = ((packet->dts*9/100)>>1) & 0xff;
            ts[10] = (((packet->dts*9/100)&0x1)<<7) | 0x7e;
            ts[11] = 0;

            jbufwriter_write(ctx->wt, ts, 12);

            //stuff
            if (stuff_size > 0)
                jbufwriter_dump(ctx->wt, stuff_size, 0xff);

            //pes header
            jbufwriter_write(ctx->wt, pes, 19);

            //avc data
            jbufwriter_write(ctx->wt, &(packet->data[wsize]), 188-12-19-stuff_size);

            wsize += (188-12-19-stuff_size);
            total -= (188-12-stuff_size);
        }

        while (total >= 184)
        {
            uint8_t ts[4] = {0};

            //sync
            ts[0] = 0x47;
            ts[1] = 0;

            //PID
            ts[1] |= 0x1;
            ts[2] = 0;

            ts[3] = 0x10 | ((ctx->avc_cc++)&0xf);

            jbufwriter_write(ctx->wt, ts, 4);

            jbufwriter_write(ctx->wt, &(packet->data[wsize]), 188-4);

            wsize += 184;
            total -= 184;
        }

        if (total > 0)
        {
            uint8_t ts[4] = {0};

            //sync
            ts[0] = 0x47;
            ts[1] = 0;

            //PID
            ts[1] |= 0x01;
            ts[2] = 0;

            ts[3] = 0x30 | ((ctx->avc_cc++)&0xf);

            jbufwriter_write(ctx->wt, ts, 4);

            uint8_t af_size = 183-total;
            jbufwriter_dump(ctx->wt, 1, af_size);
            if (af_size > 0)
            {
                //flag
                jbufwriter_dump(ctx->wt, 1, 0);
                af_size--;
            }
            if (af_size > 0)
            {
                jbufwriter_dump(ctx->wt, af_size, 0xff);
            }

            jbufwriter_write(ctx->wt, &(packet->data[wsize]), total);
        }
    }
    else
    {
        //convert to ts packet
        jbool first = jtrue;
        int total = 19 + packet->size;
        int wsize = 0;
        while (total >= 184)
        {
            uint8_t ts[4] = {0};

            //sync
            ts[0] = 0x47;

            if (first)
            {
                ts[1] = 0x40;
            }
            else
            {
                ts[1] = 0;
            }

            //PID
            ts[1] |= 0x1;
            ts[2] = 0;

            ts[3] = 0x10 | ((ctx->avc_cc++)&0xf);

            jbufwriter_write(ctx->wt, ts, 4);

            if (first)
            {
                jbufwriter_write(ctx->wt, pes, 19);
                jbufwriter_write(ctx->wt, &(packet->data[wsize]), 188-4-19);
                wsize += (188-4-19);
                first = jfalse;
            }
            else
            {
                jbufwriter_write(ctx->wt, &(packet->data[wsize]), 188-4);
                wsize += 184;
            }

            total -= 184;
        }

        if (total > 0)
        {
            uint8_t ts[4] = {0};

            //sync
            ts[0] = 0x47;

            if (first)
            {
                ts[1] = 0x40;
            }
            else
            {
                ts[1] = 0;
            }

            //PID
            ts[1] |= 0x01;
            ts[2] = 0;

            ts[3] = 0x30 | ((ctx->avc_cc++)&0xf);

            jbufwriter_write(ctx->wt, ts, 4);

            uint8_t af_size = 183-total;
            jbufwriter_dump(ctx->wt, 1, af_size);
            if (af_size > 0)
            {
                jbufwriter_dump(ctx->wt, 1, 0);
                af_size--;
            }
            if (af_size > 0)
            {
                jbufwriter_dump(ctx->wt, af_size, 0xff);
            }

            if (first)
            {
                jbufwriter_write(ctx->wt, pes, 19);
                jbufwriter_write(ctx->wt, &(packet->data[wsize]), total-19);
            }
            else
            {
                jbufwriter_write(ctx->wt, &(packet->data[wsize]), total);
            }
        }
    }

    return SUCCESS;
}

static int jmm_ts_muxer_write_aac(jmm_ts_muxer_ctx *ctx, jmm_packet *packet)
{
    if ((ctx==NULL) || (packet==NULL))
        return ERROR_FAIL;

    //add pes header
    uint8_t pes[14] = {0};

    //packet_start_code_prefix
    pes[0] = 0;
    pes[1] = 0;
    pes[2] = 0x01;

    //stream_id
    pes[3] = 0xc0;

    //PES_packet_length
    pes[4] = ((packet->size+8)>>8)&0xff;
    pes[5] = (packet->size+8) & 0xff;

    pes[6] = 0x80;

    //pts & dts flag
    pes[7] = 0x80;
    //pts & dts size
    pes[8] = 0x05;

    //pts
    pes[9] = 0x21 | ((((packet->pts*9/100)>>30)&0x7)<<1);
    pes[10] = ((packet->pts*9/100)>>22) & 0xff;
    pes[11] = 0x01 | ((((packet->pts*9/100)>>15)&0x7f)<<1);
    pes[12] = ((packet->pts*9/100)>>7) & 0xff;
    pes[13] = 0x01 | (((packet->pts*9/100)&0x7f)<<1);

    //convert to ts packet
    jbool first = jtrue;
    int total = 14 + packet->size;
    int wsize = 0;
    while (total >= 184)
    {
        uint8_t ts[4] = {0};

        //sync
        ts[0] = 0x47;

        if (first)
        {
            ts[1] = 0x40;
        }
        else
        {
            ts[1] = 0;
        }

        //PID
        ts[1] |= 0x1;
        ts[2] = 0x01;

        ts[3] = 0x10 | ((ctx->aac_cc++)&0xf);

        jbufwriter_write(ctx->wt, ts, 4);

        if (first)
        {
            jbufwriter_write(ctx->wt, pes, 14);
            jbufwriter_write(ctx->wt, &(packet->data[wsize]), 188-4-14);
            wsize += (188-4-14);
            first = jfalse;
        }
        else
        {
            jbufwriter_write(ctx->wt, &(packet->data[wsize]), 188-4);
            wsize += 184;
        }

        total -= 184;
    }

    if (total > 0)
    {
        uint8_t ts[4] = {0};

        //sync
        ts[0] = 0x47;

        if (first)
        {
            ts[1] = 0x40;
        }
        else
        {
            ts[1] = 0;
        }

        //PID
        ts[1] |= 0x01;
        ts[2] = 0x01;

        ts[3] = 0x30 | ((ctx->aac_cc++)&0xf);

        jbufwriter_write(ctx->wt, ts, 4);

        uint8_t af_size = 183-total;
        jbufwriter_dump(ctx->wt, 1, af_size);
        if (af_size > 0)
        {
            jbufwriter_dump(ctx->wt, 1, 0);
            af_size--;
        }
        if (af_size > 0)
        {
            jbufwriter_dump(ctx->wt, af_size, 0xff);
        }

        if (first)
        {
            jbufwriter_write(ctx->wt, pes, 14);
            jbufwriter_write(ctx->wt, &(packet->data[wsize]), total-14);
        }
        else
        {
            jbufwriter_write(ctx->wt, &(packet->data[wsize]), total);
        }
    }

    return SUCCESS;
}

static int jmm_ts_muxer_write(jhandle h, jmm_packet *packet)
{
    if ((h==NULL) || (packet==NULL))
        return ERROR_FAIL;

    jmm_ts_muxer_ctx *ctx = (jmm_ts_muxer_ctx *)h;

    //wait asc
    if (!ctx->asc_flg)
        if ((packet->type==JMM_CODEC_TYPE_AAC) && (packet->fmt!=JMM_BS_FMT_AAC_ASC))
        {
            //jwarn("[jmm_ts_muxer] drop aac pkt: %d\n", packet->fmt);
            return ERROR_FAIL;
        }

    //wait avcc
    if (!ctx->avcc_flg)
        if ((packet->type==JMM_CODEC_TYPE_AVC) && (packet->fmt!=JMM_BS_FMT_AVC_AVCC))
        {
            //jwarn("[jmm_ts_muxer] drop avc pkt: %d\n", packet->fmt);
            return ERROR_FAIL;
        }

    if ((packet->type==JMM_CODEC_TYPE_AAC) && (packet->fmt==JMM_BS_FMT_AAC_ASC))
    {
        jmm_aac_asc_parse(packet, &(ctx->ainfo));
        ctx->asc_flg = jtrue;
        return SUCCESS;
    }

    if (packet->type == JMM_CODEC_TYPE_AVC)
    {
        if (ctx->v_count == 0)
        {
            jmm_ts_muxer_write_pat(ctx);
            jmm_ts_muxer_write_pmt(ctx);
        }
        ctx->v_count++;
        if (ctx->v_count >= PAT_PMT_GAP)
            ctx->v_count = 0;
    }

    jbool copy = jfalse;
    jmm_packet *pkt = packet;
    if ((packet->type==JMM_CODEC_TYPE_AAC) && (packet->fmt==JMM_BS_FMT_AAC_ES))
    {
        pkt = jmm_aac_es2adts(packet, &(ctx->ainfo), jtrue);
        copy = jtrue;
    }
    else if ((packet->type==JMM_CODEC_TYPE_AVC) && (packet->fmt==JMM_BS_FMT_AVC_AVCC))
    {
        ctx->avcc_flg = jtrue;
        pkt = jmm_avc_avcc2annexb(packet, jtrue);
        copy = jtrue;
    }
    else if ((packet->type==JMM_CODEC_TYPE_AVC) && (packet->fmt==JMM_BS_FMT_AVC_MP4))
    {
        pkt = jmm_avc_mp42annexb(packet, jtrue);
        copy = jtrue;
    }

    if (pkt == NULL)
        return ERROR_FAIL;

    if ((pkt->type==JMM_CODEC_TYPE_AAC) && (pkt->fmt==JMM_BS_FMT_AAC_ES))
        jmm_ts_muxer_write_aac(ctx, pkt);
    else if ((pkt->type==JMM_CODEC_TYPE_AVC) && (pkt->fmt==JMM_BS_FMT_AVC_ANNEXB))
        jmm_ts_muxer_write_avc(ctx, pkt);

    if (copy)
        jmm_packet_free(pkt);

    return SUCCESS;
}

const jmm_muxer jmm_ts_muxer = {
    jmm_ts_muxer_open,
    jmm_ts_muxer_close,
    jmm_ts_muxer_write
};


