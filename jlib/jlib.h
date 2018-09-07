/*
 * jlib.h
 *
 * @chuanjiong
 */

#ifndef _J_LIB_H_
#define _J_LIB_H_

#include "jcommon.h"

#include "jatomic.h"
#include "jbitreader.h"
#include "jbufreader.h"
#include "jbufwriter.h"
#include "jdate.h"
#include "jdict.h"
#include "jdynarray.h"
#include "jevent.h"
#include "jprocfs.h"
#include "jqueue.h"
#include "jsocket.h"
#include "jstring.h"
#include "jthread.h"
#include "jtime.h"
#include "jvfs.h"

#ifdef __cplusplus
extern "C"
{
#endif

void jlib_setup(void);

jbool jlib_is_exit(void);

void jlib_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif //_J_LIB_H_


