/*
 * jmm_source.c
 *
 * @chuanjiong
 */

#include "jmm_source.h"

typedef struct jmm_source_ctx {
    jhandle demuxer;
    jstring url;
    jbool net;
    jbool loop;

    jhandle aq;
    jhandle vq;

    jhandle ct;
    jhandle at;
    jhandle vt;

    int a_reset_ts;
    int v_reset_ts;

    jtime a_start_time;
    int64_t a_start_dts;

    jtime v_start_time;
    int64_t v_start_dts;

    jmm_source_callback cb;
    void *arg;

    pthread_mutex_t mutex;
}jmm_source_ctx;

static int jmm_source_c_thread(jhandle t, void *arg)
{
    if ((t==NULL) || (arg==NULL))
        return ERROR_FAIL;

    jmm_source_ctx *ctx = (jmm_source_ctx *)arg;

    int64_t prev_ts, base_time = 0;

loop:
    while (jthread_is_running(t))
    {
        jmm_packet *pkt = jmm_demuxer_read(ctx->demuxer);
        if (pkt == NULL)
            break;

        if (!ctx->net)
        {
            pkt->pts += base_time;
            pkt->dts += base_time;
        }
        prev_ts = pkt->dts;

        if (ctx->net)
        {
            ctx->cb(ctx->arg, pkt);
            jmm_packet_free(pkt);
        }
        else
        {
            if (pkt->type == JMM_CODEC_TYPE_AAC)
            {
                while ((jqueue_size(ctx->aq) > 10) && jthread_is_running(t))
                    jsleep(10);
                jqueue_push(ctx->aq, (uint8_t *)pkt, sizeof(jmm_packet));
            }
            else if (pkt->type == JMM_CODEC_TYPE_AVC)
            {
                while ((jqueue_size(ctx->vq) > 5) && jthread_is_running(t))
                    jsleep(10);
                jqueue_push(ctx->vq, (uint8_t *)pkt, sizeof(jmm_packet));
            }
            else
            {
                jmm_packet_free(pkt);
            }
        }
    }

    base_time = prev_ts;

    if (ctx->loop && jthread_is_running(t))
    {
        if (ctx->net)
        {
            jmm_demuxer_close(ctx->demuxer);
            jsleep(2000);
            jlog("[jmm_source] re-connect %s\n", tostring(ctx->url));
            ctx->demuxer = jmm_demuxer_open(tostring(ctx->url));
        }
        else
        {
            jlog("[jmm_source] loop media %s\n", tostring(ctx->url));
            jmm_demuxer_seek(ctx->demuxer, 0);
        }
        goto loop;
    }

    return SUCCESS;
}

static int jmm_source_a_thread(jhandle t, void *arg)
{
    if ((t==NULL) || (arg==NULL))
        return ERROR_FAIL;

    jmm_source_ctx *ctx = (jmm_source_ctx *)arg;

    ctx->a_reset_ts = 1;

    while (jthread_is_running(t))
    {
        jmm_packet *pkt = NULL;
        jqueue_pop(ctx->aq, (uint8_t **)&pkt, NULL);
        if (pkt == NULL)
        {
            jsleep(10);
            continue;
        }

        if (ctx->a_reset_ts == 1)
        {
            ctx->a_reset_ts = 0;
            ctx->a_start_time = jtime_set_anchor();
            ctx->a_start_dts = pkt->dts;
        }

        int64_t period = jtime_to_us(jtime_get_period(ctx->a_start_time));
        if (((pkt->dts-ctx->a_start_dts)-period) >= 5000)
        {
            jsleep(((pkt->dts-ctx->a_start_dts)-period)/1000);
        }

        pthread_mutex_lock(&(ctx->mutex));
        ctx->cb(ctx->arg, pkt);
        pthread_mutex_unlock(&(ctx->mutex));

        jmm_packet_free(pkt);
    }

    return SUCCESS;
}

static int jmm_source_v_thread(jhandle t, void *arg)
{
    if ((t==NULL) || (arg==NULL))
        return ERROR_FAIL;

    jmm_source_ctx *ctx = (jmm_source_ctx *)arg;

    ctx->v_reset_ts = 1;

    while (jthread_is_running(t))
    {
        jmm_packet *pkt = NULL;
        jqueue_pop(ctx->vq, (uint8_t **)&pkt, NULL);
        if (pkt == NULL)
        {
            jsleep(10);
            continue;
        }

        if (ctx->v_reset_ts == 1)
        {
            ctx->v_reset_ts = 0;
            ctx->v_start_time = jtime_set_anchor();
            ctx->v_start_dts = pkt->dts;
        }

        int64_t period = jtime_to_us(jtime_get_period(ctx->v_start_time));
        if (((pkt->dts-ctx->v_start_dts)-period) >= 5000)
        {
            jsleep(((pkt->dts-ctx->v_start_dts)-period)/1000);
        }

        pthread_mutex_lock(&(ctx->mutex));
        ctx->cb(ctx->arg, pkt);
        pthread_mutex_unlock(&(ctx->mutex));

        jmm_packet_free(pkt);
    }

    return SUCCESS;
}

jhandle jmm_source_setup(const char *url, jbool net, jbool loop, jmm_source_callback cb, void *arg)
{
    if ((url==NULL) || (cb==NULL))
        return NULL;

    jmm_source_ctx *ctx = (jmm_source_ctx *)jmalloc(sizeof(jmm_source_ctx));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(jmm_source_ctx));

    ctx->demuxer = jmm_demuxer_open(url);
    if (ctx->demuxer == NULL)
    {
        if (!net)
        {
            jerr("[jmm_source] open file %s fail\n", url);
            jfree(ctx);
            return NULL;
        }
    }

    ctx->url = jstring_copy(url);
    ctx->net = net;
    ctx->loop = loop;
    ctx->cb = cb;
    ctx->arg = arg;
    pthread_mutex_init(&(ctx->mutex), NULL);

    if (!net)
    {
        ctx->aq = jqueue_alloc();
        ctx->vq = jqueue_alloc();
        ctx->at = jthread_setup(jmm_source_a_thread, ctx);
        ctx->vt = jthread_setup(jmm_source_v_thread, ctx);
    }
    ctx->ct = jthread_setup(jmm_source_c_thread, ctx);
    return ctx;
}

void jmm_source_shutdown(jhandle h)
{
    if (h == NULL)
        return;

    jmm_source_ctx *ctx = (jmm_source_ctx *)h;

    jthread_shutdown(ctx->vt);
    jthread_shutdown(ctx->at);
    jthread_shutdown(ctx->ct);

    jmm_packet *pkt;
    while (jqueue_pop(ctx->vq, (uint8_t **)&pkt, NULL) == SUCCESS)
        jmm_packet_free(pkt);
    jqueue_free(ctx->vq);
    while (jqueue_pop(ctx->aq, (uint8_t **)&pkt, NULL) == SUCCESS)
        jmm_packet_free(pkt);
    jqueue_free(ctx->aq);

    jmm_demuxer_close(ctx->demuxer);
    jstring_free(ctx->url);
    pthread_mutex_destroy(&(ctx->mutex));

    jfree(ctx);
}


