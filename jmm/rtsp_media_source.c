/*
 * rtsp_media_source.c
 *
 * @chuanjiong
 */

#include "rtsp_media_source.h"
#include "base64.h"
#include "rtsp_rtp_sender.h"

typedef struct rtp_sink_context {
    rtsp_rtp_sender_cfg cfg;
    jhandle rtp_sender;
    int used;
}rtp_sink_context;

typedef struct media_source_context {
    int64_t a_time;         //timebase: samplerate
    int64_t v_time;         //timebase: 90000

    media_source_type type;
    jhandle url_source;

    rtp_sink_context sink[MEDIA_SOURCE_MAX_SINK];

    jmm_packet *asc;
    jmm_packet *avcc;

    pthread_mutex_t mutex;
}media_source_context;

jhandle media_source_setup(media_source_config *cfg)
{
    if (cfg == NULL)
        return NULL;

    media_source_context *ctx = (media_source_context *)jmalloc(sizeof(media_source_context));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(media_source_context));

    ctx->type = cfg->type;

    pthread_mutex_init(&(ctx->mutex), NULL);

    if (ctx->type == MEDIA_SOURCE_TYPE_URL)
        ctx->url_source = jmm_source_setup(cfg->url, jfalse, jtrue, media_source_send_packet, ctx);

    return ctx;
}

char *media_source_get_sdp(jhandle h)
{
    if (h == NULL)
        return NULL;

    media_source_context *ctx = (media_source_context *)h;

    pthread_mutex_lock(&(ctx->mutex));

    if (ctx->avcc == NULL)
    {
        pthread_mutex_unlock(&(ctx->mutex));
        return NULL;
    }

    jmm_asc_info asc_info;
    jmm_aac_asc_parse(ctx->asc, &asc_info);

    jmm_packet *avc = jmm_avc_avcc2annexb(ctx->avcc, jtrue);
    if (avc == NULL)
    {
        pthread_mutex_unlock(&(ctx->mutex));
        return NULL;
    }
    pthread_mutex_unlock(&(ctx->mutex));

    char sps_buf[64] = {0};
    char pps_buf[64] = {0};

    int i=4;
    while (1)
    {
        if ((avc->data[i]==0) && (avc->data[i+1]==0) && (avc->data[i+2]==0) && (avc->data[i+3]==1))
            break;
        i++;
    }

    base64_encode(&avc->data[4], i-4, sps_buf);
    base64_encode(&avc->data[i+4], avc->size-i-4, pps_buf);

    char *buf = (char *)jmalloc(1024);
    if (buf == NULL)
    {
        jmm_packet_free(avc);
        return NULL;
    }
    memset(buf, 0, 1024);

    if (ctx->asc)
    {
        jsnprintf(buf, 1024, SDP_FORMAT, asc_info.samplerate, asc_info.channels,
            ctx->asc->data[0], ctx->asc->data[1], sps_buf, pps_buf);
    }
    else
    {
        jsnprintf(buf, 1024, SDP_FORMAT_ONLY_VIDEO, sps_buf, pps_buf);
    }

    jmm_packet_free(avc);
    return buf;
}

int64_t media_source_get_time(jhandle h, jstring url)
{
    if ((h==NULL) || (stringsize(url)<=0))
        return -1;

    media_source_context *ctx = (media_source_context *)h;

    int64_t t;

    pthread_mutex_lock(&(ctx->mutex));

    if (!strcmp(strrchr(tostring(url), '/'), "/audio"))
    {
        t = ctx->a_time;
    }
    else
    {
        t = ctx->v_time;
    }

    pthread_mutex_unlock(&(ctx->mutex));

    return t;
}

int media_source_add_sink(jhandle h, jstring url[2], const char *ip, jsocket sock, jstring transport[2])
{
    if (h == NULL)
        return -1;

    media_source_context *ctx = (media_source_context *)h;

    int i;

    pthread_mutex_lock(&(ctx->mutex));

    for (i=0; i<MEDIA_SOURCE_MAX_SINK; i++)
    {
        if (ctx->sink[i].used == 0)
        {
            rtsp_rtp_sender_cfg cfg = {0};
            if (strstr(tostring(transport[0]), "RTP/AVP/TCP"))
            {
                cfg.type = RTSP_RTP_SENDER_TYPE_TCP;
                cfg.tcp.sock = sock;
                if (!strcmp(strrchr(tostring(url[0]), '/'), "/audio"))
                {
                    //0 -> audio
                    cfg.tcp.a_rtp_chn = jstring_pick_value(tostring(transport[0]), "interleaved=", "-");
                    cfg.tcp.a_rtcp_chn = cfg.tcp.a_rtp_chn+1;
                    //1 -> video
                    cfg.tcp.v_rtp_chn = jstring_pick_value(tostring(transport[1]), "interleaved=", "-");
                    cfg.tcp.v_rtcp_chn = cfg.tcp.v_rtp_chn+1;
                }
                else
                {
                    //0 -> video
                    cfg.tcp.v_rtp_chn = jstring_pick_value(tostring(transport[0]), "interleaved=", "-");
                    cfg.tcp.v_rtcp_chn = cfg.tcp.v_rtp_chn+1;
                    //1 -> audio
                    cfg.tcp.a_rtp_chn = jstring_pick_value(tostring(transport[1]), "interleaved=", "-");
                    cfg.tcp.a_rtcp_chn = cfg.tcp.a_rtp_chn+1;
                }
            }
            else
            {
                cfg.type = RTSP_RTP_SENDER_TYPE_UDP;
                if (!strcmp(strrchr(tostring(url[0]), '/'), "/audio"))
                {
                    //0 -> audio
                    cfg.udp.a_rtp_sock = udp_connect(ip, jstring_pick_value(tostring(transport[0]), "client_port=", "-"));
                    cfg.udp.a_rtcp_sock = udp_connect(ip, jstring_pick_value(tostring(transport[0]), "client_port=", "-")+1);
                    //1 -> video
                    cfg.udp.v_rtp_sock = udp_connect(ip, jstring_pick_value(tostring(transport[1]), "client_port=", "-"));
                    cfg.udp.v_rtcp_sock = udp_connect(ip, jstring_pick_value(tostring(transport[1]), "client_port=", "-")+1);
                }
                else
                {
                    //0 -> video
                    cfg.udp.v_rtp_sock = udp_connect(ip, jstring_pick_value(tostring(transport[0]), "client_port=", "-"));
                    cfg.udp.v_rtcp_sock = udp_connect(ip, jstring_pick_value(tostring(transport[0]), "client_port=", "-")+1);
                    //1 -> audio
                    cfg.udp.a_rtp_sock = udp_connect(ip, jstring_pick_value(tostring(transport[1]), "client_port=", "-"));
                    cfg.udp.a_rtcp_sock = udp_connect(ip, jstring_pick_value(tostring(transport[1]), "client_port=", "-")+1);
                }
            }
            jmm_asc_info asc_info = {0};
            jmm_aac_asc_parse(ctx->asc, &asc_info);
            cfg.a_seq = 0;
            cfg.a_timebase = (asc_info.samplerate>0)?asc_info.samplerate:44100;
            cfg.v_seq = 0;
            cfg.v_timebase = 90000;
            ctx->sink[i].cfg = cfg;
            ctx->sink[i].rtp_sender = rtsp_rtp_sender_alloc(&cfg);
            ctx->sink[i].used = 1;
            break;
        }
    }

    if (i == MEDIA_SOURCE_MAX_SINK)
    {
        pthread_mutex_unlock(&(ctx->mutex));
        return -1;
    }

    pthread_mutex_unlock(&(ctx->mutex));
    return i;
}

int media_source_del_sink(jhandle h, int index)
{
    if ((h==NULL) || (index<0) || (index>=MEDIA_SOURCE_MAX_SINK))
        return ERROR_FAIL;

    media_source_context *ctx = (media_source_context *)h;

    pthread_mutex_lock(&(ctx->mutex));

    if (ctx->sink[index].used)
    {
        rtsp_rtp_sender_free(ctx->sink[index].rtp_sender);
        if (ctx->sink[index].cfg.type == RTSP_RTP_SENDER_TYPE_UDP)
        {
            jsocket_close(ctx->sink[index].cfg.udp.a_rtp_sock);
            jsocket_close(ctx->sink[index].cfg.udp.a_rtcp_sock);
            jsocket_close(ctx->sink[index].cfg.udp.v_rtp_sock);
            jsocket_close(ctx->sink[index].cfg.udp.v_rtcp_sock);
        }
        ctx->sink[index].used = 0;
    }

    pthread_mutex_unlock(&(ctx->mutex));

    return SUCCESS;
}

int media_source_send_packet(jhandle h, jmm_packet *packet)
{
    if ((h==NULL) || (packet==NULL))
        return ERROR_FAIL;

    media_source_context *ctx = (media_source_context *)h;

    if (packet->fmt == JMM_BS_FMT_AAC_ASC)
    {
        pthread_mutex_lock(&(ctx->mutex));
        if (ctx->asc)
            jmm_packet_free(ctx->asc);
        ctx->asc = jmm_packet_clone(packet);
        pthread_mutex_unlock(&(ctx->mutex));
        return SUCCESS;
    }
    else if (packet->fmt == JMM_BS_FMT_AVC_AVCC)
    {
        pthread_mutex_lock(&(ctx->mutex));
        if (ctx->avcc)
            jmm_packet_free(ctx->avcc);
        ctx->avcc = jmm_packet_clone(packet);
        pthread_mutex_unlock(&(ctx->mutex));
        return SUCCESS;
    }

    jbool copy = jfalse;
    jmm_packet *pkt = packet;
    if (packet->fmt == JMM_BS_FMT_AAC_ADTS)
    {
        pkt = jmm_aac_adts2es(packet, jtrue);
        copy = jtrue;
    }
    else if (packet->fmt == JMM_BS_FMT_AVC_ANNEXB)
    {
        pkt = jmm_avc_annexb2mp4(packet, jtrue);
        copy = jtrue;
    }

    if (pkt == NULL)
        return ERROR_FAIL;

    pthread_mutex_lock(&(ctx->mutex));

    if (pkt->type == JMM_CODEC_TYPE_AAC)
    {
        if (ctx->asc)
        {
            jmm_asc_info asc_info;
            jmm_aac_asc_parse(ctx->asc, &asc_info);
            ctx->a_time = pkt->dts*asc_info.samplerate/1000000;
        }
    }
    else
        ctx->v_time = pkt->dts*9/100;

    int i;
    for (i=0; i<MEDIA_SOURCE_MAX_SINK; i++)
        if (ctx->sink[i].used)
            rtsp_rtp_sender_send(ctx->sink[i].rtp_sender, pkt);

    pthread_mutex_unlock(&(ctx->mutex));

    if (copy)
        jmm_packet_free(pkt);

    return SUCCESS;
}

void media_source_shutdown(jhandle h)
{
    if (h == NULL)
        return;

    media_source_context *ctx = (media_source_context *)h;

    if (ctx->type == MEDIA_SOURCE_TYPE_URL)
        jmm_source_shutdown(ctx->url_source);

    int i;
    for (i=0; i<MEDIA_SOURCE_MAX_SINK; i++)
        if (ctx->sink[i].used)
            rtsp_rtp_sender_free(ctx->sink[i].rtp_sender);

    if (ctx->asc)
        jmm_packet_free(ctx->asc);
    if (ctx->avcc)
        jmm_packet_free(ctx->avcc);

    pthread_mutex_destroy(&(ctx->mutex));

    jfree(ctx);
}


