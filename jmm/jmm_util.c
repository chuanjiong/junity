/*
 * jmm_util.c
 *
 * @chuanjiong
 */

#include "jmm_util.h"

static const char *type2str[] = {
    "aac",
    "avc"
};

static const char *fmt2str[] = {
    "asc",
    "es",
    "adts",

    "avcc",
    "annexb",
    "mp4"
};

// packet

jmm_packet *jmm_packet_alloc(int size)
{
    if (size < 0)
        return NULL;

    jmm_packet *pkt = (jmm_packet *)jmalloc(sizeof(jmm_packet));
    if (pkt == NULL)
        return NULL;
    memset(pkt, 0, sizeof(jmm_packet));

    if (size > 0)
    {
        pkt->size = size;
        pkt->data = (uint8_t *)jmalloc(pkt->size);
        if (pkt->data == NULL)
        {
            jfree(pkt);
            return NULL;
        }
        memset(pkt->data, 0, pkt->size);
    }

    return pkt;
}

void jmm_packet_free(jmm_packet *packet)
{
    if (packet == NULL)
        return;

    if (packet->size > 0)
        jfree(packet->data);

    jfree(packet);
}

jmm_packet *jmm_packet_clone(jmm_packet *packet)
{
    if (packet == NULL)
        return NULL;

    jmm_packet *pkt = jmm_packet_alloc(packet->size);
    if (pkt == NULL)
        return NULL;

    pkt->type = packet->type;
    pkt->fmt = packet->fmt;
    pkt->key = packet->key;
    pkt->dts = packet->dts;
    pkt->pts = packet->pts;

    memcpy(pkt->data, packet->data, pkt->size);

    return pkt;
}

// aac

/*
    -- asc --

bits
5   audioObjectType(if aot == 31, flow 6-bits aot_ext)
4   samplingFrequencyIndex(if sfi == 15, flow 24-bits sf)
4   channelConfiguration
    (if aot == 5, ...)
(aot == 1,2,3,4..., GASpecificConfig)
1   frameLengthFlag
1   dependsOnCoreCoder(if docc == 1, flow 14-bits coreCoderDelay)
1   extensionFlag
    (if cc == 0, program_config_element)
    (if aot == 5,20, 3-bits layerNr)
    (if extensionFlag == 1, ...)
*/

static const int jmm_aac_sr[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350};
static const int jmm_aac_ch[] = {0, 1, 2, 3, 4, 5, 5+1, 7+1};

int jmm_aac_asc_parse(jmm_packet *packet, jmm_asc_info *info)
{
    if ((packet==NULL) || (info==NULL))
        return ERROR_FAIL;

    if ((packet->type!=JMM_CODEC_TYPE_AAC) || (packet->fmt!=JMM_BS_FMT_AAC_ASC))
    {
        jerr("[jmm_util] jmm_aac_asc_parse invalid type %s, or fmt %s\n", type2str[packet->type], fmt2str[packet->fmt]);
        return ERROR_FAIL;
    }

    //aot 1:aac-main, 2:aac-lc, 3:aac-ssr, 4:aac-ltp
    uint32_t aot = (packet->data[0]&0xf8) >> 3;
    if ((aot<1) || (aot>4))
        return ERROR_FAIL;
    info->profile = aot;

    uint32_t sfi = ((packet->data[0]&0x7)<<1) | ((packet->data[1]>>7)&0x1);
    if (sfi > 12)
        return ERROR_FAIL;
    info->samplerate = jmm_aac_sr[sfi];

    uint32_t cc = (packet->data[1]>>3) & 0xf;
    if ((cc<1) || (cc>7))
        return ERROR_FAIL;
    info->channels = jmm_aac_ch[cc];

    return SUCCESS;
}

/*
    -- adts --

bits
(adts_fixed_header)
12  syncword (0xfff)
1   ID (0:mpeg-4 1:mpeg-2)
2   layer
1   protection_absent
2   profile_ObjectType
4   sampling_frequency_index
1   private_bit
3   channel_configuration
1   original_copy
1   home

(adts_variable_header)
1   copyright_identification_bit
1   copyright_identification_start
13  aac_frame_length
11  adts_buffer_fullness
2   number_of_raw_data_blocks_in_frame
*/

jmm_packet *jmm_aac_adts2es(jmm_packet *packet, jbool copy)
{
    if (packet == NULL)
        return NULL;

    if ((packet->type!=JMM_CODEC_TYPE_AAC) || (packet->fmt!=JMM_BS_FMT_AAC_ADTS))
    {
        jerr("[jmm_util] jmm_aac_adts2es invalid type %s, or fmt %s\n", type2str[packet->type], fmt2str[packet->fmt]);
        return NULL;
    }

    uint8_t *data = (uint8_t *)jmalloc(packet->size-7);
    if (data == NULL)
        return NULL;
    memset(data, 0, packet->size-7);

    memcpy(data, &(packet->data[7]), packet->size-7);

    jmm_packet *pkt;
    if (copy)
    {
        pkt = jmm_packet_alloc(0);
        if (pkt == NULL)
        {
            jfree(data);
            return NULL;
        }
        pkt->type = JMM_CODEC_TYPE_AAC;
        pkt->fmt = JMM_BS_FMT_AAC_ES;
        pkt->key = packet->key;
        pkt->dts = packet->dts;
        pkt->pts = packet->pts;
        pkt->size = packet->size-7;
        pkt->data = data;
    }
    else
    {
        pkt = packet;
        pkt->fmt = JMM_BS_FMT_AAC_ES;
        jfree(pkt->data);
        pkt->size -= 7;
        pkt->data = data;
    }

    return pkt;
}

jmm_packet *jmm_aac_es2adts(jmm_packet *packet, jmm_asc_info *info, jbool copy)
{
    if ((packet==NULL) || (info==NULL))
        return NULL;

    if ((packet->type!=JMM_CODEC_TYPE_AAC) || (packet->fmt!=JMM_BS_FMT_AAC_ES))
    {
        jerr("[jmm_util] jmm_aac_es2adts invalid type %s, or fmt %s\n", type2str[packet->type], fmt2str[packet->fmt]);
        return NULL;
    }

    uint8_t *data = (uint8_t *)jmalloc(packet->size+7);
    if (data == NULL)
        return NULL;
    memset(data, 0, packet->size+7);

    int i;
    int sfi, cc;
    for (i=0; i<jarray_size(jmm_aac_sr); i++)
        if (info->samplerate == jmm_aac_sr[i])
            break;
    if (i != jarray_size(jmm_aac_sr))
        sfi = i;
    else
        sfi = 4; //44100

    for (i=0; i<jarray_size(jmm_aac_ch); i++)
        if (info->channels == jmm_aac_ch[i])
            break;
    if (i != jarray_size(jmm_aac_ch))
        cc = i;
    else
        cc = 2; //2

    //adts header
    data[0] = 0xff;
    data[1] = 0xf1;
    data[2] = ((info->profile-1)<<6) | ((sfi&0xf)<<2) | ((cc>>2)&0x1);
    data[3] = (cc<<6) | (((packet->size+7)>>11)&0x3);
    data[4] = ((packet->size+7)>>3) & 0xff;
    data[5] = ((packet->size+7)<<5) | 0x1f;
    data[6] = 0x3f << 2;

    memcpy(&(data[7]), packet->data, packet->size);

    jmm_packet *pkt;
    if (copy)
    {
        pkt = jmm_packet_alloc(0);
        if (pkt == NULL)
        {
            jfree(data);
            return NULL;
        }
        pkt->type = JMM_CODEC_TYPE_AAC;
        pkt->fmt = JMM_BS_FMT_AAC_ADTS;
        pkt->key = packet->key;
        pkt->dts = packet->dts;
        pkt->pts = packet->pts;
        pkt->size = packet->size+7;
        pkt->data = data;
    }
    else
    {
        pkt = packet;
        pkt->fmt = JMM_BS_FMT_AAC_ADTS;
        jfree(pkt->data);
        pkt->size += 7;
        pkt->data = data;
    }

    return pkt;
}

// avc

/*
    leadingZeroBits = -1
    for (b=0; !b; leadingZeroBits++)
        b = read_bits(1)
    codeNum = 2^leadingZeroBits-1 + read_bits(leadingZeroBits)
*/
static uint32_t jmm_read_ExpGolombCode(jhandle rd)
{
    int r = 0;
    int i = 0;

    while ((jbitreader_read(rd, 1)==0) && (i<32))
        i++;

    r = jbitreader_read(rd, i);
    r += (1 << i) - 1;

    return r;
}

static int32_t jmm_read_signed_ExpGolombCode(jhandle rd)
{
    int r = jmm_read_ExpGolombCode(rd);

    if (r & 0x01)
        r = (r+1)/2;
    else
        r = -(r/2);

    return r;
}

/*
    -- avcc --

Byte    bits
0       8   version (always 0x01)
1       8   avc profile (sps[0][1])
2       8   avc compatibility (sps[0][2])
3       8   avc level (sps[0][3])
4       6   reserved (all bits on)
        2   NALULengthSizeMinusOne
5       3   reserved (all bits on)
        5   number of SPS NALUs (usually 1)
        repeated once per SPS:
67        16        SPS size
          variable  SPS NALU data
8+s     8   number of PPS NALUs (usually 1)
        repeated once per PPS
90+s      16        PPS size
          variable  PPS NALU data
*/

int jmm_avc_avcc_parse(jmm_packet *packet, jmm_avcc_info *info)
{
    if ((packet==NULL) || (info==NULL))
        return ERROR_FAIL;

    if ((packet->type!=JMM_CODEC_TYPE_AVC) || (packet->fmt!=JMM_BS_FMT_AVC_AVCC))
    {
        jerr("[jmm_util] jmm_avc_avcc_parse invalid type %s, or fmt %s\n", type2str[packet->type], fmt2str[packet->fmt]);
        return ERROR_FAIL;
    }

    int nalulengthsize = (packet->data[4]&0x3) + 1;
    if (nalulengthsize != 4)
    {
        jerr("[jmm_util] nalulengthsize != 4\n");
        return ERROR_FAIL;
    }

    //skip 67
    jhandle rd = jbitreader_alloc(&(packet->data[9]), ((packet->data[6]<<8)|packet->data[7])-1);

    //copy from http://stackoverflow.com/questions/12018535/get-the-width-height-of-the-video-from-h-264-nalu/27636670#27636670
    int frame_crop_left_offset=0;
    int frame_crop_right_offset=0;
    int frame_crop_top_offset=0;
    int frame_crop_bottom_offset=0;

    int profile_idc = jbitreader_read(rd, 8);
    jbitreader_read(rd, 1); //constraint_set0_flag
    jbitreader_read(rd, 1); //constraint_set1_flag
    jbitreader_read(rd, 1); //constraint_set2_flag
    jbitreader_read(rd, 1); //constraint_set3_flag
    jbitreader_read(rd, 1); //constraint_set4_flag
    jbitreader_read(rd, 1); //constraint_set5_flag
    jbitreader_read(rd, 2); //reserved_zero_2bits
    jbitreader_read(rd, 8); //level_idc
    jmm_read_ExpGolombCode(rd);  //seq_parameter_set_id

    if (profile_idc == 100 || profile_idc == 110 ||
        profile_idc == 122 || profile_idc == 244 ||
        profile_idc == 44 || profile_idc == 83 ||
        profile_idc == 86 || profile_idc == 118)
    {
        int chroma_format_idc = jmm_read_ExpGolombCode(rd);
        if (chroma_format_idc == 3)
            jbitreader_read(rd, 1); //residual_colour_transform_flag

        jmm_read_ExpGolombCode(rd);  //bit_depth_luma_minus8
        jmm_read_ExpGolombCode(rd);  //bit_depth_chroma_minus8
        jbitreader_read(rd, 1); //qpprime_y_zero_transform_bypass_flag

        int seq_scaling_matrix_present_flag = jbitreader_read(rd, 1);
        if (seq_scaling_matrix_present_flag)
        {
            int i=0;
            for (i=0; i<8; i++)
            {
                int seq_scaling_list_present_flag = jbitreader_read(rd, 1);
                if (seq_scaling_list_present_flag)
                {
                    int sizeOfScalingList = (i<6) ? 16 : 64;
                    int lastScale = 8;
                    int nextScale = 8;
                    int j;
                    for (j=0; j<sizeOfScalingList; j++)
                    {
                        if (nextScale != 0)
                        {
                            int delta_scale = jmm_read_signed_ExpGolombCode(rd);
                            nextScale = (lastScale + delta_scale + 256) % 256;
                        }
                        lastScale = (nextScale==0) ? lastScale : nextScale;
                    }
                }
            }
        }
    }

    jmm_read_ExpGolombCode(rd); //log2_max_frame_num_minus4
    int pic_order_cnt_type = jmm_read_ExpGolombCode(rd);
    if (pic_order_cnt_type == 0)
    {
        jmm_read_ExpGolombCode(rd); //log2_max_pic_order_cnt_lsb_minus4
    }
    else if (pic_order_cnt_type == 1)
    {
        jbitreader_read(rd, 1);      //delta_pic_order_always_zero_flag
        jmm_read_signed_ExpGolombCode(rd); //offset_for_non_ref_pic
        jmm_read_signed_ExpGolombCode(rd); //offset_for_top_to_bottom_field
        int num_ref_frames_in_pic_order_cnt_cycle = jmm_read_ExpGolombCode(rd);
        int i;
        for(i=0; i<num_ref_frames_in_pic_order_cnt_cycle; i++)
            jmm_read_signed_ExpGolombCode(rd);
    }

    jmm_read_ExpGolombCode(rd);  //max_num_ref_frames
    jbitreader_read(rd, 1); //gaps_in_frame_num_value_allowed_flag
    int pic_width_in_mbs_minus1 = jmm_read_ExpGolombCode(rd);
    int pic_height_in_map_units_minus1 = jmm_read_ExpGolombCode(rd);

    int frame_mbs_only_flag = jbitreader_read(rd, 1);
    if (!frame_mbs_only_flag)
        jbitreader_read(rd, 1); //mb_adaptive_frame_field_flag

    jbitreader_read(rd, 1); //direct_8x8_inference_flag

    int frame_cropping_flag = jbitreader_read(rd, 1);
    if (frame_cropping_flag)
    {
        frame_crop_left_offset = jmm_read_ExpGolombCode(rd);
        frame_crop_right_offset = jmm_read_ExpGolombCode(rd);
        frame_crop_top_offset = jmm_read_ExpGolombCode(rd);
        frame_crop_bottom_offset = jmm_read_ExpGolombCode(rd);
    }

    jbitreader_read(rd, 1); //vui_parameters_present_flag

    info->width = ((pic_width_in_mbs_minus1+1)*16) - frame_crop_bottom_offset*2 - frame_crop_top_offset*2;
    info->height = ((2-frame_mbs_only_flag)*(pic_height_in_map_units_minus1+1)*16) - frame_crop_right_offset*2 - frame_crop_left_offset*2;

    jbitreader_free(rd);

    return SUCCESS;
}

jmm_packet *jmm_avc_avcc2annexb(jmm_packet *packet, jbool copy)
{
    if (packet == NULL)
        return NULL;

    if ((packet->type!=JMM_CODEC_TYPE_AVC) || (packet->fmt!=JMM_BS_FMT_AVC_AVCC))
    {
        jerr("[jmm_util] jmm_avc_avcc2annexb invalid type %s, or fmt %s\n", type2str[packet->type], fmt2str[packet->fmt]);
        return NULL;
    }

    //assume number of sps NALUs is 1
    uint32_t sps_num = packet->data[5] & 0x1f;
    if (sps_num != 1)
        return NULL;
    uint32_t sps_size = (packet->data[6]<<8) | packet->data[7];

    //assume number of pps NALUs is 1
    uint32_t pps_num = packet->data[8+sps_size];
    if (pps_num != 1)
        return NULL;
    uint32_t pps_size = (packet->data[9+sps_size]<<8) | packet->data[10+sps_size];

    //0 0 0 1 sps 0 0 0 1 pps
    uint8_t *data = (uint8_t *)jmalloc(sps_size+pps_size+4+4);
    if (data == NULL)
        return NULL;
    memset(data, 0, sps_size+pps_size+4+4);

    //0 0 0 1 sps
    data[3] = 0x1;
    memcpy(&(data[4]), &(packet->data[8]), sps_size);

    //0 0 0 1 pps
    data[7+sps_size] = 0x1;
    memcpy(&(data[8+sps_size]), &(packet->data[11+sps_size]), pps_size);

    jmm_packet *pkt;
    if (copy)
    {
        pkt = jmm_packet_alloc(0);
        if (pkt == NULL)
        {
            jfree(data);
            return NULL;
        }
        pkt->type = JMM_CODEC_TYPE_AVC;
        pkt->fmt = JMM_BS_FMT_AVC_ANNEXB;
        pkt->key = packet->key;
        pkt->dts = packet->dts;
        pkt->pts = packet->pts;
        pkt->size = sps_size+pps_size+4+4;
        pkt->data = data;
    }
    else
    {
        pkt = packet;
        pkt->fmt = JMM_BS_FMT_AVC_ANNEXB;
        jfree(pkt->data);
        pkt->size = sps_size+pps_size+4+4;
        pkt->data = data;
    }

    return pkt;
}

jmm_packet *jmm_avc_annexb2avcc(jmm_packet *packet, jbool copy)
{
    if (packet == NULL)
        return NULL;

    if ((packet->type!=JMM_CODEC_TYPE_AVC) || (packet->fmt!=JMM_BS_FMT_AVC_ANNEXB))
    {
        jerr("[jmm_util] jmm_avc_annexb2avcc invalid type %s, or fmt %s\n", type2str[packet->type], fmt2str[packet->fmt]);
        return NULL;
    }

    //assume packet: 0 0 0 1 67 ... 0 0 0 1 68 ...
    int idx = 5;
    while (idx < (packet->size-3))
    {
        if ((packet->data[idx]==0) && (packet->data[idx+1]==0) && (packet->data[idx+2]==0) && (packet->data[idx+3]==1))
            break;
        idx++;
    }

    int sps_size = idx-4; //skip 0 0 0 1
    int pps_size = packet->size-sps_size-4-4;

    uint8_t *data = (uint8_t *)jmalloc(sps_size+pps_size+11);
    if (data == NULL)
        return NULL;
    memset(data, 0, sps_size+pps_size+11);

    data[0] = 0x1;
    data[1] = packet->data[5];
    data[2] = packet->data[6];
    data[3] = packet->data[7];
    data[4] = 0xff;
    data[5] = 0xe1;
    data[6] = (sps_size>>8) & 0xff;
    data[7] = sps_size & 0xff;
    memcpy(&(data[8]), &(packet->data[4]), sps_size);
    data[8+sps_size] = 0x1;
    data[9+sps_size] = (pps_size>>8) & 0xff;
    data[10+sps_size] = pps_size & 0xff;
    memcpy(&(data[11+sps_size]), &(packet->data[sps_size+8]), pps_size);

    jmm_packet *pkt;
    if (copy)
    {
        pkt = jmm_packet_alloc(0);
        if (pkt == NULL)
        {
            jfree(data);
            return NULL;
        }
        pkt->type = JMM_CODEC_TYPE_AVC;
        pkt->fmt = JMM_BS_FMT_AVC_AVCC;
        pkt->key = packet->key;
        pkt->dts = packet->dts;
        pkt->pts = packet->pts;
        pkt->size = sps_size+pps_size+11;
        pkt->data = data;
    }
    else
    {
        pkt = packet;
        pkt->fmt = JMM_BS_FMT_AVC_AVCC;
        jfree(pkt->data);
        pkt->size = sps_size+pps_size+11;
        pkt->data = data;
    }

    return pkt;
}

jmm_packet *jmm_avc_mp42annexb(jmm_packet *packet, jbool copy)
{
    if (packet == NULL)
        return NULL;

    if ((packet->type!=JMM_CODEC_TYPE_AVC) || (packet->fmt!=JMM_BS_FMT_AVC_MP4))
    {
        jerr("[jmm_util] jmm_avc_mp42annexb invalid type %s, or fmt %s\n", type2str[packet->type], fmt2str[packet->fmt]);
        return NULL;
    }

    jmm_packet *pkt;
    if (copy)
        pkt = jmm_packet_clone(packet);
    else
        pkt = packet;

    int idx = 0;
    while (idx < (pkt->size-3))
    {
        uint32_t size = (pkt->data[idx]<<24)|(pkt->data[idx+1]<<16)|(pkt->data[idx+2]<<8)|(pkt->data[idx+3]);
        pkt->data[idx] = 0;
        pkt->data[idx+1] = 0;
        pkt->data[idx+2] = 0;
        pkt->data[idx+3] = 1;
        idx = idx + 4 + size;
    }

    pkt->fmt = JMM_BS_FMT_AVC_ANNEXB;

    return pkt;
}

jmm_packet *jmm_avc_annexb2mp4(jmm_packet *packet, jbool copy)
{
    if (packet == NULL)
        return NULL;

    if ((packet->type!=JMM_CODEC_TYPE_AVC) || (packet->fmt!=JMM_BS_FMT_AVC_ANNEXB))
    {
        jerr("[jmm_util] jmm_avc_annexb2mp4 invalid type %s, or fmt %s\n", type2str[packet->type], fmt2str[packet->fmt]);
        return NULL;
    }

    jmm_packet *pkt;
    if (copy)
        pkt = jmm_packet_clone(packet);
    else
        pkt = packet;

    int size;
    int idx = 0;
    int pos = idx;
    if ((pkt->data[idx]==0) && (pkt->data[idx+1]==0) && (pkt->data[idx+2]==0) && (pkt->data[idx+3]==1))
    {
        idx += 4;
        while (idx < (pkt->size-3))
        {
            if ((pkt->data[idx]==0) && (pkt->data[idx+1]==0) && (pkt->data[idx+2]==0) && (pkt->data[idx+3]==1))
            {
                size = idx - pos - 4;
                pkt->data[pos] = (size>>24) & 0xff;
                pkt->data[pos+1] = (size>>16) & 0xff;
                pkt->data[pos+2] = (size>>8) & 0xff;
                pkt->data[pos+3] = size & 0xff;
                pos = idx;
                idx += 4;
            }
            else
                idx++;
        }
        if (idx != (pkt->size-3))
        {
            if (copy)
                jmm_packet_free(pkt);
            return NULL;
        }
        size = idx - pos - 1;
        pkt->data[pos] = (size>>24) & 0xff;
        pkt->data[pos+1] = (size>>16) & 0xff;
        pkt->data[pos+2] = (size>>8) & 0xff;
        pkt->data[pos+3] = size & 0xff;
    }
    else
    {
        if (copy)
            jmm_packet_free(pkt);
        return NULL;
    }

    pkt->fmt = JMM_BS_FMT_AVC_MP4;

    return pkt;
}

// flv

jmm_packet *jmm_packet_flv_tag(jmm_packet *packet)
{
    if (packet == NULL)
        return NULL;

    //include PreviousTagSize
    jmm_packet *tag;

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
        return NULL;

    if (pkt->type == JMM_CODEC_TYPE_AAC)
    {
        tag = jmm_packet_alloc(pkt->size+17);
        if (tag == NULL)
        {
            if (copy)
                jmm_packet_free(pkt);
            return NULL;
        }
        //audio
        tag->data[0] = 0x8;
        //size
        tag->data[1] = ((pkt->size+2)>>16) & 0xff;
        tag->data[2] = ((pkt->size+2)>>8) & 0xff;
        tag->data[3] = (pkt->size+2) & 0xff;
        //dts 0~23
        tag->data[4] = ((pkt->dts/1000)>>16) & 0xff;
        tag->data[5] = ((pkt->dts/1000)>>8) & 0xff;
        tag->data[6] = (pkt->dts/1000) & 0xff;
        //dts 24~31
        tag->data[7] = ((pkt->dts/1000)>>24) & 0xff;
        //always 0
        tag->data[8] = 0;
        tag->data[9] = 0;
        tag->data[10] = 0;

        //assume 16bit 2-channel
        tag->data[11] = 0xaf;
        if (pkt->fmt == JMM_BS_FMT_AAC_ASC)
            tag->data[12] = 0;
        else
            tag->data[12] = 1;

        memcpy(&(tag->data[13]), pkt->data, pkt->size);
        tag->data[pkt->size+13] = ((pkt->size+13)>>24) & 0xff;
        tag->data[pkt->size+14] = ((pkt->size+13)>>16) & 0xff;
        tag->data[pkt->size+15] = ((pkt->size+13)>>8) & 0xff;
        tag->data[pkt->size+16] = (pkt->size+13) & 0xff;
    }
    else if (pkt->type == JMM_CODEC_TYPE_AVC)
    {
        tag = jmm_packet_alloc(pkt->size+20);
        if (tag == NULL)
        {
            if (copy)
                jmm_packet_free(pkt);
            return NULL;
        }
        //video
        tag->data[0] = 0x9;
        //size
        tag->data[1] = ((pkt->size+5)>>16) & 0xff;
        tag->data[2] = ((pkt->size+5)>>8) & 0xff;
        tag->data[3] = (pkt->size+5) & 0xff;
        //dts 0~23
        tag->data[4] = ((pkt->dts/1000)>>16) & 0xff;
        tag->data[5] = ((pkt->dts/1000)>>8) & 0xff;
        tag->data[6] = (pkt->dts/1000) & 0xff;
        //dts 24~31
        tag->data[7] = ((pkt->dts/1000)>>24) & 0xff;
        //always 0
        tag->data[8] = 0;
        tag->data[9] = 0;
        tag->data[10] = 0;

        if (pkt->key)
            tag->data[11] = 0x17;
        else
            tag->data[11] = 0x27;
        if (pkt->fmt == JMM_BS_FMT_AVC_AVCC)
        {
            tag->data[12] = 0;  //sps pps
            tag->data[13] = 0;
            tag->data[14] = 0;
            tag->data[15] = 0;
        }
        else
        {
            tag->data[12] = 1;  //nalu
            tag->data[13] = (((pkt->pts-pkt->dts)/1000)>>16) & 0xff;
            tag->data[14] = (((pkt->pts-pkt->dts)/1000)>>8) & 0xff;
            tag->data[15] = ((pkt->pts-pkt->dts)/1000) & 0xff;
        }

        memcpy(&(tag->data[16]), pkt->data, pkt->size);
        tag->data[pkt->size+16] = ((pkt->size+16)>>24) & 0xff;
        tag->data[pkt->size+17] = ((pkt->size+16)>>16) & 0xff;
        tag->data[pkt->size+18] = ((pkt->size+16)>>8) & 0xff;
        tag->data[pkt->size+19] = (pkt->size+16) & 0xff;
    }

    if (copy)
        jmm_packet_free(pkt);

    return tag;
}

jmm_packet *fetch_sps_pps(uint8_t *buf)
{
    if (buf == NULL)
        return NULL;

    //assume 0 0 0 1 67 ... 0 0 0 1 68 ... 0 0 0 1 ...
    int idx=0, count=0;
    while (count < 3)
    {
        if ((buf[idx]==0) && (buf[idx+1]==0) && (buf[idx+2]==0) && (buf[idx+3]==1))
        {
            count++;
            idx += 4;
        }
        else
            idx++;
    }

    int size = idx - 4;
    jmm_packet *pkt = jmm_packet_alloc(size);
    if (pkt == NULL)
        return NULL;

    pkt->type = JMM_CODEC_TYPE_AVC;
    pkt->fmt = JMM_BS_FMT_AVC_ANNEXB;
    pkt->key = jtrue;
    memcpy(pkt->data, buf, size);
    pkt = jmm_avc_annexb2avcc(pkt, jfalse);
    return pkt;
}

jmm_packet *jmm_packet_merge(jmm_packet *pkt1, jmm_packet *pkt2)
{
    if (pkt1 == NULL)
        return jmm_packet_clone(pkt2);
    if (pkt2 == NULL)
        return jmm_packet_clone(pkt1);

    jmm_packet *pkt = jmm_packet_alloc(pkt1->size+pkt2->size);
    if (pkt == NULL)
        return NULL;
    pkt->type = pkt1->type;
    pkt->fmt = pkt1->fmt;
    pkt->key = pkt1->key;
    pkt->dts = pkt1->dts;
    pkt->pts = pkt1->pts;
    memcpy(pkt->data, pkt1->data, pkt1->size);
    memcpy(pkt->data+pkt1->size, pkt2->data, pkt2->size);
    return pkt;
}


