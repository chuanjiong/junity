/*
 * jmm_mp4_demuxer.c
 *
 * @chuanjiong
 */

#include "jmm_module.h"

/*

ftyp

moov
    mvhd
    trak
        tkhd
        mdia
            mdhd
            hdlr
            minf
                (vmhd, smhd, hmhd, nmhd)
                stbl
                    stsd
                    stts
                    (stsz, stz2)
                    stsc
                    (stco, co64)
                    ctts
                    stss
                dinf
                    (url , urn , dref)

mdat

*/

/*

aligned(8) class Box (unsigned int(32) boxtype, optional unsigned int(8)[16] extended_type)
{
    unsigned int(32) size;
    unsigned int(32) type = boxtype;
    if (size == 1)
    {
        unsigned int(64) largesize;
    }
    else if (size == 0)
    {
        // box extends to end of file
    }
    if (boxtype == "uuid")
    {
        unsigned int(8)[16] usertype = extended_type;
    }
}

aligned(8) class FullBox (unsigned int(32) boxtype, unsigned int(8) v, bit(24) f)
{
    extends Box(boxtype)

    unsigned int(8) version = v;
    bit(24) flags = f;
}

mp4 file structure

"ftyp"
    aligned(8) class FileTypeBox
    {
        extends Box("ftyp")

        unsigned int(32) major_brand;
        unsigned int(32) minor_version;
        unsigned int(32) compatible_brands[]; // to end of the box
    }

"moov"
    "mvhd"
        aligned(8) class MovieHeaderBox
        {
            extends FullBox("mvhd", version, 0)
            if (version == 1)
            {
                unsigned int(64) creation_time;
                unsigned int(64) modification_time;
                unsigned int(32) timescale;
                unsigned int(64) duration;
            }
            else // version == 0
            {
                unsigned int(32) creation_time;
                unsigned int(32) modification_time;
                unsigned int(32) timescale;
                unsigned int(32) duration;
            }
            template int(32) rate = 0x00010000; // typically 1.0
            template int(16) volume = 0x0100; // typically, full volume
            const bit(16) reserved = 0;
            const unsigned int(32)[2] reserved = 0;
            template int(32)[9] matrix =
            { 0x00010000,0,0,0,0x00010000,0,0,0,0x40000000 };
            // Unity matrix
            bit(32)[6] pre_defined = 0;
            unsigned int(32) next_track_ID;
        }

    "trak"
        "tkhd"
            aligned(8) class TrackHeaderBox
            {
                extends FullBox("tkhd", version, flags)
                if (version == 1)
                {
                    unsigned int(64) creation_time;
                    unsigned int(64) modification_time;
                    unsigned int(32) track_ID;
                    const unsigned int(32) reserved = 0;
                    unsigned int(64) duration;
                }
                else // version == 0
                {
                    unsigned int(32) creation_time;
                    unsigned int(32) modification_time;
                    unsigned int(32) track_ID;
                    const unsigned int(32) reserved = 0;
                    unsigned int(32) duration;
                }
                const unsigned int(32)[2] reserved = 0;
                template int(16) layer = 0;
                template int(16) alternate_group = 0;
                template int(16) volume = {if track_is_audio 0x0100 else 0};
                const unsigned int(16) reserved = 0;
                template int(32)[9] matrix=
                { 0x00010000,0,0,0,0x00010000,0,0,0,0x40000000 };
                // unity matrix
                unsigned int(32) width;
                unsigned int(32) height;
            }

        "mdia"
            "mdhd"
                aligned(8) class MediaHeaderBox
                {
                    extends FullBox("mdhd", version, 0)
                    if (version == 1)
                    {
                        unsigned int(64) creation_time;
                        unsigned int(64) modification_time;
                        unsigned int(32) timescale;
                        unsigned int(64) duration;
                    }
                    else // version == 0
                    {
                        unsigned int(32) creation_time;
                        unsigned int(32) modification_time;
                        unsigned int(32) timescale;
                        unsigned int(32) duration;
                    }
                    bit(1) pad = 0;
                    unsigned int(5)[3] language; // ISO-639-2/T language code
                    unsigned int(16) pre_defined = 0;
                }

            "hdlr"
                aligned(8) class HandlerBox
                {
                    extends FullBox("hdlr", version = 0, 0)
                    unsigned int(32) pre_defined = 0;
                    unsigned int(32) handler_type;
                    const unsigned int(32)[3] reserved = 0;
                    string name;
                }

            "minf"
                (vmhd, smhd, hmhd, nmhd)
                    aligned(8) class VideoMediaHeaderBox
                    {
                        extends FullBox("vmhd", version = 0, 1)
                        template unsigned int(16) graphicsmode = 0; // copy, see below
                        template unsigned int(16)[3] opcolor = {0, 0, 0};
                    }
                    aligned(8) class SoundMediaHeaderBox
                    {
                        extends FullBox("smhd", version = 0, 0)
                        template int(16) balance = 0;
                        const unsigned int(16) reserved = 0;
                    }
                    aligned(8) class HintMediaHeaderBox
                    {
                        extends FullBox("hmhd", version = 0, 0)
                        unsigned int(16) maxPDUsize;
                        unsigned int(16) avgPDUsize;
                        unsigned int(32) maxbitrate;
                        unsigned int(32) avgbitrate;
                        unsigned int(32) reserved = 0;
                    }
                    aligned(8) class NullMediaHeaderBox
                    {
                        extends FullBox("nmhd", version = 0, flags)
                    }

                "stbl"
                    "stsd"
                        aligned(8) abstract class SampleEntry (unsigned int(32) format)
                        {
                            extends Box(format)
                            const unsigned int(8)[6] reserved = 0;
                            unsigned int(16) data_reference_index;
                        }
                        class HintSampleEntry()
                        {
                            extends SampleEntry (protocol)
                            unsigned int(8) data [];
                        }
                        class MetaDataSampleEntry(codingname)
                        {
                            extends SampleEntry (codingname)
                        }
                        // Visual Sequences
                        class PixelAspectRatioBox
                        {
                            extends Box("pasp")
                            unsigned int(32) hSpacing;
                            unsigned int(32) vSpacing;
                        }
                        class CleanApertureBox
                        {
                            extends Box("clap")
                            unsigned int(32) cleanApertureWidthN;
                            unsigned int(32) cleanApertureWidthD;
                            unsigned int(32) cleanApertureHeightN;
                            unsigned int(32) cleanApertureHeightD;
                            unsigned int(32) horizOffN;
                            unsigned int(32) horizOffD;
                            unsigned int(32) vertOffN;
                            unsigned int(32) vertOffD;
                        }
                        class VisualSampleEntry(codingname)
                        {
                            extends SampleEntry (codingname)
                            unsigned int(16) pre_defined = 0;
                            const unsigned int(16) reserved = 0;
                            unsigned int(32)[3] pre_defined = 0;
                            unsigned int(16) width;
                            unsigned int(16) height;
                            template unsigned int(32) horizresolution = 0x00480000; // 72 dpi
                            template unsigned int(32) vertresolution = 0x00480000; // 72 dpi
                            const unsigned int(32) reserved = 0;
                            template unsigned int(16) frame_count = 1;
                            string[32] compressorname;
                            template unsigned int(16) depth = 0x0018;
                            int(16) pre_defined = -1;
                            // other boxes from derived specifications
                            CleanApertureBox clap; // optional
                            PixelAspectRatioBox pasp; // optional
                        }
                        // Audio Sequences
                        class AudioSampleEntry(codingname)
                        {
                            extends SampleEntry (codingname)
                            const unsigned int(32)[2] reserved = 0;
                            template unsigned int(16) channelcount = 2;
                            template unsigned int(16) samplesize = 16;
                            unsigned int(16) pre_defined = 0;
                            const unsigned int(16) reserved = 0 ;
                            template unsigned int(32) samplerate = { default samplerate of media}<<16;
                        }
                        aligned(8) class SampleDescriptionBox (unsigned int(32) handler_type)
                        {
                            extends FullBox("stsd", 0, 0)
                            int i;
                            unsigned int(32) entry_count;
                            for (i=1; i<=entry_count; i++)
                            {
                                switch (handler_type)
                                {
                                    case "soun": // for audio tracks
                                        AudioSampleEntry();
                                        break;
                                    case "vide": // for video tracks
                                        VisualSampleEntry();
                                        break;
                                    case "hint": // Hint track
                                        HintSampleEntry();
                                        break;
                                    case "meta": // Metadata track
                                        MetadataSampleEntry();
                                        break;
                                }
                            }
                        }

                        //es
                        {
                            //mp4a
                            (8) tag; (0x5 -> asc)
                            (1~4) size;
                        }

                        aligned(8) class ESDBox
                        {
                            extends FullBox("esds", version = 0, 0)
                            ES Descriptor ES;
                        }
                        // Visual Streams
                        class MP4VisualSampleEntry()
                        {
                            extends VisualSampleEntry ("mp4v")
                            ESDBox ES;
                        }
                        // Audio Streams
                        class MP4AudioSampleEntry()
                        {
                            extends AudioSampleEntry ("mp4a")
                            ESDBox ES;
                        }
                        // all other Mpeg stream types
                        class MpegSampleEntry()
                        {
                            extends SampleEntry ("mp4s")
                            ESDBox ES;
                        }

                    "stts"
                        aligned(8) class TimeToSampleBox
                        {
                            extends FullBox("stts", version = 0, 0)
                            unsigned int(32) entry_count;
                            int i;
                            for (i=0; i < entry_count; i++)
                            {
                                unsigned int(32) sample_count;
                                unsigned int(32) sample_delta;
                            }
                        }

                    (stsz, stz2)
                        aligned(8) class SampleSizeBox
                        {
                            extends FullBox("stsz", version = 0, 0)
                            unsigned int(32) sample_size;
                            unsigned int(32) sample_count;
                            if (sample_size==0)
                            {
                                for (i=1; i <= sample_count; i++)
                                {
                                    unsigned int(32) entry_size;
                                }
                            }
                        }
                        aligned(8) class CompactSampleSizeBox
                        {
                            extends FullBox("stz2", version = 0, 0)
                            unsigned int(24) reserved = 0;
                            unisgned int(8) field_size;
                            unsigned int(32) sample_count;
                            for (i=1; i <= sample_count; i++)
                            {
                                unsigned int(field_size) entry_size;
                            }
                        }

                    stsc
                        aligned(8) class SampleToChunkBox
                        {
                            extends FullBox("stsc", version = 0, 0)
                            unsigned int(32) entry_count;
                            for (i=1; i <= entry_count; i++)
                            {
                                unsigned int(32) first_chunk;
                                unsigned int(32) samples_per_chunk;
                                unsigned int(32) sample_description_index;
                            }
                        }

                    (stco, co64)
                        aligned(8) class ChunkOffsetBox
                        {
                            extends FullBox("stco", version = 0, 0)
                            unsigned int(32) entry_count;
                            for (i=1; i <= entry_count; i++)
                            {
                                unsigned int(32) chunk_offset;
                            }
                        }
                        aligned(8) class ChunkLargeOffsetBox
                        {
                            extends FullBox("co64", version = 0, 0)
                            unsigned int(32) entry_count;
                            for (i=1; i <= entry_count; i++)
                            {
                                unsigned int(64) chunk_offset;
                            }
                        }

                    "ctts"
                        aligned(8) class CompositionOffsetBox
                        {
                            extends FullBox("ctts", version = 0, 0)
                            unsigned int(32) entry_count;
                            int i;
                            if (version == 0)
                            {
                                for (i=0; i<entry_count; i++)
                                {
                                    unsigned int(32) sample_count;
                                    unsigned int(32) sample_offset;
                                }
                            }
                            else if (version == 1)
                            {
                                for (i=0; i<entry_count; i++)
                                {
                                    unsigned int(32) sample_count;
                                    signed int(32) sample_offset;
                                }
                            }
                        }

                    "stss"
                        aligned(8) class SyncSampleBox
                        {
                            extends FullBox("stss", version = 0, 0)
                            unsigned int(32) entry_count;
                            int i;
                            for (i=0; i<entry_count; i++)
                            {
                                unsigned int(32) sample_number;
                            }
                        }

                "dinf"
                    (url , urn , dref)
                        aligned(8) class DataEntryUrlBox (bit(24) flags)
                        {
                            extends FullBox("url ", version = 0, flags)
                            string location;
                        }
                        aligned(8) class DataEntryUrnBox (bit(24) flags)
                        {
                            extends FullBox("urn ", version = 0, flags)
                            string name;
                            string location;
                        }
                        aligned(8) class DataReferenceBox
                        {
                            extends FullBox("dref", version = 0, 0)
                            unsigned int(32) entry_count;
                            for (i=1; i <= entry_count; i++)
                            {
                                DataEntryBox(entry_version, entry_flags) data_entry;
                            }
                        }

"mdat"

*/

#define fourcc(a,b,c,d)         ((a)|((b)<<8)|((c)<<16)|((d)<<24))

typedef struct jmm_mp4_demuxer_entry {
    jbool key;
    int64_t dts;
    int64_t pts;
    int size;
    int64_t offset;
}jmm_mp4_demuxer_entry;

typedef struct jmm_mp4_demuxer_stream {
    //stts
    int sttsSize;
    uint32_t *stts;

    //stsc
    int stscSize;
    uint32_t *stsc;

    //stco
    int stcoSize;
    uint32_t *stco;

    //co64
    int co64Size;
    uint64_t *co64;

    //stsz
    int simpleSize;
    int stszSize;
    uint32_t *stsz;

    //ctts
    jbool cttsNegative;
    int cttsSize;
    uint32_t *ctts;

    //stss
    int stssSize;
    uint32_t *stss;

    //total sample size
    int total;

    int timebase;
}jmm_mp4_demuxer_stream;

typedef struct jmm_mp4_demuxer_ctx {
    jhandle rd;

    jbool finda;
    jbool findv;
    jbool findMeta;

    jmm_mp4_demuxer_stream astream;
    jmm_mp4_demuxer_stream vstream;

    uint32_t cur_handler_type;
    jmm_mp4_demuxer_stream *cur_stream;

    int timebase;

    jmm_asc_info ainfo;

    jmm_packet *asc;
    jmm_packet *avcc;

    jbool getASC;
    jbool getAVCC;

    int aidx;
    jmm_mp4_demuxer_entry *aentry;
    int vidx;
    jmm_mp4_demuxer_entry *ventry;

    jmm_file_info finfo;
}jmm_mp4_demuxer_ctx;

static uint32_t getTsize(jmm_mp4_demuxer_ctx *ctx, uint32_t *n)
{
    uint32_t size = 0;
    int i;
    for (i=0; i<4; i++)
    {
        uint32_t s = R8;
        size = (size<<7) | (s&0x7f);
        if (!(s & 0x80))
            break;
    }
    if (n)
        *n = i+1;
    return size;
}

static int jmm_mp4_demuxer_parse_avc1(jmm_mp4_demuxer_ctx *ctx)
{
    //avc1
    uint32_t bsize = RB32;
    uint32_t btype = RL32;
    if (btype == fourcc('a','v','c','1'))
    {
        jbufreader_skip(ctx->rd, 8);
        jbufreader_skip(ctx->rd, 16);
        uint32_t width = RB16;
        uint32_t height = RB16;
        jlog("[jmm_mp4_demuxer] width: %d, height %d\n", width, height);
        jbufreader_skip(ctx->rd, 50);
        bsize = bsize-4-4-8-16-2-2-50;
        while (bsize > 0)
        {
            //avcC
            uint32_t bbsize = RB32;
            uint32_t bbtype = RL32;
            if (bbtype == fourcc('a','v','c','C'))
            {
                ctx->avcc = jmm_packet_alloc(bbsize-8);
                if (ctx->avcc == NULL)
                    return ERROR_FAIL;
                ctx->avcc->type = JMM_CODEC_TYPE_AVC;
                ctx->avcc->fmt = JMM_BS_FMT_AVC_AVCC;
                ctx->avcc->key = jtrue;
                ctx->avcc->dts = 0;
                ctx->avcc->pts = 0;
                jbufreader_read(ctx->rd, ctx->avcc->data, ctx->avcc->size);
            }
            else
            {
                jbufreader_skip(ctx->rd, bbsize-8);
            }
            bsize -= bbsize;
        }
    }
    else
    {
        jbufreader_skip(ctx->rd, bsize-8);
    }
    return SUCCESS;
}

static int jmm_mp4_demuxer_parse_mp4a(jmm_mp4_demuxer_ctx *ctx)
{
    //mp4a
    uint32_t bsize = RB32;
    uint32_t btype = RL32;
    if (btype == fourcc('m','p','4','a'))
    {
        jbufreader_skip(ctx->rd, 8);
        jbufreader_skip(ctx->rd, 8);
        uint32_t channels = RB16;
        jlog("[jmm_mp4_demuxer] channels: %d\n", channels);
        jbufreader_skip(ctx->rd, 6);
        uint32_t samplerate = RB32;
        jlog("[jmm_mp4_demuxer] samplerate: %d\n", samplerate>>16);
        bsize = bsize-4-4-8-8-2-6-4;
        while (bsize > 0)
        {
            //esds
            uint32_t bbsize = RB32;
            uint32_t bbtype = RL32;
            if (bbtype == fourcc('e','s','d','s'))
            {
                //full box
                RB32;
                uint32_t total = bbsize-8-4;
                while (total > 0)
                {
                    uint32_t tag = R8;
                    uint32_t tsizesize = 0;
                    uint32_t tsize = getTsize(ctx, &tsizesize);
                    total = total-1-tsizesize-tsize;
                    if (tag == 0x3) //MP4ESDescrTag
                    {
                        /*
                        ES_DescrTag
                        {
                            bit(16) ES_ID;
                            bit(1) streamDependenceFlag;
                            bit(1) URL_Flag;
                            bit(1) OCRstreamFlag;
                            bit(5) streamPriority;
                            if (streamDependenceFlag)
                                bit(16) dependsOn_ES_ID;
                            if (URL_Flag)
                            {
                                bit(8) URLlength;
                                bit(8) URLstring[URLlength];
                            }
                            if (OCRstreamFlag)
                                bit(16) OCR_ES_Id;
                        }
                        */
                        RB16;
                        uint32_t flag = R8;
                        tsize -= 3;
                        if (flag & 0x80)
                        {
                            jbufreader_skip(ctx->rd, 2);
                            tsize -= 2;
                        }
                        if (flag & 0x40)
                        {
                            uint32_t len = R8;
                            jbufreader_skip(ctx->rd, len);
                            tsize = tsize-1-len;
                        }
                        if (flag & 0x20)
                        {
                            jbufreader_skip(ctx->rd, 2);
                            tsize -= 2;
                        }
                        //MP4DecConfigDescrTag
                        uint32_t tag2 = R8;
                        uint32_t tsizesize2 = 0;
                        uint32_t tsize2 = getTsize(ctx, &tsizesize2);
                        tsize = tsize-1-tsizesize2-tsize2;
                        if (tag2 == 0x4) //MP4DecConfigDescrTag
                        {
                            uint32_t codec = R8;
                            if (codec != 0x40) //aac
                                return ERROR_FAIL;
                            jbufreader_skip(ctx->rd, 12);
                            uint32_t tag3 = R8;
                            uint32_t tsizesize3 = 0;
                            uint32_t tsize3 = getTsize(ctx, &tsizesize3);
                            tsize2 = tsize2-1-12-1-tsizesize3-tsize3;
                            if (tag3 == 0x5) //MP4DecSpecificDescrTag
                            {
                                //asc
                                ctx->asc = jmm_packet_alloc(tsize3);
                                if (ctx->asc == NULL)
                                    return ERROR_FAIL;
                                ctx->asc->type = JMM_CODEC_TYPE_AAC;
                                ctx->asc->fmt = JMM_BS_FMT_AAC_ASC;
                                ctx->asc->key = jtrue;
                                ctx->asc->dts = 0;
                                ctx->asc->pts = 0;
                                jbufreader_read(ctx->rd, ctx->asc->data, ctx->asc->size);
                                jmm_aac_asc_parse(ctx->asc, &(ctx->ainfo));
                            }
                            jbufreader_skip(ctx->rd, tsize2);
                        }
                        jbufreader_skip(ctx->rd, tsize);
                    }
                    else
                    {
                        jbufreader_skip(ctx->rd, tsize);
                    }
                }
            }
            bsize -= bbsize;
        }
    }
    else
    {
        jbufreader_skip(ctx->rd, bsize-8);
    }
    return SUCCESS;
}

static int jmm_mp4_demuxer_parse_box(jmm_mp4_demuxer_ctx *ctx)
{
    if (ctx == NULL)
        return ERROR_FAIL;

    while ((!jbufreader_eof(ctx->rd)) && (!ctx->findMeta))
    {
        uint64_t size = RB32;
        uint32_t type = RL32;

        if (size == 1) //64-bit
        {
            size = (((uint64_t)RB32)<<32) | RB32;
        }
        else if (size == 0) //end of box
            return SUCCESS;

        if (type == fourcc('u','u','i','d'))
            jbufreader_skip(ctx->rd, 16);

        switch (type)
        {
            case fourcc('f','t','y','p'):
                {
                    uint32_t major_brand = RL32;
                    if ((major_brand!=fourcc('m','p','4','2')) && (major_brand!=fourcc('i','s','o','m')))
                        return ERROR_FAIL;
                    jbufreader_skip(ctx->rd, size-8-4);
                }
                break;

            case fourcc('m','o','o','v'):
                if (jmm_mp4_demuxer_parse_box(ctx) != SUCCESS)
                    return ERROR_FAIL;
                ctx->findMeta = jtrue;
                break;

            case fourcc('t','r','a','k'):
            case fourcc('m','d','i','a'):
            case fourcc('m','i','n','f'):
            case fourcc('s','t','b','l'):
                if (jmm_mp4_demuxer_parse_box(ctx) != SUCCESS)
                    return ERROR_FAIL;
                break;

            case fourcc('m','d','h','d'):
                {
                    //full box
                    uint32_t version = R8;
                    RB24;
                    if (version == 1)
                    {
                        jbufreader_skip(ctx->rd, 16);
                        ctx->timebase = RB32;
                        //jbufreader_skip(ctx->rd, 8);
                        uint64_t duration = (((uint64_t)RB32)<<32)|RB32;
                        int64_t total_us = duration / ctx->timebase * 1000000;
                        if (total_us > ctx->finfo.total_us)
                            ctx->finfo.total_us = total_us;
                    }
                    else
                    {
                        jbufreader_skip(ctx->rd, 8);
                        ctx->timebase = RB32;
                        //jbufreader_skip(ctx->rd, 4);
                        uint32_t duration = RB32;
                        int64_t total_us = (int64_t)duration / ctx->timebase * 1000000;
                        if (total_us > ctx->finfo.total_us)
                            ctx->finfo.total_us = total_us;
                    }
                    jbufreader_skip(ctx->rd, 4);
                }
                break;

            case fourcc('h','d','l','r'):
                {
                    //full box
                    RB32;
                    //pre_defined
                    RB32;
                    ctx->cur_handler_type = RL32;
                    if (ctx->cur_handler_type == fourcc('v','i','d','e'))
                    {
                        ctx->findv = jtrue;
                        ctx->cur_stream = &(ctx->vstream);
                    }
                    else if (ctx->cur_handler_type == fourcc('s','o','u','n'))
                    {
                        ctx->finda = jtrue;
                        ctx->cur_stream = &(ctx->astream);
                    }
                    ctx->cur_stream->timebase = ctx->timebase;
                    jbufreader_skip(ctx->rd, size-8-12);
                }
                break;

            case fourcc('s','t','s','d'):
                {
                    //full box
                    RB32;
                    int i;
                    uint32_t entry_count = RB32;
                    for (i=1; i<=entry_count; i++)
                    {
                        switch (ctx->cur_handler_type)
                        {
                            case fourcc('v','i','d','e'):
                                if (jmm_mp4_demuxer_parse_avc1(ctx) != SUCCESS)
                                    return ERROR_FAIL;
                                break;

                            case fourcc('s','o','u','n'):
                                if (jmm_mp4_demuxer_parse_mp4a(ctx) != SUCCESS)
                                    return ERROR_FAIL;
                                break;

                            default:
                                i=entry_count;
                                jbufreader_skip(ctx->rd, size-8-4-4);
                                break;
                        }
                    }
                }
                break;

            case fourcc('s','t','t','s'):
                {
                    //full box
                    RB32;
                    uint32_t entry_count = RB32;
                    ctx->cur_stream->sttsSize = entry_count;
                    ctx->cur_stream->stts = (uint32_t *)jmalloc(entry_count * 2 * 4);
                    if (ctx->cur_stream->stts == NULL)
                        return ERROR_FAIL;
                    jbufreader_read(ctx->rd, (uint8_t*)(ctx->cur_stream->stts), entry_count*2*4);
                }
                break;

            case fourcc('s','t','s','c'):
                {
                    //full box
                    RB32;
                    uint32_t entry_count = RB32;
                    ctx->cur_stream->stscSize = entry_count;
                    ctx->cur_stream->stsc = (uint32_t *)jmalloc(entry_count * 3 * 4);
                    if (ctx->cur_stream->stsc == NULL)
                        return ERROR_FAIL;
                    jbufreader_read(ctx->rd, (uint8_t*)(ctx->cur_stream->stsc), entry_count*3*4);
                }
                break;

            case fourcc('s','t','c','o'):
                {
                    //full box
                    RB32;
                    uint32_t entry_count = RB32;
                    ctx->cur_stream->stcoSize = entry_count;
                    ctx->cur_stream->stco = (uint32_t *)jmalloc(entry_count * 4);
                    if (ctx->cur_stream->stco == NULL)
                        return ERROR_FAIL;
                    jbufreader_read(ctx->rd, (uint8_t*)(ctx->cur_stream->stco), entry_count*4);
                }
                break;

            case fourcc('c','o','6','4'):
                {
                    //full box
                    RB32;
                    uint32_t entry_count = RB32;
                    ctx->cur_stream->co64Size = entry_count;
                    ctx->cur_stream->co64 = (uint64_t *)jmalloc(entry_count * 8);
                    if (ctx->cur_stream->co64 == NULL)
                        return ERROR_FAIL;
                    jbufreader_read(ctx->rd, (uint8_t*)(ctx->cur_stream->co64), entry_count*8);
                }
                break;

            case fourcc('s','t','s','z'):
                {
                    //full box
                    RB32;
                    uint32_t sample_size = RB32;
                    uint32_t sample_count = RB32;
                    ctx->cur_stream->simpleSize = sample_size;
                    if (sample_size == 0)
                    {
                        ctx->cur_stream->stszSize = sample_count;
                        ctx->cur_stream->stsz = (uint32_t *)jmalloc(sample_count * 4);
                        if (ctx->cur_stream->stsz == NULL)
                            return ERROR_FAIL;
                        jbufreader_read(ctx->rd, (uint8_t*)(ctx->cur_stream->stsz), sample_count*4);
                    }
                }
                break;

            case fourcc('s','t','s','s'):
                {
                    //full box
                    RB32;
                    uint32_t entry_count = RB32;
                    ctx->cur_stream->stssSize = entry_count;
                    ctx->cur_stream->stss = (uint32_t *)jmalloc(entry_count * 4);
                    if (ctx->cur_stream->stss == NULL)
                        return ERROR_FAIL;
                    jbufreader_read(ctx->rd, (uint8_t*)(ctx->cur_stream->stss), entry_count*4);
                }
                break;

            case fourcc('c','t','t','s'):
                {
                    //full box
                    uint32_t version = R8;
                    RB24;
                    if (version == 0)
                        ctx->cur_stream->cttsNegative = jfalse;
                    else
                        ctx->cur_stream->cttsNegative = jtrue;
                    uint32_t entry_count = RB32;
                    ctx->cur_stream->cttsSize = entry_count;
                    ctx->cur_stream->ctts = (uint32_t *)jmalloc(entry_count * 2 * 4);
                    if (ctx->cur_stream->ctts == NULL)
                        return ERROR_FAIL;
                    jbufreader_read(ctx->rd, (uint8_t*)(ctx->cur_stream->ctts), entry_count*2*4);
                }
                break;

            default:
                jlog("[jmm_mp4_demuxer] skip box:%4.4s, size: %lld\n", (const char *)(&type), size);
                jbufreader_skip(ctx->rd, size-8);
                break;
        }
    }

    return SUCCESS;
}

static int jmm_mp4_demuxer_calc_entry(jmm_mp4_demuxer_stream *stream, jmm_mp4_demuxer_entry **entry)
{
    if ((stream==NULL) || (entry==NULL))
        return ERROR_FAIL;

    int i;

    //total sample number
    int total=0;
    if (stream->simpleSize == 0)
        total = stream->stszSize;
    else
    {
        for (i=0; i<stream->sttsSize; i++)
            total += BL32(stream->stts[i*2]);
    }

    stream->total = total;

    //entry pool
    *entry = (jmm_mp4_demuxer_entry *)jmalloc(total * sizeof(jmm_mp4_demuxer_entry));
    if (*entry == NULL)
        return ERROR_FAIL;
    memset(*entry, 0, total * sizeof(jmm_mp4_demuxer_entry));

    //offset
    int chunk_idx = 0;
    int inter_idx = 0;
    int prev_total = 0;

    int entry_idx = 0;
    int chunk_sample_num=BL32(stream->stsc[entry_idx*3+1]);
    int entry_next;
    if (entry_idx < (stream->stscSize-1))
        entry_next = BL32(stream->stsc[entry_idx*3+3])-1;
    else
        entry_next = 0x7fffffff;

    for (i=0; i<total; i++)
    {
        if (stream->simpleSize == 0)
            (*entry)[i].size = BL32(stream->stsz[i]);
        else
            (*entry)[i].size = stream->simpleSize;

        //chunk offset + prev sample total offset
        if (stream->stcoSize > 0)
            (*entry)[i].offset = BL32(stream->stco[chunk_idx]) + prev_total;
        else
            (*entry)[i].offset = BL64(stream->co64[chunk_idx]) + prev_total;

        inter_idx++;
        if (inter_idx >= chunk_sample_num)
        {
            inter_idx = 0;
            prev_total = 0;
            chunk_idx++;
            if (chunk_idx >= entry_next)
            {
                entry_idx++;
                chunk_sample_num = BL32(stream->stsc[entry_idx*3+1]);
                if (entry_idx < (stream->stscSize-1))
                    entry_next = BL32(stream->stsc[entry_idx*3+3])-1;
                else
                    entry_next = 0x7fffffff;
            }
        }
        else
        {
            prev_total += (*entry)[i].size;
        }

        //actually, audio do not care this field
        (*entry)[i].key = jfalse;
    }

    //key
    if (stream->stssSize > 0)
        for (i=0; i<stream->stssSize; i++)
            (*entry)[BL32(stream->stss[i])-1].key = jtrue;

    //ts
    int sidx = 0;
    int cidx = 0;
    int sentry = 0;
    int centry = 0;
    int sidx_max = BL32(stream->stts[sentry*2]);
    int cidx_max;
    if (stream->cttsSize > 0)
        cidx_max = BL32(stream->ctts[centry*2]);
    int64_t prevts = 0;
    for (i=0; i<total; i++)
    {
        (*entry)[i].dts = BL32(stream->stts[sentry*2+1]) + prevts;

        sidx++;
        if (sidx >= sidx_max)
        {
            sidx = 0;
            sentry++;
            sidx_max = BL32(stream->stts[sentry*2]);
        }

        if (stream->cttsSize > 0)
        {
            (*entry)[i].pts = (*entry)[i].dts + BL32(stream->ctts[centry*2+1]);
            cidx++;
            if (cidx >= cidx_max)
            {
                cidx = 0;
                centry++;
                cidx_max = BL32(stream->ctts[centry*2]);
            }
        }
        else
            (*entry)[i].pts = (*entry)[i].dts;

        prevts = (*entry)[i].dts;
    }

    for (i=0; i<total; i++)
    {
        (*entry)[i].pts = (*entry)[i].pts * 1000000 / stream->timebase;
        (*entry)[i].dts = (*entry)[i].dts * 1000000 / stream->timebase;
    }

    return SUCCESS;
}

static jhandle jmm_mp4_demuxer_open(const char *url)
{
    if (url == NULL)
        return NULL;

    jmm_mp4_demuxer_ctx *ctx = (jmm_mp4_demuxer_ctx *)jmalloc(sizeof(jmm_mp4_demuxer_ctx));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(jmm_mp4_demuxer_ctx));

    ctx->rd = jbufreader_open(url);
    if (ctx->rd == NULL)
    {
        jfree(ctx);
        return NULL;
    }

    if (jmm_mp4_demuxer_parse_box(ctx) != SUCCESS)
    {
        jbufreader_close(ctx->rd);
        jfree(ctx);
        return NULL;
    }

    jmm_mp4_demuxer_calc_entry(&(ctx->astream), &(ctx->aentry));

    jmm_mp4_demuxer_calc_entry(&(ctx->vstream), &(ctx->ventry));

    return ctx;
}

static void jmm_mp4_demuxer_free_stream(jmm_mp4_demuxer_stream *stream)
{
    if (stream == NULL)
        return;

    if (stream->sttsSize > 0)
        jfree(stream->stts);

    if (stream->stscSize > 0)
        jfree(stream->stsc);

    if (stream->stcoSize > 0)
        jfree(stream->stco);

    if (stream->co64Size > 0)
        jfree(stream->co64);

    if (stream->stszSize > 0)
        jfree(stream->stsz);

    if (stream->cttsSize > 0)
        jfree(stream->ctts);

    if (stream->stssSize > 0)
        jfree(stream->stss);
}

static void jmm_mp4_demuxer_close(jhandle h)
{
    if (h == NULL)
        return;

    jmm_mp4_demuxer_ctx *ctx = (jmm_mp4_demuxer_ctx *)h;

    jmm_mp4_demuxer_free_stream(&(ctx->astream));

    jmm_mp4_demuxer_free_stream(&(ctx->vstream));

    if (ctx->asc)
        jmm_packet_free(ctx->asc);

    if (ctx->avcc)
        jmm_packet_free(ctx->avcc);

    if (ctx->aentry)
        jfree(ctx->aentry);

    if (ctx->ventry)
        jfree(ctx->ventry);

    jbufreader_close(ctx->rd);

    jfree(ctx);
}

static jmm_packet *jmm_mp4_demuxer_extradata(jhandle h, jmm_codec_type type)
{
    if (h == NULL)
        return NULL;

    jmm_mp4_demuxer_ctx *ctx = (jmm_mp4_demuxer_ctx *)h;

    if (type == JMM_CODEC_TYPE_AAC)
        return jmm_packet_clone(ctx->asc);
    else
        return jmm_packet_clone(ctx->avcc);
}

static jmm_packet *jmm_mp4_demuxer_read(jhandle h)
{
    if (h == NULL)
        return NULL;

    jmm_mp4_demuxer_ctx *ctx = (jmm_mp4_demuxer_ctx *)h;

    if (ctx->asc && (!ctx->getASC))
    {
        ctx->getASC = jtrue;
        return jmm_packet_clone(ctx->asc);
    }

    if (ctx->avcc && (!ctx->getAVCC))
    {
        ctx->getAVCC = jtrue;
        return jmm_packet_clone(ctx->avcc);
    }

    jmm_packet *pkt;

    if ((ctx->astream.total>0) && (ctx->aentry[ctx->aidx].dts<ctx->ventry[ctx->vidx].dts) && (ctx->aidx<ctx->astream.total))
    {
        //audio
        jbufreader_seek(ctx->rd, ctx->aentry[ctx->aidx].offset, SEEK_SET);

        uint32_t size = ctx->aentry[ctx->aidx].size;
        pkt = jmm_packet_alloc(size);
        if (pkt == NULL)
            return NULL;
        jbufreader_read(ctx->rd, pkt->data, pkt->size);
        pkt->type = JMM_CODEC_TYPE_AAC;
        pkt->fmt = JMM_BS_FMT_AAC_ES;
        pkt->key = ctx->aentry[ctx->aidx].key;
        pkt->dts = ctx->aentry[ctx->aidx].dts;
        pkt->pts = ctx->aentry[ctx->aidx].pts;

        jmm_packet *tmp = pkt;
        pkt = jmm_aac_es2adts(pkt, &(ctx->ainfo), jfalse);
        if (pkt == NULL)
            jmm_packet_free(tmp);

        ctx->aidx++;
    }
    else if ((ctx->vstream.total>0) && (ctx->vidx<ctx->vstream.total))
    {
        //video
        jbufreader_seek(ctx->rd, ctx->ventry[ctx->vidx].offset, SEEK_SET);

        uint32_t size = ctx->ventry[ctx->vidx].size;
        pkt = jmm_packet_alloc(size);
        if (pkt == NULL)
            return NULL;
        jbufreader_read(ctx->rd, pkt->data, pkt->size);
        pkt->type = JMM_CODEC_TYPE_AVC;
        pkt->fmt = JMM_BS_FMT_AVC_MP4;
        pkt->key = ctx->ventry[ctx->vidx].key;
        pkt->dts = ctx->ventry[ctx->vidx].dts;
        pkt->pts = ctx->ventry[ctx->vidx].pts;

        ctx->vidx++;
    }
    else
        return NULL;

    return pkt;
}

static int jmm_mp4_demuxer_seek(jhandle h, int64_t ts)
{
    if ((h==NULL) || (ts<0))
        return ERROR_FAIL;

    jmm_mp4_demuxer_ctx *ctx = (jmm_mp4_demuxer_ctx *)h;

    int i;
    //video
    for (i=0; i<ctx->vstream.total; i++)
    {
        if (ts < ctx->ventry[i].dts)
        {
            if (i > 0)
                i--;
            //find I-frame
            while ((!ctx->ventry[i].key) && (i>0))
                i--;
            ctx->vidx = i;
            ts = ctx->ventry[ctx->vidx].dts;
            break;
        }
    }

    //audio
    for (i=0; i<ctx->astream.total; i++)
    {
        if (ts < ctx->aentry[i].dts)
        {
            if (i > 0)
                i--;
            ctx->aidx = i;
            break;
        }
    }

    return SUCCESS;
}

static int jmm_mp4_demuxer_finfo(jhandle h, jmm_file_info *info)
{
    if ((h==NULL) || (info==NULL))
        return ERROR_FAIL;

    jmm_mp4_demuxer_ctx *ctx = (jmm_mp4_demuxer_ctx *)h;

    *info = ctx->finfo;

    return SUCCESS;
}

const jmm_demuxer jmm_mp4_demuxer = {
    jmm_mp4_demuxer_open,
    jmm_mp4_demuxer_close,
    jmm_mp4_demuxer_extradata,
    jmm_mp4_demuxer_read,
    jmm_mp4_demuxer_seek,
    jmm_mp4_demuxer_finfo
};


