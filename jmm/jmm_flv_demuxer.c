/*
 * jmm_flv_demuxer.c
 *
 * @chuanjiong
 */

#include "jmm_module.h"

/*
  F L V 1 a&v start

  tag-0-size

  tag-1

  tag-1-size

  ...

  tag-n

  tag-n-size
*/

typedef struct jmm_flv_demuxer_ctx {
    jhandle rd;

    jmm_packet *asc;
    jmm_packet *avcc;

    jmm_asc_info ainfo;
    jmm_file_info finfo;

    jstring url;
    jhandle build;
    pthread_mutex_t mutex;

    int total_seek_entry;
    jdynarray(jmm_seek_entry, seek_table);
}jmm_flv_demuxer_ctx;

static int build_seek_table(jhandle t, void *arg)
{
    jmm_flv_demuxer_ctx *ctx = (jmm_flv_demuxer_ctx *)arg;

    jhandle rd = jbufreader_open(tostring(ctx->url));
    if (rd == NULL)
        return ERROR_FAIL;

    jbufreader_skip(rd, 5);
    uint32_t start = jbufreader_B32(rd);
    jbufreader_skip(rd, start-jbufreader_tell(rd));
    jbufreader_skip(rd, 4);

    pthread_mutex_lock(&(ctx->mutex));
    jdynarray_alloc(jmm_seek_entry, ctx->seek_table, 1024);
    pthread_mutex_unlock(&(ctx->mutex));

    uint32_t type, size, ts, ts_ext, fmt;
    while (jthread_is_running(t) && (!jbufreader_eof(rd)))
    {
        type = jbufreader_8(rd);
        size = jbufreader_B24(rd);
        ts = jbufreader_B24(rd);
        ts_ext = jbufreader_8(rd);
        jbufreader_B24(rd); //StreamID
        if (type != 0x9)
        {
            jbufreader_skip(rd, size);
        }
        else //video
        {
            fmt = jbufreader_8(rd);
            if ((fmt&0xf0) == 0x10) //key frame
            {
                pthread_mutex_lock(&(ctx->mutex));
                jdynarray_index(ctx->seek_table, ctx->total_seek_entry);
                ctx->seek_table[ctx->total_seek_entry].ts = (ts|(ts_ext<<24)) * 1000;
                ctx->seek_table[ctx->total_seek_entry].pos = jbufreader_tell(rd)-12;
                ctx->total_seek_entry++;
                pthread_mutex_unlock(&(ctx->mutex));
            }
            jbufreader_skip(rd, size-1);
        }
        //tag-n-size
        jbufreader_B32(rd);
        int64_t total_us = (ts|(ts_ext<<24)) * 1000;
        if (total_us > ctx->finfo.total_us)
            ctx->finfo.total_us = total_us;
    }

    jbufreader_close(rd);
    return SUCCESS;
}

static jhandle jmm_flv_demuxer_open(const char *url)
{
    if (url == NULL)
        return NULL;

    jmm_flv_demuxer_ctx *ctx = (jmm_flv_demuxer_ctx *)jmalloc(sizeof(jmm_flv_demuxer_ctx));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(jmm_flv_demuxer_ctx));

    ctx->rd = jbufreader_open(url);
    if (ctx->rd == NULL)
    {
        jfree(ctx);
        return NULL;
    }

    uint8_t buf[8] = {0};
    jbufreader_read(ctx->rd, buf, 5);

    if ((buf[0]!='F') || (buf[1]!='L') || (buf[2]!='V') || (buf[3]!=0x01))
    {
        jbufreader_close(ctx->rd);
        jfree(ctx);
        return NULL;
    }

    uint32_t start = RB32;
    jbufreader_skip(ctx->rd, start-jbufreader_tell(ctx->rd));

    //tag-0-size
    jbufreader_skip(ctx->rd, 4);

    //build seek table
    ctx->url = jstring_copy(url);
    pthread_mutex_init(&(ctx->mutex), NULL);
    ctx->build = jthread_setup(build_seek_table, ctx);

    return ctx;
}

static void jmm_flv_demuxer_close(jhandle h)
{
    if (h == NULL)
        return;

    jmm_flv_demuxer_ctx *ctx = (jmm_flv_demuxer_ctx *)h;

    jdynarray_free(ctx->seek_table);

    if (ctx->asc)
        jmm_packet_free(ctx->asc);

    if (ctx->avcc)
        jmm_packet_free(ctx->avcc);

    jthread_shutdown(ctx->build);

    pthread_mutex_destroy(&(ctx->mutex));

    jstring_free(ctx->url);

    jbufreader_close(ctx->rd);

    jfree(ctx);
}

static jmm_packet *jmm_flv_demuxer_extradata(jhandle h, jmm_codec_type type)
{
    if (h == NULL)
        return NULL;

    jmm_flv_demuxer_ctx *ctx = (jmm_flv_demuxer_ctx *)h;

    if (type == JMM_CODEC_TYPE_AAC)
        return jmm_packet_clone(ctx->asc);
    else
        return jmm_packet_clone(ctx->avcc);
}

static jmm_packet *jmm_flv_demuxer_read(jhandle h)
{
    if (h == NULL)
        return NULL;

    jmm_flv_demuxer_ctx *ctx = (jmm_flv_demuxer_ctx *)h;

    jmm_packet *pkt;
    uint32_t type, size, ts, ts_ext;

retry:
    if (jbufreader_eof(ctx->rd))
    {
        jlog("[jmm_flv_demuxer] eof\n");
        return NULL;
    }

    type = R8;
    size = RB24;
    ts = RB24;
    ts_ext = R8;
    RB24; //StreamID

    if (type == 0x8) //audio
    {
        uint32_t fmt = R8;
        if ((fmt&0xf0) != 0xa0)
        {
            jwarn("[jmm_flv_demuxer] audio is not aac format\n");
            return NULL;
        }

        uint32_t aac_pt = R8;
        if (aac_pt == 0) //asc
        {
            pkt = jmm_packet_alloc(size-2);
            if (pkt == NULL)
                return NULL;
            pkt->type = JMM_CODEC_TYPE_AAC;
            pkt->fmt = JMM_BS_FMT_AAC_ASC;
            pkt->key = jtrue;
            pkt->dts = (ts|(ts_ext<<24)) * 1000;
            pkt->pts = (ts|(ts_ext<<24)) * 1000;
            jbufreader_read(ctx->rd, pkt->data, pkt->size);
            jmm_aac_asc_parse(pkt, &(ctx->ainfo));
            if (ctx->asc)
                jmm_packet_free(ctx->asc);
            ctx->asc = jmm_packet_clone(pkt);
        }
        else
        {
            pkt = jmm_packet_alloc(size-2);
            if (pkt == NULL)
                return NULL;
            pkt->type = JMM_CODEC_TYPE_AAC;
            pkt->fmt = JMM_BS_FMT_AAC_ES;
            pkt->key = jtrue;
            pkt->dts = (ts|(ts_ext<<24)) * 1000;
            pkt->pts = (ts|(ts_ext<<24)) * 1000;
            jbufreader_read(ctx->rd, pkt->data, pkt->size);
            jmm_packet *tmp = pkt;
            pkt = jmm_aac_es2adts(pkt, &(ctx->ainfo), jfalse);
            if (pkt == NULL)
                jmm_packet_free(tmp);
        }

        //tag-n-size
        uint32_t check = RB32;
        if (check != (size+11))
        {
            jerr("[jmm_flv_demuxer] check size fail: %d, %d\n", check, size);
            jmm_packet_free(pkt);
            return NULL;
        }
        return pkt;
    }
    else if (type == 0x9) //video
    {
        uint32_t fmt = R8;
        if ((fmt&0x0f) != 0x07)
        {
            jwarn("[jmm_flv_demuxer] video is not avc format\n");
            return NULL;
        }

        uint32_t avc_pt = R8;
        uint32_t ct = RB24;
        if (avc_pt == 0) //avcc
        {
            pkt = jmm_packet_alloc(size-5);
            if (pkt == NULL)
                return NULL;
            pkt->type = JMM_CODEC_TYPE_AVC;
            pkt->fmt = JMM_BS_FMT_AVC_AVCC;
            pkt->key = jtrue;
            pkt->dts = (ts|(ts_ext<<24)) * 1000;
            pkt->pts = pkt->dts + ct*1000;
            jbufreader_read(ctx->rd, pkt->data, pkt->size);
            if (ctx->avcc)
                jmm_packet_free(ctx->avcc);
            ctx->avcc = jmm_packet_clone(pkt);
        }
        else
        {
            pkt = jmm_packet_alloc(size-5);
            if (pkt == NULL)
                return NULL;
            pkt->type = JMM_CODEC_TYPE_AVC;
            pkt->fmt = JMM_BS_FMT_AVC_MP4;
            pkt->key = ((fmt&0xf0)==0x10)?jtrue:jfalse;
            pkt->dts = (ts|(ts_ext<<24)) * 1000;
            pkt->pts = pkt->dts + ct*1000;
            jbufreader_read(ctx->rd, pkt->data, pkt->size);
        }

        //tag-n-size
        uint32_t check = RB32;
        if (check != (size+11))
        {
            jerr("[jmm_flv_demuxer] check size fail: %d, %d\n", check, size);
            jmm_packet_free(pkt);
            return NULL;
        }
        return pkt;
    }
    else if (type == 0x12) //script
    {
        int64_t pos = jbufreader_tell(ctx->rd);
        uint32_t e_type = R8;
        if (e_type == 0x2)
        {
            uint32_t e_size = RB16;
            jbufreader_skip(ctx->rd, e_size); //"onMetaData"
            e_type = R8;
            if (e_type == 0x8)
            {
                e_size = RB32;
                int i;
                for (i=0; i<e_size; i++)
                {
                    //string
                    uint32_t str_len = RB16;
                    uint8_t str[32] = {0};
                    jbufreader_read(ctx->rd, str, str_len);
                    if (!strcmp((const char *)str, "duration"))
                    {
                        jbufreader_skip(ctx->rd, 1); //0
                        uint64_t v64 = 0;
                        jbufreader_read(ctx->rd, (uint8_t *)(&v64), 8);
                        v64 = BL64(v64);
                        double *v = (double *)(&v64);
                        ctx->finfo.total_us = (*v) * 1000000;
                        continue;
                    }
                    //data
                    uint32_t data_type = R8;
                    if (data_type == 0)
                    {
                        jbufreader_skip(ctx->rd, 8);
                    }
                    else if (data_type == 0x1)
                    {
                        jbufreader_skip(ctx->rd, 1);
                    }
                    else if (data_type == 0x2)
                    {
                        uint32_t data_str_len = RB16;
                        jbufreader_skip(ctx->rd, data_str_len);
                    }
                    else
                    {
                        jbufreader_seek(ctx->rd, pos, SEEK_SET);
                        jbufreader_skip(ctx->rd, size-3);
                        break;
                    }
                }
                jbufreader_skip(ctx->rd, 3); //0 0 9
            }
            else
            {
                jbufreader_skip(ctx->rd, size-e_size-4);
            }
        }
        else
        {
            jbufreader_skip(ctx->rd, size-1);
        }
    }
    else
    {
        jbufreader_skip(ctx->rd, size);
    }

    //tag-n-size
    uint32_t check = RB32;
    if (check != (size+11))
    {
        jerr("[jmm_flv_demuxer] check size fail: %d, %d\n", check, size);
        return NULL;
    }

    goto retry;
}

static int jmm_flv_demuxer_seek(jhandle h, int64_t ts)
{
    if ((h==NULL) || (ts<0))
        return ERROR_FAIL;

    jmm_flv_demuxer_ctx *ctx = (jmm_flv_demuxer_ctx *)h;

    pthread_mutex_lock(&(ctx->mutex));

    int i;
    for (i=0; i<ctx->total_seek_entry; i++)
    {
        if (ts < ctx->seek_table[i].ts)
        {
            if (i > 0)
                i--;
            jbufreader_seek(ctx->rd, ctx->seek_table[i].pos, SEEK_SET);
            pthread_mutex_unlock(&(ctx->mutex));
            return SUCCESS;
        }
    }

    pthread_mutex_unlock(&(ctx->mutex));

    return ERROR_FAIL;
}

static int jmm_flv_demuxer_finfo(jhandle h, jmm_file_info *info)
{
    if ((h==NULL) || (info==NULL))
        return ERROR_FAIL;

    jmm_flv_demuxer_ctx *ctx = (jmm_flv_demuxer_ctx *)h;

    *info = ctx->finfo;

    return SUCCESS;
}

const jmm_demuxer jmm_flv_demuxer = {
    jmm_flv_demuxer_open,
    jmm_flv_demuxer_close,
    jmm_flv_demuxer_extradata,
    jmm_flv_demuxer_read,
    jmm_flv_demuxer_seek,
    jmm_flv_demuxer_finfo
};


