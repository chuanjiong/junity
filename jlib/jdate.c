/*
 * jdate.c
 *
 * @chuanjiong
 */

#include "jdate.h"

static pthread_mutex_t date_mutex = PTHREAD_MUTEX_INITIALIZER;

static const char *week[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char *month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

jdate jdate_get_UTC(void)
{
    jdate date;

    pthread_mutex_lock(&date_mutex);

    time_t t = time(NULL);
    struct tm *g = gmtime(&t);

    date = *g;

    pthread_mutex_unlock(&date_mutex);

    return date;
}

jdate jdate_get_local(void)
{
    jdate date;

    pthread_mutex_lock(&date_mutex);

    time_t t = time(NULL);
    struct tm *g = localtime(&t);

    date = *g;

    pthread_mutex_unlock(&date_mutex);

    return date;
}

int jdate_format_date(jdate date, char *buf, int size, jdate_format format)
{
    if ((buf==NULL) || (size<=0))
        return ERROR_FAIL;

    memset(buf, 0, size);

    switch (format)
    {
        // week, day month year hour:min:sec GMT
        case DATEFORMAT_W_DMY_HMS_GMT:
            jsnprintf(buf, size, "%s, %02d %s %4d %02d:%02d:%02d GMT",
                week[date.tm_wday], date.tm_mday, month[date.tm_mon], date.tm_year+1900,
                date.tm_hour, date.tm_min, date.tm_sec);
            break;

        default:
            break;
    }

    return SUCCESS;
}


