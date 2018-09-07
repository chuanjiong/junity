/*
 * jlog.h
 *
 * @chuanjiong
 */

#ifndef _J_LOG_H_
#define _J_LOG_H_

#if (LOG_COLOR == 1)
#define LCOLOR_NONE         "\033[0m"
#define LCOLOR_BLACK        "\033[1;30m"
#define LCOLOR_RED          "\033[1;31m"
#define LCOLOR_GREEN        "\033[1;32m"
#define LCOLOR_YELLOW       "\033[1;33m"
#define LCOLOR_BLUE         "\033[1;34m"
#define LCOLOR_MAGENTA      "\033[1;35m"
#define LCOLOR_CYAN         "\033[1;36m"
#define LCOLOR_WHITE        "\033[1;37m"
#else
#define LCOLOR_NONE
#define LCOLOR_BLACK
#define LCOLOR_RED
#define LCOLOR_GREEN
#define LCOLOR_YELLOW
#define LCOLOR_BLUE
#define LCOLOR_MAGENTA
#define LCOLOR_CYAN
#define LCOLOR_WHITE
#endif

#if (LOG_SWITCH == 1)

#define jlog(fmt,arg...)    { \
                                printf(LCOLOR_GREEN fmt LCOLOR_NONE, ##arg); \
                            }

#define jwarn(fmt,arg...)   { \
                                printf(LCOLOR_YELLOW fmt LCOLOR_NONE, ##arg); \
                            }

#define jerr(fmt,arg...)    { \
                                printf(LCOLOR_RED fmt LCOLOR_NONE, ##arg); \
                            }

#else

#define jlog(fmt,arg...)    { \
                            }

#define jwarn(fmt,arg...)   { \
                            }

#define jerr(fmt,arg...)    { \
                            }

#endif

#endif //_J_LOG_H_


