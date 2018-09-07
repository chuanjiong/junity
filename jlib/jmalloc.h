/*
 * jmalloc.h
 *
 * @chuanjiong
 */

#ifndef _J_MALLOC_H_
#define _J_MALLOC_H_

#if (MEM_DEBUG == 1)

#ifdef __cplusplus
extern "C"
{
#endif

void jmalloc_setup(void);

void jmalloc_shutdown(void);

void *jmalloc_fence(int size, const char *file, int line);

void jfree_fence(void *p, const char *file, int line);

int jmalloc_trace(char *buf, int size);

#ifdef __cplusplus
}
#endif

#endif

#endif //_J_MALLOC_H_


