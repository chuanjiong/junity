/*
 * judpfs.c
 *
 * @chuanjiong
 */

#include "jvfsfs.h"
#include "jstring.h"
#include "jsocket.h"

typedef struct judpfs_context {
    jsocket sock;
}judpfs_context;

static jhandle judpfs_open(const char *url, jvfs_rw rw)
{
    if ((url==NULL) || (rw==VFS_READ))
        return NULL;

    judpfs_context *ctx = (judpfs_context *)jmalloc(sizeof(judpfs_context));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(judpfs_context));

    //udp://ip:port
    //udp://@ip:port
    jstring ip = jstring_pick(url, "udp://@", ":");
    if (stringsize(ip) <= 0)
        ip = jstring_pick(url, "udp://", ":");

    const char *a = strstr(url+strlen("udp://"), ":");
    jstring temp = jstring_copy(a+1);
    int port = atoi(tostring(temp));
    jstring_free(temp);

    ctx->sock = udp_connect(tostring(ip), port);
    return ctx;
}

static int judpfs_seek(jhandle h, int64_t pos, int base)
{
    return ERROR_FAIL;
}

static int64_t judpfs_tell(jhandle h)
{
    return -1;
}

static int64_t judpfs_read(jhandle h, uint8_t *buf, int64_t size)
{
    return 0;
}

static int64_t judpfs_write(jhandle h, uint8_t *buf, int64_t size)
{
    if ((h==NULL) || (buf==NULL) || (size<=0))
        return 0;

    judpfs_context *ctx = (judpfs_context *)h;

    return udp_write(ctx->sock, buf, size);
}

static void judpfs_close(jhandle h)
{
    if (h == NULL)
        return;

    judpfs_context *ctx = (judpfs_context *)h;

    jsocket_close(ctx->sock);

    jfree(ctx);
}

const jvfsfs judpfs =
{
    judpfs_open,
    judpfs_seek,
    judpfs_tell,
    judpfs_read,
    judpfs_write,
    judpfs_close
};


