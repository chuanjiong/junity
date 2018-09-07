/*
 * rtsp_server.c
 *
 * @chuanjiong
 */

#include "rtsp_server.h"
#include "rtsp_response.h"

typedef struct rtsp_session_context {
    jstring session_id;
    jstring url[2];
    jstring transport[2];
    int media_idx;
    int sink_idx;
    jsocket sock;
    int used;
}rtsp_session_context;

typedef struct rtsp_media_context {
    jstring path;
    jhandle media;
    int used;
}rtsp_media_context;

typedef struct rtsp_server_context {
    jsocket sock;

    rtsp_session_context session_pool[RTSP_SERVER_MAX_SESSION];
    rtsp_media_context media_pool[RTSP_SERVER_MAX_MEDIA_SOURCE];

    pthread_mutex_t mutex;
}rtsp_server_context;

typedef struct rtsp_server_rr_context {
    uint8_t request[RTSP_SERVER_REQ_RES_BUF_SIZE];
    uint8_t req_size;

    uint8_t response[RTSP_SERVER_REQ_RES_BUF_SIZE];

    rtsp_server_context *ctx;
}rtsp_server_rr_context;

static jstring rtsp_server_session_rand()
{
    int i;
    uint8_t id[8];

    static int srand_flag = 0;
    if (srand_flag == 0)
    {
        srand_flag = 1;
        srand(time(NULL));
    }
    for (i=0; i<8; i++)
        id[i] = (rand()%10)+0x30;

    char buf[9] = {0};
    for (i=0; i<8; i++)
        sprintf(buf+i, "%c", id[i]);

    return jstring_copy(buf);
}

static int rtsp_server_session_find(rtsp_server_context *ctx, jstring session)
{
    if ((ctx==NULL) || (stringsize(session)<=0))
        return -1;

    int i;
    for (i=0; i<RTSP_SERVER_MAX_SESSION; i++)
    {
        if (ctx->session_pool[i].used)
        {
            if (!strcmp(tostring(ctx->session_pool[i].session_id), tostring(session)))
            {
                return i;
            }
        }
    }

    return -1;
}

static int rtsp_server_session_find_by_sock(rtsp_server_context *ctx, jsocket sock)
{
    if ((ctx==NULL) || (sock<0))
        return -1;

    int i;
    for (i=0; i<RTSP_SERVER_MAX_SESSION; i++)
    {
        if (ctx->session_pool[i].used)
        {
            if (ctx->session_pool[i].sock == sock)
            {
                return i;
            }
        }
    }

    return -1;
}

static int rtsp_server_get_media_idx(rtsp_server_context *ctx, const char *url)
{
    if ((ctx==NULL) || (url==NULL))
        return -1;

    int i;

    pthread_mutex_lock(&(ctx->mutex));

    for (i=0; i<RTSP_SERVER_MAX_MEDIA_SOURCE; i++)
    {
        if (ctx->media_pool[i].used && (!strcmp(url, tostring(ctx->media_pool[i].path))))
        {
            break;
        }
    }
    if (i == RTSP_SERVER_MAX_MEDIA_SOURCE)
        i = -1;

    pthread_mutex_unlock(&(ctx->mutex));

    return i;
}

static jevent_ret rtsp_server_handle_request(rtsp_server_context *ctx, char *request, char *response, int fd)
{
    if ((ctx==NULL) || (request==NULL) || (response==NULL) || (fd<0))
        return EVT_RET_REMOVE;

    int cseq = jstring_pick_value(request, "CSeq: ", "\r\n");
    if (cseq < 0)
        return EVT_RET_SUCCESS;

    if (!strncmp(request, "OPTIONS", strlen("OPTIONS")))
    {
        jsnprintf(response, RTSP_SERVER_REQ_RES_BUF_SIZE, RESPONSE_OPTIONS, cseq);
    }
    else if (!strncmp(request, "DESCRIBE", strlen("DESCRIBE")))
    {
        jstring str = jstring_pick(request, "Accept: ", "\r\n");
        if (strstr(tostring(str), "application/sdp"))
        {
            //url: rtsp://ip[:port][path]
            jstring url = jstring_pick(request, "DESCRIBE ", " RTSP/1.0\r\n");
            if (strchr(tostring(url)+7, '/'))
            {
                jstring path = jstring_copy(strchr(tostring(url)+7, '/'));
                int i = rtsp_server_get_media_idx(ctx, tostring(path));
                if (i >= 0)
                {
                    jdate date = jdate_get_UTC();
                    char date_buf[32] = {0};
                    jdate_format_date(date, date_buf, 32, DATEFORMAT_W_DMY_HMS_GMT);
                    char *sdp = media_source_get_sdp(ctx->media_pool[i].media);
                    if (sdp != NULL)
                    {
                        jsnprintf(response, RTSP_SERVER_REQ_RES_BUF_SIZE, RESPONSE_DESCRIBE,
                            date_buf, tostring(url), strlen(sdp), cseq, sdp);
                        jfree(sdp);
                    }
                }
                jstring_free(path);
            }
            jstring_free(url);
        }
        jstring_free(str);
    }
    else if (!strncmp(request, "SETUP", strlen("SETUP")))
    {
        jstring url = jstring_pick(request, "SETUP ", " RTSP/1.0\r\n");
        jstring transport = jstring_pick(request, "Transport: ", "\r\n");
        jstring session = jstring_pick_1st_word(request, "Session: ", "\r\n");
        if (stringsize(session) > 0)
        {
            int i = rtsp_server_session_find(ctx, session);
            if (i >= 0)
            {
                ctx->session_pool[i].url[1] = url;
                ctx->session_pool[i].transport[1] = transport;
                jdate date = jdate_get_UTC();
                char date_buf[32] = {0};
                jdate_format_date(date, date_buf, 32, DATEFORMAT_W_DMY_HMS_GMT);
                jsnprintf(response, RTSP_SERVER_REQ_RES_BUF_SIZE, RESPONSE_SETUP,
                    date_buf, tostring(transport), tostring(ctx->session_pool[i].session_id), cseq);
            }
        }
        else
        {
            int i;
            for (i=0; i<RTSP_SERVER_MAX_SESSION; i++)
            {
                if (ctx->session_pool[i].used == 0)
                {
                    ctx->session_pool[i].used = 1;
                    ctx->session_pool[i].session_id = rtsp_server_session_rand();
                    ctx->session_pool[i].url[0] = url;
                    ctx->session_pool[i].transport[0] = transport;
                    ctx->session_pool[i].sock = fd;
                    jdate date = jdate_get_UTC();
                    char date_buf[32] = {0};
                    jdate_format_date(date, date_buf, 32, DATEFORMAT_W_DMY_HMS_GMT);
                    jsnprintf(response, RTSP_SERVER_REQ_RES_BUF_SIZE, RESPONSE_SETUP,
                        date_buf, tostring(transport), tostring(ctx->session_pool[i].session_id), cseq);
                    //jlog("add session: %s\n", tostring(ctx->session_pool[i].session_id));
                    break;
                }
            }
        }
        jstring_free(session);
    }
    else if (!strncmp(request, "PLAY", strlen("PLAY")))
    {
        int media_idx, session_idx;
        jstring url = jstring_pick(request, "PLAY ", " RTSP/1.0\r\n");
        if (strchr(tostring(url)+7, '/'))
        {
            jstring path = jstring_copy(strchr(tostring(url)+7, '/'));
            media_idx = rtsp_server_get_media_idx(ctx, tostring(path));
            jstring session = jstring_pick_1st_word(request, "Session: ", "\r\n");
            session_idx = rtsp_server_session_find(ctx, session);
            if ((media_idx>=0) && (session_idx>=0))
            {
                ctx->session_pool[session_idx].media_idx = media_idx;
                jdate date = jdate_get_UTC();
                char date_buf[32] = {0};
                jdate_format_date(date, date_buf, 32, DATEFORMAT_W_DMY_HMS_GMT);
                jsnprintf(response, RTSP_SERVER_REQ_RES_BUF_SIZE, RESPONSE_PLAY, date_buf,
                    tostring(ctx->session_pool[session_idx].url[0]),
                    media_source_get_time(ctx->media_pool[media_idx].media,
                        ctx->session_pool[session_idx].url[0]),
                    tostring(ctx->session_pool[session_idx].url[1]),
                    media_source_get_time(ctx->media_pool[media_idx].media,
                        ctx->session_pool[session_idx].url[1]),
                    tostring(ctx->session_pool[session_idx].session_id), cseq);

                struct sockaddr_in sa;
                socklen_t len = sizeof(sa);
                if (getpeername(fd, (struct sockaddr *)&sa, &len) == 0)
                {
                    ctx->session_pool[session_idx].sink_idx =
                        media_source_add_sink(ctx->media_pool[media_idx].media,
                        ctx->session_pool[session_idx].url,
                        inet_ntoa(sa.sin_addr), fd,
                        ctx->session_pool[session_idx].transport);
                }
            }
            jstring_free(session);
            jstring_free(path);
        }
        jstring_free(url);
    }
    else if (!strncmp(request, "PAUSE", strlen("PAUSE")))
    {
        jdate date = jdate_get_UTC();
        char date_buf[32] = {0};
        jdate_format_date(date, date_buf, 32, DATEFORMAT_W_DMY_HMS_GMT);
        jsnprintf(response, RTSP_SERVER_REQ_RES_BUF_SIZE, RESPONSE_PAUSE, date_buf, cseq);
    }
    else if (!strncmp(request, "TEARDOWN", strlen("TEARDOWN")))
    {
        jstring session = jstring_pick_1st_word(request, "Session: ", "\r\n");
        int session_idx = rtsp_server_session_find(ctx, session);
        media_source_del_sink(ctx->media_pool[ctx->session_pool[session_idx].media_idx].media,
            ctx->session_pool[session_idx].sink_idx);
        ctx->session_pool[session_idx].session_id = jstring_free(ctx->session_pool[session_idx].session_id);
        ctx->session_pool[session_idx].url[0] = jstring_free(ctx->session_pool[session_idx].url[0]);
        ctx->session_pool[session_idx].url[1] = jstring_free(ctx->session_pool[session_idx].url[1]);
        ctx->session_pool[session_idx].transport[0] = jstring_free(ctx->session_pool[session_idx].transport[0]);
        ctx->session_pool[session_idx].transport[1] = jstring_free(ctx->session_pool[session_idx].transport[1]);
        ctx->session_pool[session_idx].used = 0;
        jsnprintf(response, RTSP_SERVER_REQ_RES_BUF_SIZE, RESPONSE_TEARDOWN, cseq);
        jstring_free(session);
    }
    else
    {
        jerr("[rtsp_server] unknown request method\n");
    }

    if (strlen(response) == 0)
    {
        jsnprintf(response, RTSP_SERVER_REQ_RES_BUF_SIZE, RESPONSE_ERROR, cseq);
        return EVT_RET_REMOVE;
    }

    return EVT_RET_SUCCESS;
}

static jevent_ret rtsp_server_recv_request(int fd, jevent_type type, void *arg)
{
    if ((fd<0) || (type!=EVT_TYPE_READ) || (arg==NULL))
        return EVT_RET_REMOVE;

    rtsp_server_rr_context *rr = (rtsp_server_rr_context *)arg;
    rtsp_server_context *ctx = rr->ctx;

    //read complete request
    int size = tcp_read(fd, &(rr->request[rr->req_size]), RTSP_SERVER_REQ_RES_BUF_SIZE-rr->req_size);
    if ((size<=0) && (errno!=EINTR) && (errno!=EAGAIN))
    {
        jerr("[rtsp_server] read request fail\n");
        int session_idx = rtsp_server_session_find_by_sock(ctx, fd);
        if (session_idx >= 0)
        {
            media_source_del_sink(ctx->media_pool[ctx->session_pool[session_idx].media_idx].media,
            ctx->session_pool[session_idx].sink_idx);
            ctx->session_pool[session_idx].session_id = jstring_free(ctx->session_pool[session_idx].session_id);
            ctx->session_pool[session_idx].url[0] = jstring_free(ctx->session_pool[session_idx].url[0]);
            ctx->session_pool[session_idx].url[1] = jstring_free(ctx->session_pool[session_idx].url[1]);
            ctx->session_pool[session_idx].transport[0] = jstring_free(ctx->session_pool[session_idx].transport[0]);
            ctx->session_pool[session_idx].transport[1] = jstring_free(ctx->session_pool[session_idx].transport[1]);
            ctx->session_pool[session_idx].used = 0;
            jsleep(1000);
        }
        return EVT_RET_REMOVE;
    }
    if (size > 0)
        rr->req_size += size;
    if (rr->req_size <= 0)
        return EVT_RET_SUCCESS;

loop:
    //skip '$' packet -- tcp mode rtcp packet
    if (rr->request[0] == '$')
    {
        if (rr->req_size < 4)
            return EVT_RET_SUCCESS;
        int psize = (rr->request[2]<<8) | rr->request[3];
        if (rr->req_size < (psize+4))
            return EVT_RET_SUCCESS;
        int left = rr->req_size - psize - 4;
        if (left > 0)
            memmove(rr->request, &(rr->request[psize+4]), left);
        rr->req_size = left;
        memset(&(rr->request[left]), 0, RTSP_SERVER_REQ_RES_BUF_SIZE-left);
        if (left > 0)
            goto loop;
        return EVT_RET_SUCCESS;
    }
    else if (strstr((const char *)(rr->request), "\r\n\r\n") == NULL)
        return EVT_RET_SUCCESS;

    //jlog("[rtsp_server] request: \n\n%s\n\n", rr->request);

    jevent_ret ret = rtsp_server_handle_request(ctx, (char *)(rr->request), (char *)(rr->response), fd);
    if (ret == EVT_RET_REMOVE)
    {
        jerr("[rtsp_server] handle request fail\n");
        int session_idx = rtsp_server_session_find_by_sock(ctx, fd);
        if (session_idx >= 0)
        {
            media_source_del_sink(ctx->media_pool[ctx->session_pool[session_idx].media_idx].media,
            ctx->session_pool[session_idx].sink_idx);
            ctx->session_pool[session_idx].session_id = jstring_free(ctx->session_pool[session_idx].session_id);
            ctx->session_pool[session_idx].url[0] = jstring_free(ctx->session_pool[session_idx].url[0]);
            ctx->session_pool[session_idx].url[1] = jstring_free(ctx->session_pool[session_idx].url[1]);
            ctx->session_pool[session_idx].transport[0] = jstring_free(ctx->session_pool[session_idx].transport[0]);
            ctx->session_pool[session_idx].transport[1] = jstring_free(ctx->session_pool[session_idx].transport[1]);
            ctx->session_pool[session_idx].used = 0;
            jsleep(1000);
        }
        return EVT_RET_REMOVE;
    }

    //jlog("[rtsp_server] response: \n\n%s\n\n", rr->response);

    if (strlen((char *)(rr->response)) > 0)
        if (tcp_write(fd, rr->response, strlen((char *)(rr->response))) != strlen((char *)(rr->response)))
        {
            jerr("[rtsp_server] write response fail\n");
            int session_idx = rtsp_server_session_find_by_sock(ctx, fd);
            if (session_idx >= 0)
            {
                media_source_del_sink(ctx->media_pool[ctx->session_pool[session_idx].media_idx].media,
                ctx->session_pool[session_idx].sink_idx);
                ctx->session_pool[session_idx].session_id = jstring_free(ctx->session_pool[session_idx].session_id);
                ctx->session_pool[session_idx].url[0] = jstring_free(ctx->session_pool[session_idx].url[0]);
                ctx->session_pool[session_idx].url[1] = jstring_free(ctx->session_pool[session_idx].url[1]);
                ctx->session_pool[session_idx].transport[0] = jstring_free(ctx->session_pool[session_idx].transport[0]);
                ctx->session_pool[session_idx].transport[1] = jstring_free(ctx->session_pool[session_idx].transport[1]);
                ctx->session_pool[session_idx].used = 0;
                jsleep(1000);
            }
            return EVT_RET_REMOVE;
        }

    const char *p = strstr((const char *)(rr->request), "\r\n\r\n");
    if (p)
    {
        p += 4;
        int left = rr->req_size - (p - (const char *)(rr->request));
        if (left >= 0)
        {
            if (left > 0)
                memmove(rr->request, p, left);
            rr->req_size = left;
            memset(&(rr->request[left]), 0, RTSP_SERVER_REQ_RES_BUF_SIZE-left);
        }
    }

    memset(rr->response, 0, RTSP_SERVER_REQ_RES_BUF_SIZE);
    return EVT_RET_SUCCESS;
}

static void rtsp_server_clean(int fd, void *arg)
{
    if ((fd<0) || (arg==NULL))
        return;

    rtsp_server_rr_context *ctx = (rtsp_server_rr_context *)arg;

    jfree(ctx);

    jsocket_close(fd);
}

static jevent_ret rtsp_server_accept_client(int fd, jevent_type type, void *arg)
{
    if ((fd<0) || (type!=EVT_TYPE_READ) || (arg==NULL))
        return EVT_RET_REMOVE;

    jsocket sock = tcp_accept(fd);
    if (sock < 0)
        return EVT_RET_REMOVE;

    rtsp_server_rr_context *ctx = (rtsp_server_rr_context *)jmalloc(sizeof(rtsp_server_rr_context));
    if (ctx == NULL)
    {
        jsocket_close(sock);
        return EVT_RET_REMOVE;
    }
    memset(ctx, 0, sizeof(rtsp_server_rr_context));

    ctx->ctx = (rtsp_server_context *)arg;

    jevent_add_event(g_event, sock, EVT_TYPE_READ,
        rtsp_server_recv_request, ctx, rtsp_server_clean, ctx);

    return EVT_RET_SUCCESS;
}

jhandle rtsp_server_setup(int port)
{
    if (port < 0)
        return NULL;

    rtsp_server_context *ctx = (rtsp_server_context *)jmalloc(sizeof(rtsp_server_context));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(rtsp_server_context));

    ctx->sock = tcp_listen(port);
    if (ctx->sock < 0)
    {
        jerr("[rtsp_server] listen fail\n");
        jfree(ctx);
        return NULL;
    }

    pthread_mutex_init(&(ctx->mutex), NULL);

    jevent_add_event(g_event, ctx->sock, EVT_TYPE_READ,
        rtsp_server_accept_client, ctx, NULL, NULL);

    return ctx;
}

int rtsp_server_add_media(jhandle h, const char *url, media_source_config *cfg)
{
    if ((h==NULL) || (url==NULL) || (cfg==NULL))
        return ERROR_FAIL;

    rtsp_server_context *ctx = (rtsp_server_context *)h;

    int i = rtsp_server_get_media_idx(ctx, url);
    if (i >= 0)
        return ERROR_FAIL;

    pthread_mutex_lock(&(ctx->mutex));

    for (i=0; i<RTSP_SERVER_MAX_MEDIA_SOURCE; i++)
    {
        if (ctx->media_pool[i].used == 0)
        {
            ctx->media_pool[i].path = jstring_copy(url);
            ctx->media_pool[i].media = media_source_setup(cfg);
            ctx->media_pool[i].used = 1;
            break;
        }
    }

    if (i == RTSP_SERVER_MAX_MEDIA_SOURCE)
    {
        pthread_mutex_unlock(&(ctx->mutex));
        return ERROR_FAIL;
    }

    pthread_mutex_unlock(&(ctx->mutex));
    return SUCCESS;
}

int rtsp_server_del_media(jhandle h, const char *url)
{
    if ((h==NULL) || (url==NULL))
        return ERROR_FAIL;

    rtsp_server_context *ctx = (rtsp_server_context *)h;

    int i = rtsp_server_get_media_idx(ctx, url);
    if (i < 0)
        return ERROR_FAIL;

    pthread_mutex_lock(&(ctx->mutex));

    if (ctx->media_pool[i].used == 1)
    {
        media_source_shutdown(ctx->media_pool[i].media);
        jstring_free(ctx->media_pool[i].path);
        ctx->media_pool[i].used = 0;
    }

    pthread_mutex_unlock(&(ctx->mutex));
    return SUCCESS;
}

int rtsp_server_get_callback(jhandle h, const char *url, media_source_callback *cb, jhandle *cb_arg)
{
    if ((h==NULL) || (url==NULL))
        return ERROR_FAIL;

    rtsp_server_context *ctx = (rtsp_server_context *)h;

    int idx = rtsp_server_get_media_idx(ctx, url);

    if (cb)
        *cb = media_source_send_packet;
    if (cb_arg)
        *cb_arg = ctx->media_pool[idx].media;

    return SUCCESS;
}

void rtsp_server_shutdown(jhandle h)
{
    if (h == NULL)
        return;

    rtsp_server_context *ctx = (rtsp_server_context *)h;

    jevent_del_event(g_event, ctx->sock);

    jsocket_close(ctx->sock);

    int i;
    for (i=0; i<RTSP_SERVER_MAX_MEDIA_SOURCE; i++)
    {
        if (ctx->media_pool[i].used)
        {
            media_source_shutdown(ctx->media_pool[i].media);
            jstring_free(ctx->media_pool[i].path);
        }
    }

    pthread_mutex_destroy(&(ctx->mutex));

    jfree(ctx);
}


