/*
 * jdate.h
 *
 * @chuanjiong
 */

#ifndef _J_DATE_H_
#define _J_DATE_H_

#include "jcommon.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct tm jdate;

typedef enum jdate_format {
    DATEFORMAT_W_DMY_HMS_GMT,
}jdate_format;

jdate jdate_get_UTC(void);

jdate jdate_get_local(void);

int jdate_format_date(jdate date, char *buf, int size, jdate_format format);

#ifdef __cplusplus
}
#endif

#endif //_J_DATE_H_


