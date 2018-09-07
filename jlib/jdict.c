/*
 * jdict.c
 *
 * @chuanjiong
 */

#include "jdict.h"
#include "jlist.h"

typedef struct jdict_context {
    struct list_head list;
    pthread_mutex_t mutex;
    int total;
}jdict_context;

typedef struct jdict_item {
    struct list_head list;

    jstring key;
    jdict_value value;
}jdict_item;

jhandle jdict_alloc(void)
{
    jdict_context *ctx = (jdict_context *)jmalloc(sizeof(jdict_context));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(jdict_context));

    pthread_mutex_init(&(ctx->mutex), NULL);

    INIT_LIST_HEAD(&(ctx->list));

    return ctx;
}

void jdict_free(jhandle h)
{
    if (h == NULL)
        return;

    jdict_context *ctx = (jdict_context *)h;

    while (!list_empty(&(ctx->list)))
    {
        jdict_item *item = list_first_entry(&(ctx->list), jdict_item, list);
        list_del(&(item->list));
        jstring_free(item->key);
        if (item->value.type == DICT_VALUE_TYPE_STRING)
            jstring_free(item->value.s);
        jfree(item);
    }

    pthread_mutex_destroy(&(ctx->mutex));

    jfree(ctx);
}

int jdict_set_value(jhandle h, const char *key, jdict_value value)
{
    if ((h==NULL) || (key==NULL))
        return ERROR_FAIL;

    jdict_context *ctx = (jdict_context *)h;

    jdict_item *item;

    pthread_mutex_lock(&(ctx->mutex));

    list_for_each_entry(item, &(ctx->list), list)
    {
        if (!jstring_compare(item->key, key))
        {
            if (item->value.type == DICT_VALUE_TYPE_STRING)
                jstring_free(item->value.s);
            item->value = value;
            pthread_mutex_unlock(&(ctx->mutex));
            return SUCCESS;
        }
    }

    pthread_mutex_unlock(&(ctx->mutex));

    item = (jdict_item *)jmalloc(sizeof(jdict_item));
    if (item == NULL)
        return ERROR_FAIL;
    memset(item, 0, sizeof(jdict_item));

    item->key = jstring_copy(key);
    item->value = value;

    pthread_mutex_lock(&(ctx->mutex));

    list_add_tail(&(item->list), &(ctx->list));

    ctx->total++;

    pthread_mutex_unlock(&(ctx->mutex));

    return SUCCESS;
}

jdict_value jdict_get_value(jhandle h, const char *key)
{
    jdict_value value = {0};

    if ((h==NULL) || (key==NULL))
        return value;

    jdict_context *ctx = (jdict_context *)h;

    pthread_mutex_lock(&(ctx->mutex));

    jdict_item *item;
    list_for_each_entry(item, &(ctx->list), list)
    {
        if (!jstring_compare(item->key, key))
        {
            value = item->value;
            break;
        }
    }

    pthread_mutex_unlock(&(ctx->mutex));

    return value;
}

int jdict_get_capacity(jhandle h)
{
    if (h == NULL)
        return 0;

    jdict_context *ctx = (jdict_context *)h;

    int total;

    pthread_mutex_lock(&(ctx->mutex));

    total = ctx->total;

    pthread_mutex_unlock(&(ctx->mutex));

    return total;
}

int jdict_get_value_by_index(jhandle h, int index, jstring *key, jdict_value *value)
{
    if ((h==NULL) || (index<0))
        return ERROR_FAIL;

    jdict_context *ctx = (jdict_context *)h;

    pthread_mutex_lock(&(ctx->mutex));

    if (index >= ctx->total)
    {
        pthread_mutex_unlock(&(ctx->mutex));
        return ERROR_FAIL;
    }

    int idx = 0;
    jdict_item *item;
    list_for_each_entry(item, &(ctx->list), list)
    {
        if (idx == index)
        {
            if (key)
                *key = item->key;
            if (value)
                *value = item->value;
            break;
        }
        idx++;
    }

    pthread_mutex_unlock(&(ctx->mutex));

    return SUCCESS;
}


