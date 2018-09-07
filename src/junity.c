/*
 * junity.c
 *
 * @chuanjiong
 */

#include "jmm.h"
#include "jpic.h"
#include "jpcm.h"

int main(int argc, char **argv)
{
    jlib_setup();

#if 0
    jhandle re = jresample_open(2, 44100, 8000);

    jwav_format fmt;
    jhandle rd = jwav_reader_open("syst.wav", &fmt);

    jwav_format out;
    out.samplerate = 8000;
    out.channels = 2;
    out.bpp = 16;
    jhandle wt = jwav_writer_open("o.wav", &out);

    uint8_t buf[1024], dst[1024];

    while (1)
    {
        int size = jwav_reader_read(rd, buf, 1024);
        if (size <= 0)
            break;

        int rr = jresample_resample(re, buf, size/4, dst, 256);
        if (rr <= 0)
            break;

        jwav_writer_write(wt, dst, rr*4);
    }

    jwav_writer_close(wt);
    jwav_reader_close(rd);

    jresample_close(re);
#endif

#if 0
    jwav_format fmt;
    jhandle i = jwav_reader_open("syst.wav", &fmt);
    jhandle o = jwav_writer_open("o.wav", &fmt);

    jhandle e = jopus_encoder_open(48000, fmt.channels, 64000);
    jhandle d = jopus_decoder_open(48000, fmt.channels, 16);

    uint8_t buf[2*2*960]; //20ms
    int size;
    do {
        size = jwav_reader_read(i, buf, 2*2*960);

        jopus_frame *ef = jopus_encoder_enc(e, buf, size/4);
        if (ef == NULL)
            break;

        jopus_frame *df = jopus_decoder_dec(d, ef->data, ef->size);
        if (df == NULL)
            break;

        jlog("%d -> %d\n", ef->size, df->size);

        if (df->size > 0)
            jwav_writer_write(o, df->data, df->size);

        jopus_frame_free(ef);
        jopus_frame_free(df);
    }while (size > 0);

    jopus_encoder_close(e);
    jopus_decoder_close(d);

    jwav_reader_close(i);
    jwav_writer_close(o);
#endif

#if 1
    jpic_frame *frame = NULL;
    jpic_frame *out = NULL;

    //read
    jpic_read("p.png", &frame);

    //write
    jpic_resize(frame, &out, 800, 600);
    jpic_write("o.png", out);
    jpic_frame_free(out);

    jpic_resize(frame, &out, 800, 600);
    jpic_write("o.bmp", out);
    jpic_frame_free(out);

    jpic_resize(frame, &out, 1024, 768);
    jpic_write("o.jpg", out);
    jpic_frame_free(out);

    jpic_frame_free(frame);
#endif

#if 0
    jhandle rtsp = rtsp_server_setup(RTSP_SERVER_DEFAULT_PORT);

    media_source_config src_cfg;

    src_cfg.type = MEDIA_SOURCE_TYPE_URL;
    src_cfg.url = "a.mp4";
    rtsp_server_add_media(rtsp, "/aaa", &src_cfg);

    //src_cfg.type = MEDIA_SOURCE_TYPE_URL;
    //src_cfg.url = "b.mp4";
    //rtsp_server_add_media(rtsp, "/bbb", &src_cfg);

    while (!jlib_is_exit())
        jsleep(1000);

    rtsp_server_shutdown(rtsp);
#endif

#if 0
    jhandle demux = jmm_demuxer_open("rtsp://admin:12345@192.168.1.125/ch1/stream");
    jhandle mux = jmm_muxer_open("rtmp://192.168.1.150:1935/live/abc");
    jhandle mux1 = jmm_muxer_open("o.flv");

    while (!jlib_is_exit())
    {
        jmm_packet *pkt = jmm_demuxer_read(demux);
        if (pkt == NULL)
            break;
        jmm_muxer_write(mux, pkt);
        jmm_muxer_write(mux1, pkt);
        jmm_packet_free(pkt);
    }

    jmm_demuxer_close(demux);
    jmm_muxer_close(mux);
    jmm_muxer_close(mux1);
#endif

#if 0
    //"rtsp://admin:12345@192.168.1.125/ch1/stream"
    //"rtsp://192.168.1.152:8554/aaa"
    jhandle rtsp = jmm_demuxer_open("rtsp://192.168.1.152:8554/aaa");

    while (!jlib_is_exit())
        jsleep(1000);
#endif

    jlib_shutdown();
    return 0;
}


