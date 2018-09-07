/*
 * jvfs.c
 *
 * @chuanjiong
 */

#include "jvfs.h"
#include "jvfsfs.h"
#include "jprocfs.h"
#include "jstring.h"

typedef struct jvfs_context {
    jvfs_rw rw;
    const jvfsfs *fs;
    jhandle h;
}jvfs_context;

int jvfs_setup(void)
{
    jvfsfs_setup();

    jprocfs_setup();

    extern const jvfsfs jfilefs;
    jvfsfs_register_fs("file://", &jfilefs);
    extern const jvfsfs judpfs;
    jvfsfs_register_fs("udp://", &judpfs);

    return SUCCESS;
}

void jvfs_shutdown(void)
{
    jprocfs_shutdown();

    jvfsfs_shutdown();
}

jhandle jvfs_open(const char *url, jbool write)
{
    if (url == NULL)
        return NULL;

    jvfs_context *ctx = (jvfs_context *)jmalloc(sizeof(jvfs_context));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(jvfs_context));

    ctx->rw = write ? VFS_WRITE : VFS_READ;

    jstring protocol = jstring_protocol(url);
    ctx->fs = jvfsfs_find(tostring(protocol));
    if (ctx->fs == NULL)
    {
        jstring_free(protocol);
        jfree(ctx);
        return NULL;
    }
    jstring_free(protocol);

    ctx->h = ctx->fs->vfs_open(url, ctx->rw);
    if (ctx->h == NULL)
    {
        jfree(ctx);
        return NULL;
    }

    return ctx;
}

int jvfs_seek(jhandle h, int64_t pos, int base)
{
    if (h == NULL)
        return ERROR_FAIL;

    jvfs_context *ctx = (jvfs_context *)h;

    return ctx->fs->vfs_seek(ctx->h, pos, base);
}

int64_t jvfs_tell(jhandle h)
{
    if (h == NULL)
        return -1;

    jvfs_context *ctx = (jvfs_context *)h;

    return ctx->fs->vfs_tell(ctx->h);
}

int64_t jvfs_read(jhandle h, uint8_t *buf, int64_t size)
{
    if ((h==NULL) || (buf==NULL) || (size<=0))
        return 0;

    jvfs_context *ctx = (jvfs_context *)h;

    if (ctx->rw != VFS_READ)
        return 0;

    return ctx->fs->vfs_read(ctx->h, buf, size);
}

int64_t jvfs_write(jhandle h, uint8_t *buf, int64_t size)
{
    if ((h==NULL) || (buf==NULL) || (size<=0))
        return 0;

    jvfs_context *ctx = (jvfs_context *)h;

    if (ctx->rw != VFS_WRITE)
        return 0;

    return ctx->fs->vfs_write(ctx->h, buf, size);
}

void jvfs_close(jhandle h)
{
    if (h == NULL)
        return;

    jvfs_context *ctx = (jvfs_context *)h;

    ctx->fs->vfs_close(ctx->h);

    jfree(ctx);
}


