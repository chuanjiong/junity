/*
 * jvfs.h
 *
 * @chuanjiong
 */

#ifndef _J_VFS_H_
#define _J_VFS_H_

#include "jcommon.h"

#ifdef __cplusplus
extern "C"
{
#endif

int jvfs_setup(void);

void jvfs_shutdown(void);

jhandle jvfs_open(const char *url, jbool write);

int jvfs_seek(jhandle h, int64_t pos, int base);

int64_t jvfs_tell(jhandle h);

int64_t jvfs_read(jhandle h, uint8_t *buf, int64_t size);

int64_t jvfs_write(jhandle h, uint8_t *buf, int64_t size);

void jvfs_close(jhandle h);

#ifdef __cplusplus
}
#endif

#endif //_J_VFS_H_


