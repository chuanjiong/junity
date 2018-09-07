/*
 * jvfsfs.c
 *
 * @chuanjiong
 */

#include "jvfsfs.h"
#include "jlist.h"
#include "jstring.h"

static struct list_head jvfsfs_list;
static pthread_mutex_t jvfsfs_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct jvfsfs_context {
    struct list_head list;

    jstring protocol;
    const jvfsfs *fs;
}jvfsfs_context;

int jvfsfs_setup(void)
{
    INIT_LIST_HEAD(&jvfsfs_list);
    return SUCCESS;
}

void jvfsfs_shutdown(void)
{
    while (!list_empty(&jvfsfs_list))
    {
        jvfsfs_context *ctx = list_first_entry(&jvfsfs_list, jvfsfs_context, list);
        list_del(&(ctx->list));
        jstring_free(ctx->protocol);
        jfree(ctx);
    }
}

int jvfsfs_register_fs(const char *protocol, const jvfsfs *fs)
{
    if ((protocol==NULL) || (fs==NULL))
        return ERROR_FAIL;

    jvfsfs_context *ctx = (jvfsfs_context *)jmalloc(sizeof(jvfsfs_context));
    if (ctx == NULL)
        return ERROR_FAIL;
    memset(ctx, 0, sizeof(jvfsfs_context));

    ctx->protocol = jstring_copy(protocol);
    ctx->fs = fs;

    pthread_mutex_lock(&jvfsfs_mutex);

    list_add_tail(&(ctx->list), &jvfsfs_list);

    pthread_mutex_unlock(&jvfsfs_mutex);

    return SUCCESS;
}

const jvfsfs *jvfsfs_find(const char *protocol)
{
    if (protocol == NULL)
        return NULL;

    const jvfsfs *fs = NULL;

    pthread_mutex_lock(&jvfsfs_mutex);
    if (list_empty(&jvfsfs_list))
    {
        pthread_mutex_unlock(&jvfsfs_mutex);
        return NULL;
    }

    jvfsfs_context *ctx;
    list_for_each_entry(ctx, &jvfsfs_list, list)
    {
        if (!jstring_compare(ctx->protocol, protocol))
        {
            fs = ctx->fs;
            break;
        }
    }

    pthread_mutex_unlock(&jvfsfs_mutex);

    return fs;
}


