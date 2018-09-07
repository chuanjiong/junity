/*
 * rtsp_response.h
 *
 * @chuanjiong
 */

#ifndef _RTSP_RESPONSE_H_
#define _RTSP_RESPONSE_H_

#define RESPONSE_OPTIONS    "RTSP/1.0 200 OK\r\n" \
                            "Server: rtsp server by chuanjiong\r\n" \
                            "Content-Length: 0\r\n" \
                            "CSeq: %d\r\n" \
                            "Public: DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE\r\n" \
                            "\r\n"

#define RESPONSE_DESCRIBE   "RTSP/1.0 200 OK\r\n" \
                            "Server: rtsp server by chuanjiong\r\n" \
                            "Date: %s\r\n" \
                            "Content-Type: application/sdp\r\n" \
                            "Content-Base: %s\r\n" \
                            "Content-Length: %d\r\n" \
                            "Cache-Control: no-cache\r\n" \
                            "CSeq: %d\r\n" \
                            "\r\n" \
                            "%s"

#define RESPONSE_SETUP      "RTSP/1.0 200 OK\r\n" \
                            "Server: rtsp server by chuanjiong\r\n" \
                            "Date: %s\r\n" \
                            "Transport: %s\r\n" \
                            "Session: %s;timeout=60\r\n" \
                            "Content-Length: 0\r\n" \
                            "Cache-Control: no-cache\r\n" \
                            "CSeq: %d\r\n" \
                            "\r\n"

#define RESPONSE_PLAY       "RTSP/1.0 200 OK\r\n" \
                            "Server: rtsp server by chuanjiong\r\n" \
                            "Date: %s\r\n" \
                            "RTP-Info: url=%s;seq=0;rtptime=%lld, url=%s;seq=0;rtptime=%lld\r\n" \
                            "Range: npt=0.000-\r\n" \
                            "Session: %s;timeout=60\r\n" \
                            "Content-Length: 0\r\n" \
                            "Cache-Control: no-cache\r\n" \
                            "CSeq: %d\r\n" \
                            "\r\n"

#define RESPONSE_PAUSE      "RTSP/1.0 200 OK\r\n" \
                            "Server: rtsp server by chuanjiong\r\n" \
                            "Date: %s\r\n" \
                            "CSeq: %d\r\n" \
                            "\r\n"

#define RESPONSE_TEARDOWN   "RTSP/1.0 200 OK\r\n" \
                            "Server: rtsp server by chuanjiong\r\n" \
                            "CSeq: %d\r\n" \
                            "\r\n"

#define RESPONSE_ERROR      "RTSP/1.0 400 Bad Request\r\n" \
                            "Server: rtsp server by chuanjiong\r\n" \
                            "CSeq: %d\r\n" \
                            "Connection: Close\r\n" \
                            "\r\n"

#endif //_RTSP_RESPONSE_H_


