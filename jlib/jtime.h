/*
 * jtime.h
 *
 * @chuanjiong
 */

#ifndef _J_TIME_H_
#define _J_TIME_H_

#include "jcommon.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct timeval jtime;

jtime jtime_set_anchor(void);

jtime jtime_get_period(jtime anchor);

int64_t jtime_to_s(jtime t);

int64_t jtime_to_ms(jtime t);

int64_t jtime_to_us(jtime t);

#ifdef __cplusplus
}
#endif

#endif //_J_TIME_H_


