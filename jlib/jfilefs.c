/*
 * jfilefs.c
 *
 * @chuanjiong
 */

#include "jvfsfs.h"

typedef struct jfilefs_context {
    int fd;
}jfilefs_context;

static jhandle jfilefs_open(const char *url, jvfs_rw rw)
{
    if (url == NULL)
        return NULL;

    jfilefs_context *ctx = (jfilefs_context *)jmalloc(sizeof(jfilefs_context));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(jfilefs_context));

    if (rw == VFS_READ)
    {
        ctx->fd = open(url, O_RDONLY);
    }
    else
    {
        ctx->fd = creat(url, S_IRWXU|S_IRWXG|S_IRWXO);
    }

    if (ctx->fd < 0)
    {
        jfree(ctx);
        return NULL;
    }

    return ctx;
}

static int jfilefs_seek(jhandle h, int64_t pos, int base)
{
    if (h == NULL)
        return ERROR_FAIL;

    jfilefs_context *ctx = (jfilefs_context *)h;

    return lseek(ctx->fd, pos, base);
}

static int64_t jfilefs_tell(jhandle h)
{
    if (h == NULL)
        return -1;

    jfilefs_context *ctx = (jfilefs_context *)h;

    return lseek(ctx->fd, 0, SEEK_CUR);
}

static int64_t jfilefs_read(jhandle h, uint8_t *buf, int64_t size)
{
    if ((h==NULL) || (buf==NULL) || (size<=0))
        return 0;

    jfilefs_context *ctx = (jfilefs_context *)h;

    return read(ctx->fd, buf, size);
}

static int64_t jfilefs_write(jhandle h, uint8_t *buf, int64_t size)
{
    if ((h==NULL) || (buf==NULL) || (size<=0))
        return 0;

    jfilefs_context *ctx = (jfilefs_context *)h;

    return write(ctx->fd, buf, size);
}

static void jfilefs_close(jhandle h)
{
    if (h == NULL)
        return;

    jfilefs_context *ctx = (jfilefs_context *)h;

    close(ctx->fd);

    jfree(ctx);
}

const jvfsfs jfilefs =
{
    jfilefs_open,
    jfilefs_seek,
    jfilefs_tell,
    jfilefs_read,
    jfilefs_write,
    jfilefs_close
};


