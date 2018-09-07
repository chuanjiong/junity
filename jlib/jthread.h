/*
 * jthread.h
 *
 * @chuanjiong
 */

#ifndef _J_THREAD_H_
#define _J_THREAD_H_

#include "jcommon.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef int (*jthread_routine)(jhandle t, void *arg);

jhandle jthread_setup(jthread_routine r, void *arg);

int jthread_is_running(jhandle h);

void jthread_shutdown(jhandle h);

#ifdef __cplusplus
}
#endif

#endif //_J_THREAD_H_


