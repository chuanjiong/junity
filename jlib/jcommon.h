/*
 * jcommon.h
 *
 * @chuanjiong
 */

#ifndef _J_COMMON_H_
#define _J_COMMON_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/stat.h>
#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>
#include <errno.h>

#include "jconfig.h"
#include "jplatform.h"
#include "jlog.h"

#define jmin(x,y)           ((x)<(y)?(x):(y))
#define jmax(x,y)           ((x)>(y)?(x):(y))

#define jalign(s,a)         (((s)+(a)-1)&(~((a)-1)))

#define jarray_size(a)      ((sizeof(a))/(sizeof(a[0])))

typedef enum jbool {
    jfalse = 0,
    jtrue = 1,
}jbool;

#define SUCCESS             (0)
#define ERROR_FAIL          (-1)

#if (MEM_DEBUG == 0)
    #define jmalloc         malloc
    #define jfree           free
#else
    #include "jmalloc.h"
    #define jmalloc(s)      jmalloc_fence((s),__FILE__,__LINE__)
    #define jfree(p)        jfree_fence((p),__FILE__,__LINE__)
#endif

#define jsnprintf(buf,size,fmt,arg...) \
                            (size<=0?0:snprintf(buf,size,fmt,##arg))

typedef void *jhandle;

#endif //_J_COMMON_H_


