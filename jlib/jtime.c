/*
 * jtime.c
 *
 * @chuanjiong
 */

#include "jtime.h"

#define jtime_sub(a,b,r)    { \
                                (r)->tv_sec = (a)->tv_sec - (b)->tv_sec; \
                                (r)->tv_usec = (a)->tv_usec - (b)->tv_usec; \
                                if ((r)->tv_usec < 0) \
                                { \
                                    (r)->tv_sec -= 1; \
                                    (r)->tv_usec += 1000000; \
                                } \
                            }

jtime jtime_set_anchor(void)
{
    jtime anchor;

    gettimeofday(&anchor, NULL);

    return anchor;
}

jtime jtime_get_period(jtime anchor)
{
    jtime cur;

    gettimeofday(&cur, NULL);

    jtime_sub(&cur, &anchor, &cur);

    return cur;
}

int64_t jtime_to_s(jtime t)
{
    return t.tv_sec;
}

int64_t jtime_to_ms(jtime t)
{
    return (((int64_t)(t.tv_sec))*1000 + t.tv_usec/1000);
}

int64_t jtime_to_us(jtime t)
{
    return (((int64_t)(t.tv_sec))*1000000 + t.tv_usec);
}


