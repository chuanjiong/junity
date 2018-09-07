/*
 * jevent.c
 *
 * @chuanjiong
 */

#include "jevent.h"
#include "jatomic.h"
#include "jlist.h"
#include "jsocket.h"
#include "jthread.h"

typedef struct jevent {
    struct list_head list;

    int fd;
    jevent_type type;

    jevent_handle hdl;
    void *arg;

    jevent_clean clean;
    void *carg;

    volatile int del;
}jevent;

typedef struct jevent_context {
    struct list_head inglist;
    struct list_head list;

    fd_set rfds;
    fd_set wfds;
    struct timeval timeout;

    jhandle t;

    pthread_mutex_t mutex;
}jevent_context;

jhandle g_event = NULL;

int jevent_setup(void)
{
    g_event = jevent_alloc();
    if (g_event == NULL)
        return ERROR_FAIL;

    return SUCCESS;
}

void jevent_shutdown(void)
{
    jevent_free(g_event);
    g_event = NULL;
}

static int jevent_step(jevent_context *ctx)
{
    if (ctx == NULL)
        return ERROR_FAIL;

    //list -> inglist
    pthread_mutex_lock(&(ctx->mutex));
    while (!list_empty(&(ctx->list)))
    {
        jevent *evt = list_first_entry(&(ctx->list), jevent, list);
        list_del(&(evt->list));
        list_add_tail(&(evt->list), &(ctx->inglist));
    }
    pthread_mutex_unlock(&(ctx->mutex));

    FD_ZERO(&(ctx->rfds));
    FD_ZERO(&(ctx->wfds));

    int sockMax=-1;
    jevent *evt;
    list_for_each_entry(evt, &(ctx->inglist), list)
    {
        if (evt->type & EVT_TYPE_READ)
        {
            FD_SET(evt->fd, &(ctx->rfds));
            if (sockMax < evt->fd)
                sockMax = evt->fd;
        }

        if (evt->type & EVT_TYPE_WRITE)
        {
            FD_SET(evt->fd, &(ctx->wfds));
            if (sockMax < evt->fd)
                sockMax = evt->fd;
        }
    }

    struct timeval timeout;
    timeout.tv_sec = ctx->timeout.tv_sec;
    timeout.tv_usec = ctx->timeout.tv_usec;

    int ret = select(sockMax+1, &(ctx->rfds), &(ctx->wfds), NULL, &timeout);
    if ((ret<0) && (errno!=EINTR))
        return ERROR_FAIL;
    else if (ret == 0)
        return SUCCESS;

    list_for_each_entry(evt, &(ctx->inglist), list)
    {
        if (evt->type & EVT_TYPE_READ)
        {
            if (FD_ISSET(evt->fd, &(ctx->rfds)))
            {
                if (evt->hdl)
                    if (evt->hdl(evt->fd, EVT_TYPE_READ, evt->arg) != EVT_RET_SUCCESS)
                        jatomic_true(&(evt->del));
            }
        }

        if (evt->type & EVT_TYPE_WRITE)
        {
            if (FD_ISSET(evt->fd, &(ctx->wfds)))
            {
                if (evt->hdl)
                    if (evt->hdl(evt->fd, EVT_TYPE_WRITE, evt->arg) != EVT_RET_SUCCESS)
                        jatomic_true(&(evt->del));
            }
        }
    }

    pthread_mutex_lock(&(ctx->mutex));

retry:
    list_for_each_entry(evt, &(ctx->inglist), list)
    {
        if (jatomic_get(&(evt->del)))
        {
            list_del(&(evt->list));
            if (evt->clean)
                evt->clean(evt->fd, evt->carg);
            jfree(evt);
            goto retry;
        }
    }

    pthread_mutex_unlock(&(ctx->mutex));

    return SUCCESS;
}

static int jevent_routine(jhandle t, void *arg)
{
    if ((t==NULL) || (arg==NULL))
        return ERROR_FAIL;

    while (jthread_is_running(t))
    {
        if (jevent_step((jevent_context *)arg) != SUCCESS)
            break;
    }

    return SUCCESS;
}

jhandle jevent_alloc(void)
{
    jevent_context *ctx = (jevent_context *)jmalloc(sizeof(jevent_context));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(jevent_context));

    INIT_LIST_HEAD(&(ctx->inglist));
    INIT_LIST_HEAD(&(ctx->list));

    ctx->timeout.tv_sec = 0;
    ctx->timeout.tv_usec = 1000000;

    pthread_mutex_init(&(ctx->mutex), NULL);

    ctx->t = jthread_setup(jevent_routine, ctx);
    if (ctx->t == NULL)
    {
        pthread_mutex_destroy(&(ctx->mutex));
        jfree(ctx);
        return NULL;
    }

    return ctx;
}

void jevent_free(jhandle h)
{
    if (h == NULL)
        return;

    jevent_context *ctx = (jevent_context *)h;

    jthread_shutdown(ctx->t);

    while (!list_empty(&(ctx->inglist)))
    {
        jevent *evt = list_first_entry(&(ctx->inglist), jevent, list);
        list_del(&(evt->list));
        if (evt->clean)
            evt->clean(evt->fd, evt->carg);
        jfree(evt);
    }

    while (!list_empty(&(ctx->list)))
    {
        jevent *evt = list_first_entry(&(ctx->list), jevent, list);
        list_del(&(evt->list));
        if (evt->clean)
            evt->clean(evt->fd, evt->carg);
        jfree(evt);
    }

    pthread_mutex_destroy(&(ctx->mutex));

    jfree(ctx);
}

int jevent_add_event(jhandle h, int fd, jevent_type type,
                     jevent_handle hdl, void *arg,
                     jevent_clean clean, void *carg)
{
    if ((h==NULL) || (fd<0) || (hdl==NULL))
        return ERROR_FAIL;

    jevent_context *ctx = (jevent_context *)h;

    jevent *evt = (jevent *)jmalloc(sizeof(jevent));
    if (evt == NULL)
        return ERROR_FAIL;
    memset(evt, 0, sizeof(jevent));

    evt->fd = fd;
    evt->type = type;
    evt->hdl = hdl;
    evt->arg = arg;
    evt->clean = clean;
    evt->carg = carg;

    pthread_mutex_lock(&(ctx->mutex));

    list_add_tail(&(evt->list), &(ctx->list));

    pthread_mutex_unlock(&(ctx->mutex));

    return SUCCESS;
}

int jevent_del_event(jhandle h, int fd)
{
    if ((h==NULL) || (fd<0))
        return ERROR_FAIL;

    jevent_context *ctx = (jevent_context *)h;

    pthread_mutex_lock(&(ctx->mutex));

    jevent *evt;
    list_for_each_entry(evt, &(ctx->list), list)
    {
        if (evt->fd == fd)
        {
            list_del(&(evt->list));
            if (evt->clean)
                evt->clean(evt->fd, evt->carg);
            jfree(evt);
            break;
        }
    }

    list_for_each_entry(evt, &(ctx->inglist), list)
    {
        if (evt->fd == fd)
            jatomic_true(&(evt->del));
    }

    pthread_mutex_unlock(&(ctx->mutex));

    return SUCCESS;
}

void jevent_general_clean(int fd, void *arg)
{
    if (fd < 0)
        return;

    jsocket_close(fd);
}


