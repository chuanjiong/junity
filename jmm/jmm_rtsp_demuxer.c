/*
 * jmm_rtsp_demuxer.c
 *
 * @chuanjiong
 */

#include "jmm_module.h"
#include "rtsp_request.h"
#include "base64.h"
#include "md5.h"

#define JMM_RTSP_DEMUXER_TCP_MODE   (0)

#define REQ_RES_RECV_BUF_SIZE       (4096)

typedef enum {
    OPTIONS,
    DESCRIBE,
    SETUP_VIDEO,
    SETUP_AUDIO,
    PLAY,
    TEARDOWN,
}STEP;

typedef struct jmm_rtsp_demuxer_ctx {
    STEP step;

    jstring url;
    jstring user;
    jstring passwd;
    jstring ip;
    jstring port;
    jstring path;

    jhandle evt;

    jsocket sock;
    jsocket a_rtp_sock;
    jsocket a_rtcp_sock;
    jsocket v_rtp_sock;
    jsocket v_rtcp_sock;
    int a_rtp_port;

    int cseq;
    uint8_t request[REQ_RES_RECV_BUF_SIZE];
    uint8_t response[REQ_RES_RECV_BUF_SIZE];
    int res_size;

    jbool auth;
    jstring auth_method;
    jstring auth_realm;
    jstring auth_nonce;

    jstring sdp;
    jstring base;
    jstring atrack;
    jstring vtrack;
    jstring session;

    int a_timebase;
    int v_timebase;
    int a_rtp_seq;
    int64_t a_rtp_time;
    int v_rtp_seq;
    int64_t v_rtp_time;

    uint8_t recv_buf[REQ_RES_RECV_BUF_SIZE];
    jhandle a_rtp_merge;
    jhandle v_rtp_merge;

    volatile int play_flag;

    jbool getASC;
    jbool getAVCC;
    jmm_packet *asc;
    jmm_packet *avcc;
}jmm_rtsp_demuxer_ctx;

// rtsp://[user[:passwd]@]ip[:port][/path]
static int jmm_rtsp_demuxer_parse_url(jmm_rtsp_demuxer_ctx *ctx, const char *url)
{
    if ((ctx==NULL) || (url==NULL))
        return ERROR_FAIL;

    const char *p, *q;

    if ((p=strstr(url, "rtsp://")) == NULL)
        return ERROR_FAIL;
    const char *cut_rtsp = p + strlen("rtsp://");

    jstring user_passwd = jstring_pick(url, "rtsp://", "@");
    if (stringsize(user_passwd) > 0)
    {
        if (strchr(tostring(user_passwd), ':') == NULL)
        {
            ctx->user = jstring_copy(tostring(user_passwd));
        }
        else
        {
            ctx->user = jstring_pick(url, "rtsp://", ":");
            ctx->passwd = jstring_pick(cut_rtsp, ":", "@");
        }
    }
    jstring_free(user_passwd);

    const char *cut_user_passwd;
    if ((p=strchr(url, '@')) == NULL)
        cut_user_passwd = cut_rtsp;
    else
        cut_user_passwd = p + 1;

    ctx->url = jstring_link("rtsp://", cut_user_passwd);

    p = strchr(cut_user_passwd, ':');
    q = strchr(cut_user_passwd, '/');

    if ((p==NULL) && (q==NULL))
    {
        ctx->ip = jstring_copy(cut_user_passwd);
    }
    else if (p == NULL)
    {
        ctx->ip = jstring_cut(cut_user_passwd, "/");
        q = strchr(cut_user_passwd, '/');
        ctx->path = jstring_copy(q);
    }
    else if (q == NULL)
    {
        ctx->ip = jstring_cut(cut_user_passwd, ":");
        q = strchr(cut_user_passwd, ':');
        ctx->port = jstring_copy(q+1);
    }
    else
    {
        ctx->ip = jstring_cut(cut_user_passwd, ":");
        ctx->port = jstring_pick(cut_user_passwd, ":", "/");
        q = strchr(cut_user_passwd, '/');
        ctx->path = jstring_copy(q);
    }

    return SUCCESS;
}

//response= md5(md5(username:realm:password):nonce:md5(public_method:url));
static jstring jmm_rtsp_demuxer_auth(jmm_rtsp_demuxer_ctx *ctx, STEP step)
{
    jstring auth = {0, NULL};

    MD5_CTX md5;
    unsigned char temp[16];

    if (!jstring_compare(ctx->auth_method, "Digest"))
    {
        //md5(username:realm:password)
        MD5_Init(&md5);
        MD5_Update(&md5, tostring(ctx->user), stringsize(ctx->user));
        MD5_Update(&md5, ":", 1);
        MD5_Update(&md5, tostring(ctx->auth_realm), stringsize(ctx->auth_realm));
        MD5_Update(&md5, ":", 1);
        MD5_Update(&md5, tostring(ctx->passwd), stringsize(ctx->passwd));
        MD5_Final(temp, &md5);
        jstring str1 = jstring_from_hex(temp, 16);

        //md5(public_method:url)
        MD5_Init(&md5);
        switch (step)
        {
            case DESCRIBE:
                MD5_Update(&md5, "DESCRIBE", strlen("DESCRIBE"));
                break;

            case SETUP_VIDEO:
                MD5_Update(&md5, "SETUP", strlen("SETUP"));
                break;

            case SETUP_AUDIO:
                MD5_Update(&md5, "SETUP", strlen("SETUP"));
                break;

            case PLAY:
                MD5_Update(&md5, "PLAY", strlen("PLAY"));
                break;

            default:
                break;
        }
        MD5_Update(&md5, ":", 1);
        MD5_Update(&md5, tostring(ctx->url), stringsize(ctx->url));
        MD5_Final(temp, &md5);
        jstring str2 = jstring_from_hex(temp, 16);

        //md5(str1:nonce:str2)
        MD5_Init(&md5);
        MD5_Update(&md5, tostring(str1), stringsize(str1));
        MD5_Update(&md5, ":", 1);
        MD5_Update(&md5, tostring(ctx->auth_nonce), stringsize(ctx->auth_nonce));
        MD5_Update(&md5, ":", 1);
        MD5_Update(&md5, tostring(str2), stringsize(str2));
        MD5_Final(temp, &md5);
        jstring str3 = jstring_from_hex(temp, 16);

        char buf[256] = {0};
        jsnprintf(buf, 256, "%s username=\"%s\", realm=\"%s\", nonce=\"%s\", "
            "uri=\"%s\", response=\"%s\"", tostring(ctx->auth_method),
            tostring(ctx->user), tostring(ctx->auth_realm), tostring(ctx->auth_nonce),
            tostring(ctx->url), tostring(str3));

        auth = jstring_copy(buf);

        jstring_free(str1);
        jstring_free(str2);
        jstring_free(str3);
    }
    else
    {
        char buf[64] = {0};
        char e_buf[64] = {0};
        jsnprintf(buf, 64, "%s:%s", tostring(ctx->user), tostring(ctx->passwd));
        base64_encode(buf, strlen(buf), e_buf);
        jsnprintf(buf, 64, "Basic %s", e_buf);
        auth = jstring_copy(buf);
    }

    return auth;
}

static int jmm_rtsp_demuxer_parse_sdp(jmm_rtsp_demuxer_ctx *ctx)
{
    if (ctx == NULL)
        return ERROR_FAIL;

    const char *a;
    jstring temp;

    const char *audio = strstr(tostring(ctx->sdp), "m=audio");
    const char *atype = strstr(tostring(ctx->sdp), "mpeg4-generic");
    if (audio && atype)
    {
        ctx->atrack = jstring_pick(audio, "a=control:", "\r\n");
        if (strstr(tostring(ctx->atrack), "rtsp://") == NULL)
        {
            jstring atrack = jstring_link(tostring(ctx->base), tostring(ctx->atrack));
            jstring_free(ctx->atrack);
            ctx->atrack = atrack;
        }

        temp = jstring_pick(audio, ";config=", "\r\n");
        if (stringsize(temp) > 0)
        {
            jstring asc = jstring_cut(tostring(temp), ";");
            if (asc.size == 0)
                asc = temp;
            else
                jstring_free(temp);

            ctx->asc = jmm_packet_alloc(stringsize(asc)/2);
            if (ctx->asc)
            {
                ctx->asc->type = JMM_CODEC_TYPE_AAC;
                ctx->asc->fmt = JMM_BS_FMT_AAC_ASC;
                ctx->asc->key = jtrue;
                ctx->asc->dts = 0;
                ctx->asc->pts = ctx->asc->dts;
                jstring_to_hex(ctx->asc->data, asc);
            }
            jstring_free(asc);

            a = strstr(audio, "a=rtpmap:");
            temp = jstring_pick(a, "/", "/");
            ctx->a_timebase = atoi(tostring(temp));
            jstring_free(temp);
        }
        else
        {
            ctx->atrack = jstring_free(ctx->atrack);
        }
    }

    const char *video = strstr(tostring(ctx->sdp), "m=video");
    if (video)
    {
        ctx->vtrack = jstring_pick(video, "a=control:", "\r\n");
        if (strstr(tostring(ctx->vtrack), "rtsp://") == NULL)
        {
            jstring vtrack = jstring_link(tostring(ctx->base), tostring(ctx->vtrack));
            jstring_free(ctx->vtrack);
            ctx->vtrack = vtrack;
        }

        jstring sps = jstring_pick(tostring(ctx->sdp), "sprop-parameter-sets=", ",");
        int sps_sub = jstring_char_count(tostring(sps), '=');

        a = strstr(tostring(ctx->sdp), "sprop-parameter-sets=");
        jstring pps = jstring_pick(a, ",", "\r\n");
        int pps_sub = jstring_char_count(tostring(pps), '=');

        int sps_size = BASE64_DECODE_OUT_SIZE(stringsize(sps)) - sps_sub;
        int pps_size = BASE64_DECODE_OUT_SIZE(stringsize(pps)) - pps_sub;

        uint8_t buf[256] = {0};

        int size = sps_size + pps_size + 4 + 4;
        ctx->avcc = jmm_packet_alloc(size);
        if (ctx->avcc)
        {
            ctx->avcc->type = JMM_CODEC_TYPE_AVC;
            ctx->avcc->fmt = JMM_BS_FMT_AVC_ANNEXB;
            ctx->avcc->key = jtrue;
            ctx->avcc->dts = 0;
            ctx->avcc->pts = ctx->avcc->dts;

            //0 0 0 1 67 ...
            ctx->avcc->data[3] = 1;
            base64_decode(tostring(sps), stringsize(sps), buf);
            memcpy(&(ctx->avcc->data[4]), buf, sps_size);

            //0 0 0 1 68 ...
            ctx->avcc->data[7+sps_size] = 1;
            base64_decode(tostring(pps), stringsize(pps), buf);
            memcpy(&(ctx->avcc->data[8+sps_size]), buf, pps_size);
        }

        jstring_free(sps);
        jstring_free(pps);

        a = strstr(video, "a=rtpmap:");
        temp = jstring_pick(a, "/", "\r\n");
        ctx->v_timebase = atoi(tostring(temp));
        jstring_free(temp);
    }

    return SUCCESS;
}

static int jmm_rtsp_demuxer_send_request(jmm_rtsp_demuxer_ctx *ctx, STEP step)
{
    if (ctx == NULL)
        return ERROR_FAIL;

    memset(ctx->request, 0, REQ_RES_RECV_BUF_SIZE);

    switch (step)
    {
        case OPTIONS:
            if (ctx->auth)
            {
                jstring auth = jmm_rtsp_demuxer_auth(ctx, step);
                jsnprintf(ctx->request, REQ_RES_RECV_BUF_SIZE, REQUEST_OPTIONS_AUTH,
                    tostring(ctx->url), ctx->cseq++, tostring(auth));
                jstring_free(auth);
            }
            else
            {
                jsnprintf(ctx->request, REQ_RES_RECV_BUF_SIZE, REQUEST_OPTIONS,
                    tostring(ctx->url), ctx->cseq++);
            }
            break;

        case DESCRIBE:
            if (ctx->auth)
            {
                jstring auth = jmm_rtsp_demuxer_auth(ctx, step);
                jsnprintf(ctx->request, REQ_RES_RECV_BUF_SIZE, REQUEST_DESCRIBE_AUTH,
                    tostring(ctx->url), ctx->cseq++, tostring(auth));
                jstring_free(auth);
            }
            else
            {
                jsnprintf(ctx->request, REQ_RES_RECV_BUF_SIZE, REQUEST_DESCRIBE,
                    tostring(ctx->url), ctx->cseq++);
            }
            break;

        case SETUP_VIDEO:
            #if (JMM_RTSP_DEMUXER_TCP_MODE == 0)
            if (ctx->auth)
            {
                jstring auth = jmm_rtsp_demuxer_auth(ctx, step);
                jsnprintf(ctx->request, REQ_RES_RECV_BUF_SIZE, REQUEST_SETUP_VIDEO_AUTH_UDP,
                    tostring(ctx->vtrack), ctx->cseq++, tostring(auth),
                    ctx->a_rtp_port+2, ctx->a_rtp_port+3);
                jstring_free(auth);
            }
            else
            {
                jsnprintf(ctx->request, REQ_RES_RECV_BUF_SIZE, REQUEST_SETUP_VIDEO_UDP,
                    tostring(ctx->vtrack), ctx->cseq++,
                    ctx->a_rtp_port+2, ctx->a_rtp_port+3);
            }
            #else
            if (ctx->auth)
            {
                jstring auth = jmm_rtsp_demuxer_auth(ctx, step);
                jsnprintf(ctx->request, REQ_RES_RECV_BUF_SIZE, REQUEST_SETUP_VIDEO_AUTH_TCP,
                    tostring(ctx->vtrack), ctx->cseq++, tostring(auth));
                jstring_free(auth);
            }
            else
            {
                jsnprintf(ctx->request, REQ_RES_RECV_BUF_SIZE, REQUEST_SETUP_VIDEO_TCP,
                    tostring(ctx->vtrack), ctx->cseq++);
            }
            #endif
            break;

        case SETUP_AUDIO:
            #if (JMM_RTSP_DEMUXER_TCP_MODE == 0)
            if (ctx->auth)
            {
                jstring auth = jmm_rtsp_demuxer_auth(ctx, step);
                jsnprintf(ctx->request, REQ_RES_RECV_BUF_SIZE, REQUEST_SETUP_AUDIO_AUTH_UDP,
                    tostring(ctx->atrack), ctx->cseq++, tostring(auth),
                    ctx->a_rtp_port, ctx->a_rtp_port+1, tostring(ctx->session));
                jstring_free(auth);
            }
            else
            {
                jsnprintf(ctx->request, REQ_RES_RECV_BUF_SIZE, REQUEST_SETUP_AUDIO_UDP,
                    tostring(ctx->atrack), ctx->cseq++,
                    ctx->a_rtp_port, ctx->a_rtp_port+1, tostring(ctx->session));
            }
            #else
            if (ctx->auth)
            {
                jstring auth = jmm_rtsp_demuxer_auth(ctx, step);
                jsnprintf(ctx->request, REQ_RES_RECV_BUF_SIZE, REQUEST_SETUP_AUDIO_AUTH_TCP,
                    tostring(ctx->atrack), ctx->cseq++, tostring(auth), tostring(ctx->session));
                jstring_free(auth);
            }
            else
            {
                jsnprintf(ctx->request, REQ_RES_RECV_BUF_SIZE, REQUEST_SETUP_AUDIO_TCP,
                    tostring(ctx->atrack), ctx->cseq++, tostring(ctx->session));
            }
            #endif
            break;

        case PLAY:
            //FIXME
            if (ctx->base.data[stringsize(ctx->base)-1] == '/')
            {
                ctx->base.data[stringsize(ctx->base)-1] = 0;
            }
            if (ctx->auth)
            {
                jstring auth = jmm_rtsp_demuxer_auth(ctx, step);
                jsnprintf(ctx->request, REQ_RES_RECV_BUF_SIZE, REQUEST_PLAY_AUTH,
                    tostring(ctx->base), ctx->cseq++, tostring(auth), tostring(ctx->session));
                jstring_free(auth);
            }
            else
            {
                jsnprintf(ctx->request, REQ_RES_RECV_BUF_SIZE, REQUEST_PLAY,
                    tostring(ctx->base), ctx->cseq++, tostring(ctx->session));
            }
            break;

        case TEARDOWN:
            jsnprintf(ctx->request, REQ_RES_RECV_BUF_SIZE, REQUEST_TEARDOWN,
                tostring(ctx->url), ctx->cseq++, tostring(ctx->session));
            break;

        default:
            return ERROR_FAIL;
    }

    //jlog("[jmm_rtsp_demuxer] request:\n%s\n", ctx->request);

    if (tcp_write(ctx->sock, (uint8_t *)(ctx->request), strlen(ctx->request)) != strlen(ctx->request))
    {
        jerr("[jmm_rtsp_demuxer] send request fail\n");
        return ERROR_FAIL;
    }

    ctx->step = step;
    return SUCCESS;
}

static jevent_ret jmm_rtsp_demuxer_handler(int fd, jevent_type type, void *arg)
{
    if ((fd<0) || (type!=EVT_TYPE_READ) || (arg==NULL))
        return EVT_RET_REMOVE;

    jmm_rtsp_demuxer_ctx *ctx = (jmm_rtsp_demuxer_ctx *)arg;

    //read complete response
    int size = tcp_read(fd, &(ctx->response[ctx->res_size]), REQ_RES_RECV_BUF_SIZE-ctx->res_size);
    if ((size<=0) && (errno!=EINTR) && (errno!=EAGAIN))
        return EVT_RET_REMOVE;
    if (size > 0)
        ctx->res_size += size;

    if (ctx->res_size <= 0)
        return EVT_RET_SUCCESS;

    #if (JMM_RTSP_DEMUXER_TCP_MODE == 1)
loop:
    if (ctx->response[0] == '$')
    {
        if (ctx->res_size < 4)
            return EVT_RET_SUCCESS;
        int psize = (ctx->response[2]<<8) | ctx->response[3];
        if (ctx->res_size < (psize+4))
            return EVT_RET_SUCCESS;
        int chn = ctx->response[1];
        if (chn == 0) //video
            jmm_rtp_merge_write(ctx->v_rtp_merge, &(ctx->response[4]), psize);
        else if (chn == 2) //audio
            jmm_rtp_merge_write(ctx->a_rtp_merge, &(ctx->response[4]), psize);
        int left = ctx->res_size - psize - 4;
        if (left > 0)
            memmove(ctx->response, &(ctx->response[psize+4]), left);
        ctx->res_size = left;
        memset(&(ctx->response[left]), 0, REQ_RES_RECV_BUF_SIZE-left);
        if (left > 0)
            goto loop;
        return EVT_RET_SUCCESS;
    }
    else
    #endif
    if (strstr((const char *)(ctx->response), "\r\n\r\n") == NULL)
        return EVT_RET_SUCCESS;

    //jlog("[jmm_rtsp_demuxer] response:\n%s\n", ctx->response);

    if (strstr(ctx->response, "OK") == NULL)
    {
        if ((strstr(ctx->response, "Unauthorized")!=NULL) && (ctx->auth==jfalse))
        {
            ctx->auth = jtrue;
            ctx->auth_method = jstring_pick(ctx->response, "WWW-Authenticate: ", " ");
            if (jstring_compare(ctx->auth_method, "Basic") == 0)
            {
                ctx->auth_realm = jstring_pick(ctx->response, "realm=\"", "\"");
            }
            else
            {
                ctx->auth_realm = jstring_pick(ctx->response, "realm=\"", "\",");
                ctx->auth_nonce = jstring_pick(ctx->response, "nonce=\"", "\"");
            }
            if (jmm_rtsp_demuxer_send_request(ctx, ctx->step) != SUCCESS)
                return EVT_RET_REMOVE;
            ctx->res_size = 0;
            memset(ctx->response, 0, REQ_RES_RECV_BUF_SIZE);
            return EVT_RET_SUCCESS;
        }
        else
        {
            jlog("[jmm_rtsp_demuxer] fail, response:\n%s\n", ctx->response);
            return EVT_RET_REMOVE;
        }
    }

    switch (ctx->step)
    {
        case OPTIONS:
            if (jmm_rtsp_demuxer_send_request(ctx, DESCRIBE) != SUCCESS)
                return EVT_RET_REMOVE;
            break;

        case DESCRIBE:
            {
                jstring content_length = jstring_pick(ctx->response, "Content-Length: ", "\r\n");
                int content_size = atoi(tostring(content_length));
                jstring_free(content_length);
                ctx->base = jstring_pick(ctx->response, "Content-Base: ", "\r\n");
                //FIXME
                #if 1
                if (ctx->base.data[ctx->base.size-2] != '/')
                {
                    jstring temp = jstring_link(tostring(ctx->base), "/");
                    jstring_free(ctx->base);
                    ctx->base = temp;
                }
                #endif
                if (content_size > (strlen(strstr(ctx->response, "\r\n\r\n"))-4))
                    return EVT_RET_SUCCESS;
                ctx->sdp = jstring_copy(strstr(ctx->response, "\r\n\r\n")+4);
                jmm_rtsp_demuxer_parse_sdp(ctx);
                if (jmm_rtsp_demuxer_send_request(ctx, SETUP_VIDEO) != SUCCESS)
                    return EVT_RET_REMOVE;
            }
            break;

        case SETUP_VIDEO:
            {
                ctx->session = jstring_pick_1st_word(ctx->response, "Session: ", "\r\n");
                if (stringsize(ctx->atrack) > 0)
                {
                    if (jmm_rtsp_demuxer_send_request(ctx, SETUP_AUDIO) != SUCCESS)
                        return EVT_RET_REMOVE;
                }
                else
                {
                    if (jmm_rtsp_demuxer_send_request(ctx, PLAY) != SUCCESS)
                        return EVT_RET_REMOVE;
                }
            }
            break;

        case SETUP_AUDIO:
            if (jmm_rtsp_demuxer_send_request(ctx, PLAY) != SUCCESS)
                return EVT_RET_REMOVE;
            break;

        case PLAY:
            {
                if (ctx->atrack.size > 0)
                {
                    const char *p = strstr(ctx->response, tostring(ctx->atrack));
                    if (p != NULL)
                    {
                        jstring temp = jstring_pick(p, "seq=", ";");
                        if (temp.size == 0)
                            temp = jstring_pick(p, "seq=", "\r\n");
                        ctx->a_rtp_seq = atoi(tostring(temp)) - 1;
                        jstring_free(temp);
                        temp = jstring_pick(p, "rtptime=", ";");
                        if (temp.size == 0)
                            temp = jstring_pick(p, "rtptime=", "\r\n");
                        ctx->a_rtp_time = strtoull(tostring(temp), NULL, 10);
                        jstring_free(temp);
                    }
                }

                if (ctx->vtrack.size > 0)
                {
                    const char *p = strstr(ctx->response, tostring(ctx->vtrack));
                    if (p != NULL)
                    {
                        jstring temp = jstring_pick(p, "seq=", ";");
                        if (temp.size == 0)
                            temp = jstring_pick(p, "seq=", "\r\n");
                        ctx->v_rtp_seq = atoi(tostring(temp)) - 1;
                        jstring_free(temp);
                        temp = jstring_pick(p, "rtptime=", ";");
                        if (temp.size == 0)
                            temp = jstring_pick(p, "rtptime=", "\r\n");
                        ctx->v_rtp_time = strtoull(tostring(temp), NULL, 10);
                        jstring_free(temp);
                    }
                }

                jatomic_true(&(ctx->play_flag));
            }
            break;

        case TEARDOWN:
            return EVT_RET_REMOVE;

        default:
            break;
    }

    #if (JMM_RTSP_DEMUXER_TCP_MODE == 1)
    if (ctx->step == PLAY)
    {
        const char *p = strstr((const char *)(ctx->response), "\r\n\r\n");
        p += 4;
        int left = ctx->res_size - (p - (const char *)ctx->response);
        if (left > 0)
            memmove(ctx->response, p, left);
        ctx->res_size = left;
        memset(&(ctx->response[left]), 0, REQ_RES_RECV_BUF_SIZE-left);
    }
    else
    #endif
    {
        ctx->res_size = 0;
        memset(ctx->response, 0, REQ_RES_RECV_BUF_SIZE);
    }
    return EVT_RET_SUCCESS;
}

static jevent_ret jmm_rtsp_demuxer_a_rtp_handler(int fd, jevent_type type, void *arg)
{
    if ((fd<0) || (type!=EVT_TYPE_READ) || (arg==NULL))
        return EVT_RET_REMOVE;

    jmm_rtsp_demuxer_ctx *ctx = (jmm_rtsp_demuxer_ctx *)arg;

    int size = udp_read(fd, ctx->recv_buf, REQ_RES_RECV_BUF_SIZE);
    if (size > 0)
        jmm_rtp_merge_write(ctx->a_rtp_merge, ctx->recv_buf, size);

    return EVT_RET_SUCCESS;
}

static jevent_ret jmm_rtsp_demuxer_a_rtcp_handler(int fd, jevent_type type, void *arg)
{
    if ((fd<0) || (type!=EVT_TYPE_READ) || (arg==NULL))
        return EVT_RET_REMOVE;

    jmm_rtsp_demuxer_ctx *ctx = (jmm_rtsp_demuxer_ctx *)arg;

    int size = udp_read(fd, ctx->recv_buf, REQ_RES_RECV_BUF_SIZE);
    jlog("[jmm_rtsp_demuxer] a rtcp size: %d\n", size);

    return EVT_RET_SUCCESS;
}

static jevent_ret jmm_rtsp_demuxer_v_rtp_handler(int fd, jevent_type type, void *arg)
{
    if ((fd<0) || (type!=EVT_TYPE_READ) || (arg==NULL))
        return EVT_RET_REMOVE;

    jmm_rtsp_demuxer_ctx *ctx = (jmm_rtsp_demuxer_ctx *)arg;

    int size = udp_read(fd, ctx->recv_buf, REQ_RES_RECV_BUF_SIZE);
    if (size > 0)
        jmm_rtp_merge_write(ctx->v_rtp_merge, ctx->recv_buf, size);

    return EVT_RET_SUCCESS;
}

static jevent_ret jmm_rtsp_demuxer_v_rtcp_handler(int fd, jevent_type type, void *arg)
{
    if ((fd<0) || (type!=EVT_TYPE_READ) || (arg==NULL))
        return EVT_RET_REMOVE;

    jmm_rtsp_demuxer_ctx *ctx = (jmm_rtsp_demuxer_ctx *)arg;

    int size = udp_read(fd, ctx->recv_buf, REQ_RES_RECV_BUF_SIZE);
    jlog("[jmm_rtsp_demuxer] v rtcp size: %d\n", size);

    return EVT_RET_SUCCESS;
}

static void jmm_rtsp_demuxer_clear(jmm_rtsp_demuxer_ctx *ctx)
{
    jsocket_close(ctx->sock);

    #if (JMM_RTSP_DEMUXER_TCP_MODE == 0)
    jevent_del_event(ctx->evt, ctx->a_rtp_sock);
    jevent_del_event(ctx->evt, ctx->a_rtcp_sock);
    jevent_del_event(ctx->evt, ctx->v_rtp_sock);
    jevent_del_event(ctx->evt, ctx->v_rtcp_sock);
    jsocket_close(ctx->a_rtp_sock);
    jsocket_close(ctx->a_rtcp_sock);
    jsocket_close(ctx->v_rtp_sock);
    jsocket_close(ctx->v_rtcp_sock);
    #endif

    jstring_free(ctx->url);
    jstring_free(ctx->user);
    jstring_free(ctx->passwd);
    jstring_free(ctx->ip);
    jstring_free(ctx->port);
    jstring_free(ctx->path);

    jstring_free(ctx->auth_method);
    jstring_free(ctx->auth_realm);
    jstring_free(ctx->auth_nonce);

    jstring_free(ctx->sdp);
    jstring_free(ctx->base);
    jstring_free(ctx->atrack);
    jstring_free(ctx->vtrack);
    jstring_free(ctx->session);

    jmm_rtp_merge_close(ctx->a_rtp_merge);
    jmm_rtp_merge_close(ctx->v_rtp_merge);

    if (ctx->asc)
        jmm_packet_free(ctx->asc);
    if (ctx->avcc)
        jmm_packet_free(ctx->avcc);

    jevent_free(ctx->evt);

    jfree(ctx);
}

static jhandle jmm_rtsp_demuxer_open(const char *url)
{
    if (url == NULL)
        return NULL;

    jmm_rtsp_demuxer_ctx *ctx = (jmm_rtsp_demuxer_ctx *)jmalloc(sizeof(jmm_rtsp_demuxer_ctx));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(jmm_rtsp_demuxer_ctx));

    #if (JMM_RTSP_DEMUXER_TCP_MODE == 0)
    //random port for rtp and rtcp -> [10000,65536)
    for (ctx->a_rtp_port=10000; ctx->a_rtp_port<65536; ctx->a_rtp_port += 4)
    {
        ctx->a_rtp_sock = udp_bind(ctx->a_rtp_port);
        ctx->a_rtcp_sock = udp_bind(ctx->a_rtp_port+1);
        ctx->v_rtp_sock = udp_bind(ctx->a_rtp_port+2);
        ctx->v_rtcp_sock = udp_bind(ctx->a_rtp_port+3);
        if ((ctx->a_rtp_sock>=0) && (ctx->a_rtcp_sock>=0) && (ctx->v_rtp_sock>=0) && (ctx->v_rtcp_sock>=0))
            break;
        jsocket_close(ctx->a_rtp_sock);
        jsocket_close(ctx->a_rtcp_sock);
        jsocket_close(ctx->v_rtp_sock);
        jsocket_close(ctx->v_rtcp_sock);
    }
    if (ctx->a_rtp_port >= 65536)
    {
        jfree(ctx);
        return NULL;
    }
    #endif

    jmm_rtsp_demuxer_parse_url(ctx, url);
    //jlog("[jmm_rtsp_demuxer] url: %s, user: %s, passwd: %s, ip: %s, port: %s, path: %s\n",
    //    tostring(ctx->url), tostring(ctx->user), tostring(ctx->passwd),
    //    tostring(ctx->ip), tostring(ctx->port), tostring(ctx->path));

    if (stringsize(ctx->ip) <= 0)
    {
        jmm_rtsp_demuxer_clear(ctx);
        return NULL;
    }

    if (stringsize(ctx->port) <= 0)
        ctx->port = jstring_copy("554");

    ctx->sock = tcp_connect(tostring(ctx->ip), atoi(tostring(ctx->port)));
    if (ctx->sock < 0)
    {
        jmm_rtsp_demuxer_clear(ctx);
        return NULL;
    }

    if (jmm_rtsp_demuxer_send_request(ctx, OPTIONS) != SUCCESS)
    {
        jmm_rtsp_demuxer_clear(ctx);
        return NULL;
    }

    ctx->evt = jevent_alloc();

    jevent_add_event(ctx->evt, ctx->sock, EVT_TYPE_READ,
        jmm_rtsp_demuxer_handler, ctx, NULL, NULL);
    #if (JMM_RTSP_DEMUXER_TCP_MODE == 0)
    jevent_add_event(ctx->evt, ctx->a_rtp_sock, EVT_TYPE_READ,
        jmm_rtsp_demuxer_a_rtp_handler, ctx, NULL, NULL);
    jevent_add_event(ctx->evt, ctx->a_rtcp_sock, EVT_TYPE_READ,
        jmm_rtsp_demuxer_a_rtcp_handler, ctx, NULL, NULL);
    jevent_add_event(ctx->evt, ctx->v_rtp_sock, EVT_TYPE_READ,
        jmm_rtsp_demuxer_v_rtp_handler, ctx, NULL, NULL);
    jevent_add_event(ctx->evt, ctx->v_rtcp_sock, EVT_TYPE_READ,
        jmm_rtsp_demuxer_v_rtcp_handler, ctx, NULL, NULL);
    #endif

    ctx->a_rtp_merge = jmm_rtp_merge_open(JMM_CODEC_TYPE_AAC);
    ctx->v_rtp_merge = jmm_rtp_merge_open(JMM_CODEC_TYPE_AVC);

    return ctx;
}

static void jmm_rtsp_demuxer_close(jhandle h)
{
    if (h == NULL)
        return;

    jmm_rtsp_demuxer_ctx *ctx = (jmm_rtsp_demuxer_ctx *)h;
    jmm_rtsp_demuxer_send_request(ctx, TEARDOWN);
    jmm_rtsp_demuxer_clear(ctx);
}

static jmm_packet *jmm_rtsp_demuxer_extradata(jhandle h, jmm_codec_type type)
{
    if (h == NULL)
        return NULL;

    jmm_rtsp_demuxer_ctx *ctx = (jmm_rtsp_demuxer_ctx *)h;

    if (type == JMM_CODEC_TYPE_AAC)
        return jmm_packet_clone(ctx->asc);
    else
        return jmm_packet_clone(ctx->avcc);
}

static jmm_packet *jmm_rtsp_demuxer_read(jhandle h)
{
    if (h == NULL)
        return NULL;

    jmm_rtsp_demuxer_ctx *ctx = (jmm_rtsp_demuxer_ctx *)h;

    int times=100;
    while (jatomic_get(&(ctx->play_flag)) == 0)
    {
        if (times-- > 0)
            jsleep(10);
        else
            return NULL;
    }

    if (ctx->asc && (!ctx->getASC))
    {
        ctx->getASC = jtrue;
        return jmm_packet_clone(ctx->asc);
    }

    if (ctx->avcc && (!ctx->getAVCC))
    {
        ctx->getAVCC = jtrue;
        return jmm_packet_clone(ctx->avcc);
    }

    times = 300;
    jmm_packet *pkt;
    while (times-- > 0)
    {
        pkt = jmm_rtp_merge_read(ctx->a_rtp_merge);
        if (pkt == NULL)
            pkt = jmm_rtp_merge_read(ctx->v_rtp_merge);

        if (pkt)
            return pkt;

        jsleep(10);
    }
    jerr("[jmm_rtsp_demuxer] read timeout\n");
    return NULL;
}

static int jmm_rtsp_demuxer_seek(jhandle h, int64_t ts)
{
    return ERROR_FAIL;
}

static int jmm_rtsp_demuxer_finfo(jhandle h, jmm_file_info *info)
{
    return ERROR_FAIL;
}

const jmm_demuxer jmm_rtsp_demuxer = {
    jmm_rtsp_demuxer_open,
    jmm_rtsp_demuxer_close,
    jmm_rtsp_demuxer_extradata,
    jmm_rtsp_demuxer_read,
    jmm_rtsp_demuxer_seek,
    jmm_rtsp_demuxer_finfo
};


