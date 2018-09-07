/*
 * jprocfs.h
 *
 * @chuanjiong
 */

#ifndef _J_PROCFS_H_
#define _J_PROCFS_H_

#include "jcommon.h"

#ifdef __cplusplus
extern "C"
{
#endif

int jprocfs_setup(void);

void jprocfs_shutdown(void);

int jprocfs_add_file(const char *name, int (*rd)(const char *file, char *buf, int size));

#ifdef __cplusplus
}
#endif

#endif //_J_PROCFS_H_


