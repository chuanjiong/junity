/*
 * jvfsfs.h
 *
 * @chuanjiong
 */

#ifndef _J_VFSFS_H_
#define _J_VFSFS_H_

#include "jcommon.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum jvfs_rw {
    VFS_READ,
    VFS_WRITE,
}jvfs_rw;

typedef struct jvfsfs {
    jhandle (*vfs_open)(const char *url, jvfs_rw rw);
    int (*vfs_seek)(jhandle h, int64_t pos, int base);
    int64_t (*vfs_tell)(jhandle h);
    int64_t (*vfs_read)(jhandle h, uint8_t *buf, int64_t size);
    int64_t (*vfs_write)(jhandle h, uint8_t *buf, int64_t size);
    void (*vfs_close)(jhandle h);
}jvfsfs;

int jvfsfs_setup(void);

void jvfsfs_shutdown(void);

int jvfsfs_register_fs(const char *protocol, const jvfsfs *fs);

const jvfsfs *jvfsfs_find(const char *protocol);

#ifdef __cplusplus
}
#endif

#endif //_J_VFSFS_H_


