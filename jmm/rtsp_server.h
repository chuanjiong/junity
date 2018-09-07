/*
 * rtsp_server.h
 *
 * @chuanjiong
 */

#ifndef _RTSP_SERVER_H_
#define _RTSP_SERVER_H_

#include "rtsp_media_source.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define RTSP_SERVER_DEFAULT_PORT            (8554)
#define RTSP_SERVER_REQ_RES_BUF_SIZE        (4096)
#define RTSP_SERVER_MAX_MEDIA_SOURCE        (16)
#define RTSP_SERVER_MAX_SESSION             (RTSP_SERVER_MAX_MEDIA_SOURCE*MEDIA_SOURCE_MAX_SINK)

jhandle rtsp_server_setup(int port);

int rtsp_server_add_media(jhandle h, const char *url, media_source_config *cfg);

int rtsp_server_del_media(jhandle h, const char *url);

int rtsp_server_get_callback(jhandle h, const char *url, media_source_callback *cb, jhandle *cb_arg);

void rtsp_server_shutdown(jhandle h);

#ifdef __cplusplus
}
#endif

#endif //_RTSP_SERVER_H_


