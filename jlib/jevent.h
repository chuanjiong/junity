/*
 * jevent.h
 *
 * @chuanjiong
 */

#ifndef _J_EVENT_H_
#define _J_EVENT_H_

#include "jcommon.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum jevent_type {
    EVT_TYPE_READ   = 0x1,
    EVT_TYPE_WRITE  = 0x2,
    EVT_TYPE_RW     = EVT_TYPE_READ | EVT_TYPE_WRITE,
}jevent_type;

typedef enum jevent_ret {
    EVT_RET_REMOVE  = -1,
    EVT_RET_SUCCESS = 0,
}jevent_ret;

typedef jevent_ret (*jevent_handle)(int fd, jevent_type type, void *arg);

typedef void (*jevent_clean)(int fd, void *arg);

extern jhandle g_event;

int jevent_setup(void);

void jevent_shutdown(void);

jhandle jevent_alloc(void);

void jevent_free(jhandle h);

int jevent_add_event(jhandle h, int fd, jevent_type type,
                     jevent_handle hdl, void *arg,
                     jevent_clean clean, void *carg);

int jevent_del_event(jhandle h, int fd);

void jevent_general_clean(int fd, void *arg);

#ifdef __cplusplus
}
#endif

#endif //_J_EVENT_H_


