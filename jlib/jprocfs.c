/*
 * jprocfs.c
 *
 * @chuanjiong
 */

#include "jprocfs.h"
#include "jvfsfs.h"
#include "jdict.h"

static jhandle jprocfs_dict = NULL;

typedef struct jprocfs_context {
    jstring file;
    jdict_value value;
}jprocfs_context;

static jhandle jprocfs_open(const char *url, jvfs_rw rw)
{
    if ((url==NULL) || (rw==VFS_WRITE))
        return NULL;

    jprocfs_context *ctx = (jprocfs_context *)jmalloc(sizeof(jprocfs_context));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(jprocfs_context));

    ctx->file = jstring_file_name(url);

    //reg:0x80000000
    jstring file = jstring_cut(tostring(ctx->file), ":");
    if (stringsize(file) > 0)
    {
        ctx->value = jdict_get_value(jprocfs_dict, tostring(file));
        jstring_free(file);
    }
    else
    {
        ctx->value = jdict_get_value(jprocfs_dict, tostring(ctx->file));
    }

    if (ctx->value.type != DICT_VALUE_TYPE_UINT)
    {
        jstring_free(ctx->file);
        jfree(ctx);
        return NULL;
    }

    return ctx;
}

static int jprocfs_seek(jhandle h, int64_t pos, int base)
{
    return ERROR_FAIL;
}

static int64_t jprocfs_tell(jhandle h)
{
    return -1;
}

static int64_t jprocfs_read(jhandle h, uint8_t *buf, int64_t size)
{
    if ((h==NULL) || (buf==NULL) || (size<=0))
        return 0;

    jprocfs_context *ctx = (jprocfs_context *)h;

    int (*f)(const char *file, char *buf, int size) = (int (*)(const char *file, char *buf, int size))(ctx->value.v);

    return f(tostring(ctx->file), (char *)buf, size);
}

static int64_t jprocfs_write(jhandle h, uint8_t *buf, int64_t size)
{
    return 0;
}

static void jprocfs_close(jhandle h)
{
    if (h == NULL)
        return;

    jprocfs_context *ctx = (jprocfs_context *)h;

    jstring_free(ctx->file);

    jfree(ctx);
}

const jvfsfs jprocfs =
{
    jprocfs_open,
    jprocfs_seek,
    jprocfs_tell,
    jprocfs_read,
    jprocfs_write,
    jprocfs_close
};

static int jprocfs_file_procfs(const char *file, char *buf, int size)
{
    if ((buf==NULL) || (size<=0))
        return 0;

    int total = 0;

    total += jsnprintf(buf+total, size-total, "<h2> list all files in procfs </h2>");

    int i;
    jstring key;
    for (i=0; i<jdict_get_capacity(jprocfs_dict); i++)
    {
        if (jdict_get_value_by_index(jprocfs_dict, i, &key, NULL) == SUCCESS)
        {
            if (jstring_compare(key, "exit") && jstring_compare(key, "procfs")
                && jstring_compare(key, "status.html") && jstring_compare(key, "favicon.ico"))
            {
                total += jsnprintf(buf+total, size-total, "<hr/>");
                total += jsnprintf(buf+total, size-total,
                    "<button onclick=\"location.href='/%s'\"> %s </button>", tostring(key), tostring(key));
            }
        }
    }

    return total;
}

int jprocfs_setup(void)
{
    jprocfs_dict = jdict_alloc();

    jvfsfs_register_fs("proc://", &jprocfs);

    jprocfs_add_file("procfs", jprocfs_file_procfs);

    return SUCCESS;
}

void jprocfs_shutdown(void)
{
    jdict_free(jprocfs_dict);
}

int jprocfs_add_file(const char *name, int (*rd)(const char *file, char *buf, int size))
{
    if ((name==NULL) || (rd==NULL))
        return ERROR_FAIL;

    jdict_value value = {0};
    value.type = DICT_VALUE_TYPE_UINT;
    value.v = (unsigned int)(rd);

    return jdict_set_value(jprocfs_dict, name, value);
}


