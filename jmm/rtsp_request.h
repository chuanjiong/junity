/*
 * rtsp_request.h
 *
 * @chuanjiong
 */

#ifndef _RTSP_REQUEST_H_
#define _RTSP_REQUEST_H_

#define REQUEST_OPTIONS                 "OPTIONS %s RTSP/1.0\r\n" \
                                        "CSeq: %d\r\n" \
                                        "User-Agent: jmm_rtsp_demuxer\r\n" \
                                        "\r\n"

#define REQUEST_OPTIONS_AUTH            "OPTIONS %s RTSP/1.0\r\n" \
                                        "CSeq: %d\r\n" \
                                        "Authorization: %s\r\n" \
                                        "User-Agent: jmm_rtsp_demuxer\r\n" \
                                        "\r\n"

#define REQUEST_DESCRIBE                "DESCRIBE %s RTSP/1.0\r\n" \
                                        "CSeq: %d\r\n" \
                                        "User-Agent: jmm_rtsp_demuxer\r\n" \
                                        "Accept: application/sdp\r\n" \
                                        "\r\n"

#define REQUEST_DESCRIBE_AUTH           "DESCRIBE %s RTSP/1.0\r\n" \
                                        "CSeq: %d\r\n" \
                                        "Authorization: %s\r\n" \
                                        "User-Agent: jmm_rtsp_demuxer\r\n" \
                                        "Accept: application/sdp\r\n" \
                                        "\r\n"

#define REQUEST_SETUP_VIDEO_TCP         "SETUP %s RTSP/1.0\r\n" \
                                        "CSeq: %d\r\n" \
                                        "User-Agent: jmm_rtsp_demuxer\r\n" \
                                        "Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n" \
                                        "\r\n"

#define REQUEST_SETUP_VIDEO_AUTH_TCP    "SETUP %s RTSP/1.0\r\n" \
                                        "CSeq: %d\r\n" \
                                        "Authorization: %s\r\n" \
                                        "User-Agent: jmm_rtsp_demuxer\r\n" \
                                        "Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n" \
                                        "\r\n"

#define REQUEST_SETUP_AUDIO_TCP         "SETUP %s RTSP/1.0\r\n" \
                                        "CSeq: %d\r\n" \
                                        "User-Agent: jmm_rtsp_demuxer\r\n" \
                                        "Transport: RTP/AVP/TCP;unicast;interleaved=2-3\r\n" \
                                        "Session: %s\r\n" \
                                        "\r\n"

#define REQUEST_SETUP_AUDIO_AUTH_TCP    "SETUP %s RTSP/1.0\r\n" \
                                        "CSeq: %d\r\n" \
                                        "Authorization: %s\r\n" \
                                        "User-Agent: jmm_rtsp_demuxer\r\n" \
                                        "Transport: RTP/AVP/TCP;unicast;interleaved=2-3\r\n" \
                                        "Session: %s\r\n" \
                                        "\r\n"

#define REQUEST_SETUP_VIDEO_UDP         "SETUP %s RTSP/1.0\r\n" \
                                        "CSeq: %d\r\n" \
                                        "User-Agent: jmm_rtsp_demuxer\r\n" \
                                        "Transport: RTP/AVP/UDP;unicast;client_port=%d-%d\r\n" \
                                        "\r\n"

#define REQUEST_SETUP_VIDEO_AUTH_UDP    "SETUP %s RTSP/1.0\r\n" \
                                        "CSeq: %d\r\n" \
                                        "Authorization: %s\r\n" \
                                        "User-Agent: jmm_rtsp_demuxer\r\n" \
                                        "Transport: RTP/AVP/UDP;unicast;client_port=%d-%d\r\n" \
                                        "\r\n"

#define REQUEST_SETUP_AUDIO_UDP         "SETUP %s RTSP/1.0\r\n" \
                                        "CSeq: %d\r\n" \
                                        "User-Agent: jmm_rtsp_demuxer\r\n" \
                                        "Transport: RTP/AVP/UDP;unicast;client_port=%d-%d\r\n" \
                                        "Session: %s\r\n" \
                                        "\r\n"

#define REQUEST_SETUP_AUDIO_AUTH_UDP    "SETUP %s RTSP/1.0\r\n" \
                                        "CSeq: %d\r\n" \
                                        "Authorization: %s\r\n" \
                                        "User-Agent: jmm_rtsp_demuxer\r\n" \
                                        "Transport: RTP/AVP/UDP;unicast;client_port=%d-%d\r\n" \
                                        "Session: %s\r\n" \
                                        "\r\n"

#define REQUEST_PLAY                    "PLAY %s RTSP/1.0\r\n" \
                                        "CSeq: %d\r\n" \
                                        "User-Agent: jmm_rtsp_demuxer\r\n" \
                                        "Session: %s\r\n" \
                                        "Range: npt=0.000-\r\n" \
                                        "\r\n"

#define REQUEST_PLAY_AUTH               "PLAY %s RTSP/1.0\r\n" \
                                        "CSeq: %d\r\n" \
                                        "Authorization: %s\r\n" \
                                        "User-Agent: jmm_rtsp_demuxer\r\n" \
                                        "Session: %s\r\n" \
                                        "Range: npt=0.000-\r\n" \
                                        "\r\n"

#define REQUEST_TEARDOWN                "TEARDOWN %s RTSP/1.0\r\n" \
                                        "CSeq: %d\r\n" \
                                        "User-Agent: jmm_rtsp_demuxer\r\n" \
                                        "Session: %s\r\n" \
                                        "\r\n"

#endif //_RTSP_REQUEST_H_


