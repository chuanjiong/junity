/*
 * jconfig.h
 *
 * @chuanjiong
 */

#ifndef _J_CONFIG_H_
#define _J_CONFIG_H_

//
// general configuration
//

//jmalloc
#define MEM_DEBUG                   (1)

#define MEM_DEBUG_TRACE             (1)

#define MEM_DEBUG_TRACE_SIZE        (1024)

//jbufreader jbufwriter
#define BUF_RW_BUF_SIZE             (4096)

//jlog
#define LOG_SWITCH                  (1)

#define LOG_COLOR                   (1)

//jsocket
#define MAX_LISTEN_Q                (128)

#define IP_STRING_SIZE              (16)

//jstatus
#define STATUS_PORT                 (12358)

#define STATUS_BUF_SIZE             (32*1024)

#define STATUS_PREFIX               "./status"

//thread sleep time
#define THREAD_SLEEP_TIME           (100)

#endif //_J_CONFIG_H_


