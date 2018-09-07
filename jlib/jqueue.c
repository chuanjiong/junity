/*
 * jqueue.c
 *
 * @chuanjiong
 */

#include "jqueue.h"
#include "jlist.h"

typedef struct jqueue_context {
    struct list_head list;
    int count;

    pthread_mutex_t mutex;
    sem_t sem;
}jqueue_context;

typedef struct jqueue_ele {
    struct list_head list;

    uint8_t *data;
    int size;
}jqueue_ele;

jhandle jqueue_alloc(void)
{
    jqueue_context *ctx = (jqueue_context *)jmalloc(sizeof(jqueue_context));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(jqueue_context));

    pthread_mutex_init(&(ctx->mutex), NULL);
    sem_init(&(ctx->sem), 0, 0);

    INIT_LIST_HEAD(&(ctx->list));

    return ctx;
}

void jqueue_free(jhandle h)
{
    if (h == NULL)
        return;

    jqueue_context *ctx = (jqueue_context *)h;

    while (!list_empty(&(ctx->list)))
    {
        jqueue_ele *ele = list_first_entry(&(ctx->list), jqueue_ele, list);
        list_del(&(ele->list));
        ctx->count--;
        jfree(ele);
    }

    sem_destroy(&(ctx->sem));
    pthread_mutex_destroy(&(ctx->mutex));

    jfree(ctx);
}

int jqueue_push(jhandle h, uint8_t *buf, int size)
{
    if ((h==NULL) || (buf==NULL) || (size<=0))
        return ERROR_FAIL;

    jqueue_context *ctx = (jqueue_context *)h;

    jqueue_ele *ele = (jqueue_ele *)jmalloc(sizeof(jqueue_ele));
    if (ele == NULL)
        return ERROR_FAIL;
    memset(ele, 0, sizeof(jqueue_ele));

    ele->data = buf;
    ele->size = size;

    pthread_mutex_lock(&(ctx->mutex));

    list_add_tail(&(ele->list), &(ctx->list));

    ctx->count++;

    pthread_mutex_unlock(&(ctx->mutex));

    return SUCCESS;
}

int jqueue_pop(jhandle h, uint8_t **buf, int *size)
{
    if ((h==NULL) || (buf==NULL))
        return ERROR_FAIL;

    jqueue_context *ctx = (jqueue_context *)h;

    pthread_mutex_lock(&(ctx->mutex));

    if (list_empty(&(ctx->list)))
    {
        pthread_mutex_unlock(&(ctx->mutex));
        return ERROR_FAIL;
    }

    jqueue_ele *ele = list_first_entry(&(ctx->list), jqueue_ele, list);
    list_del(&(ele->list));

    ctx->count--;

    pthread_mutex_unlock(&(ctx->mutex));

    *buf = ele->data;
    if (size)
        *size = ele->size;

    jfree(ele);

    return SUCCESS;
}

int jqueue_size(jhandle h)
{
    if (h == NULL)
        return -1;

    jqueue_context *ctx = (jqueue_context *)h;

    int size;

    pthread_mutex_lock(&(ctx->mutex));

    size = ctx->count;

    pthread_mutex_unlock(&(ctx->mutex));

    return size;
}


