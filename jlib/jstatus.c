/*
 * jstatus.c
 *
 * @chuanjiong
 */

#include "jstatus.h"
#include "jevent.h"
#include "jsocket.h"
#include "jstring.h"
#include "jvfs.h"
#include "jprocfs.h"

#define STATUS_HEADER       "HTTP/1.1 200 OK\r\n" \
                            "Server: jstatus\r\n" \
                            "Content-Length: %d\r\n" \
                            "Content-Type: %s\r\n" \
                            "\r\n"

typedef struct jstatus_context {
    jsocket sock;

    uint8_t buf[STATUS_BUF_SIZE];
    int size;
}jstatus_context;

static jstatus_context *jstatus = NULL;

static int jstatus_read(char *buf, int *size)
{
    if ((buf==NULL) || (size==NULL))
        return ERROR_FAIL;

    jstring req = jstring_pick(buf, "GET ", " HTTP/1.1");
    if (!jstring_compare(req, "/"))
    {
        jstring_free(req);
        req = jstring_copy("/status.html");
    }

    jbool ico = jfalse;
    if (!jstring_compare(req, "/favicon.ico"))
        ico = jtrue;

    jstring file = jstring_link(STATUS_PREFIX, tostring(req));

    jhandle f = jvfs_open(tostring(file), jfalse);
    if (f == NULL)
    {
        jstring_free(file);
        file = jstring_link("proc://", tostring(req));
        f = jvfs_open(tostring(file), jfalse);
        if (f == NULL)
        {
            jstring_free(file);
            jstring_free(req);
            return ERROR_FAIL;
        }
    }

    jstring_free(file);
    jstring_free(req);

    uint8_t *body = (uint8_t *)jmalloc(STATUS_BUF_SIZE);
    if (body == NULL)
    {
        jvfs_close(f);
        return ERROR_FAIL;
    }
    memset(body, 0, STATUS_BUF_SIZE);

    int filesize = jvfs_read(f, body, STATUS_BUF_SIZE-512);

    *size = 0;
    memset(buf, 0, STATUS_BUF_SIZE);
    *size += jsnprintf(buf, STATUS_BUF_SIZE, STATUS_HEADER, filesize, ico?"image/x-icon":"text/html; charset=gb2312");

    memcpy(buf+(*size), body, filesize);
    *size += filesize;

    jfree(body);

    jvfs_close(f);

    return SUCCESS;
}

static jevent_ret jstatus_read_handle(int fd, jevent_type type, void *arg)
{
    if ((fd<0) || (type!=EVT_TYPE_READ) || (arg==NULL))
        return EVT_RET_REMOVE;

    jstatus_context *ctx = (jstatus_context *)arg;

    memset(ctx->buf, 0, STATUS_BUF_SIZE);

    if ((ctx->size=tcp_read(fd, ctx->buf, STATUS_BUF_SIZE)) <= 0)
        return EVT_RET_REMOVE;

    if (jstatus_read((char *)(ctx->buf), &(ctx->size)) == SUCCESS)
        tcp_write(fd, ctx->buf, ctx->size);

    return EVT_RET_REMOVE;
}

static jevent_ret jstatus_accept_handle(int fd, jevent_type type, void *arg)
{
    if ((fd<0) || (type!=EVT_TYPE_READ) || (arg==NULL))
        return EVT_RET_REMOVE;

    jsocket sock = tcp_accept(fd);
    if (sock < 0)
        return EVT_RET_REMOVE;

    jevent_add_event(g_event, sock, EVT_TYPE_READ,
        jstatus_read_handle, jstatus, jevent_general_clean, NULL);

    return EVT_RET_SUCCESS;
}

static int jprocfs_file_status_html(const char *file, char *buf, int size)
{
    if ((buf==NULL) || (size<=0))
        return 0;

    extern unsigned char g_status_html[];
    extern int g_status_html_size;

    int rsize = jmin(size, g_status_html_size);
    memcpy(buf, g_status_html, rsize);

    return rsize;
}

static int jprocfs_file_favicon_ico(const char *file, char *buf, int size)
{
    if ((buf==NULL) || (size<=0))
        return 0;

    extern unsigned char g_favicon_ico[];
    extern int g_favicon_ico_size;

    int rsize = jmin(size, g_favicon_ico_size);
    memcpy(buf, g_favicon_ico, rsize);

    return rsize;
}

int jstatus_setup(void)
{
    jprocfs_add_file("status.html", jprocfs_file_status_html);
    jprocfs_add_file("favicon.ico", jprocfs_file_favicon_ico);

    int port = STATUS_PORT;

    jstatus = (jstatus_context *)jmalloc(sizeof(jstatus_context));
    if (jstatus == NULL)
        return ERROR_FAIL;
    memset(jstatus, 0, sizeof(jstatus_context));

    jstatus->sock = tcp_listen(port);
    if (jstatus->sock < 0)
    {
        jerr("[jstatus] listen fail\n");
        jfree(jstatus);
        jstatus = NULL;
        return ERROR_FAIL;
    }

    jevent_add_event(g_event, jstatus->sock, EVT_TYPE_READ,
        jstatus_accept_handle, jstatus, NULL, NULL);

    return SUCCESS;
}

void jstatus_shutdown(void)
{
    if (jstatus)
    {
        jevent_del_event(g_event, jstatus->sock);
        jsocket_close(jstatus->sock);
        jfree(jstatus);
        jstatus = NULL;
    }
}


