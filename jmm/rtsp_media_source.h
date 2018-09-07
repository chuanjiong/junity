/*
 * rtsp_media_source.h
 *
 * @chuanjiong
 */

#ifndef _RTSP_MEDIA_SOURCE_H_
#define _RTSP_MEDIA_SOURCE_H_

#include "jmm_util.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MEDIA_SOURCE_MAX_SINK       (16)

#define SDP_FORMAT                  "v=0\r\n" \
                                    "o=- 0 0 IN IP4 RTSP_SERVER_BY_CHUANJIONG\r\n" \
                                    "s=rtsp server by chuanjiong\r\n" \
                                    "i=N/A\r\n" \
                                    "c=IN IP4 0.0.0.0\r\n" \
                                    "t=0 0\r\n" \
                                    "a=tool:rtsp server by chuanjiong\r\n" \
                                    "a=recvonly\r\n" \
                                    "a=type:broadcast\r\n" \
                                    "a=charset:UTF-8\r\n" \
                                    "m=audio 0 RTP/AVP 97\r\n" \
                                    "b=RR:0\r\n" \
                                    "a=rtpmap:97 mpeg4-generic/%d/%d\r\n" \
                                    "a=fmtp:97 streamtype=5;profile-level-id=15;mode=AAC-hbr;config=%02x%02x;sizeLength=13;indexLength=3;indexDeltaLength=3;constantDuration=1024\r\n" \
                                    "a=control:audio\r\n" \
                                    "m=video 0 RTP/AVP 96\r\n" \
                                    "b=RR:0\r\n" \
                                    "a=rtpmap:96 H264/90000\r\n" \
                                    "a=fmtp:96 packetization-mode=1;profile-level-id=00002a;sprop-parameter-sets=%s,%s;\r\n" \
                                    "a=control:video\r\n"

#define SDP_FORMAT_ONLY_VIDEO       "v=0\r\n" \
                                    "o=- 0 0 IN IP4 RTSP_SERVER_BY_CHUANJIONG\r\n" \
                                    "s=rtsp server by chuanjiong\r\n" \
                                    "i=N/A\r\n" \
                                    "c=IN IP4 0.0.0.0\r\n" \
                                    "t=0 0\r\n" \
                                    "a=tool:rtsp server by chuanjiong\r\n" \
                                    "a=recvonly\r\n" \
                                    "a=type:broadcast\r\n" \
                                    "a=charset:UTF-8\r\n" \
                                    "m=video 0 RTP/AVP 96\r\n" \
                                    "b=RR:0\r\n" \
                                    "a=rtpmap:96 H264/90000\r\n" \
                                    "a=fmtp:96 packetization-mode=1;profile-level-id=00002a;sprop-parameter-sets=%s,%s;\r\n" \
                                    "a=control:video\r\n"

typedef int (*media_source_callback)(jhandle h, jmm_packet *packet);

typedef enum media_source_type {
    MEDIA_SOURCE_TYPE_URL,
    MEDIA_SOURCE_TYPE_CALLBACK,
}media_source_type;

typedef struct media_source_config {
    media_source_type type;
    const char *url;
}media_source_config;

jhandle media_source_setup(media_source_config *cfg);

char *media_source_get_sdp(jhandle h);

int64_t media_source_get_time(jhandle h, jstring url);

/*
    ip -> udp
    sock -> tcp
    return: sink index
*/
int media_source_add_sink(jhandle h, jstring url[2], const char *ip, jsocket sock, jstring transport[2]);

int media_source_del_sink(jhandle h, int index);

int media_source_send_packet(jhandle h, jmm_packet *packet);

void media_source_shutdown(jhandle h);

#ifdef __cplusplus
}
#endif

#endif //_RTSP_MEDIA_SOURCE_H_


