/*
 * jmm_mp4_muxer.c
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

#define write_fourcc(a,b,c,d)       (WB32(((a)<<24)|((b)<<16)|((c)<<8)|(d)))

typedef struct jmm_mp4_muxer_entry {
    jbool key;
    int64_t dts;
    int64_t pts;
    int64_t duration;
    int size;
    int64_t offset;
}jmm_mp4_muxer_entry;

typedef struct jmm_mp4_muxer_ctx {
    jhandle wt;

    time_t creation_time;

    int64_t mdat_size_slot;

    jmm_packet *asc;
    jmm_packet *avcc;

    int aidx;
    jdynarray(jmm_mp4_muxer_entry, aentry);
    int vidx;
    jdynarray(jmm_mp4_muxer_entry, ventry);

    //second
    int64_t adur;
    int64_t vdur;

    jmm_asc_info ainfo;
    jmm_avcc_info vinfo;
}jmm_mp4_muxer_ctx;

static jhandle jmm_mp4_muxer_open(const char *url)
{
    if (url == NULL)
        return NULL;

    jmm_mp4_muxer_ctx *ctx = (jmm_mp4_muxer_ctx *)jmalloc(sizeof(jmm_mp4_muxer_ctx));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(jmm_mp4_muxer_ctx));

    ctx->wt = jbufwriter_open(url, 0);
    if (ctx->wt == NULL)
    {
        jfree(ctx);
        return NULL;
    }

    //0x7C25B080 1970 -> 1904
    ctx->creation_time = time(NULL)+0x7C25B080;

    //ftyp
    uint8_t buf[32] = {0, 0, 0, 0x18, 'f', 't', 'y', 'p', 'm', 'p', '4', '2', 0, 0, 0, 0, 'i', 's', 'o', 'm', 'm', 'p', '4', '2'};

    jbufwriter_write(ctx->wt, buf, 0x18);

    //mdat - 64bit
    //size slot
    WB32(1);
    write_fourcc('m','d','a','t');
    ctx->mdat_size_slot = jbufwriter_tell(ctx->wt);
    WB32(0);
    WB32(0);

    jdynarray_alloc(jmm_mp4_muxer_entry, ctx->aentry, 1024);
    jdynarray_alloc(jmm_mp4_muxer_entry, ctx->ventry, 1024);

    return ctx;
}

static int jmm_mp4_muxer_mvhd(jmm_mp4_muxer_ctx *ctx)
{
    if (ctx == NULL)
        return ERROR_FAIL;

    //size
    WB32(0x6c);
    //fourcc
    write_fourcc('m','v','h','d');
    //full box
    WB32(0);
    //creation_time
    //modification_time
    WB32(ctx->creation_time);
    WB32(ctx->creation_time);
    //timescale
    WB32(90000);
    //duration
    if (ctx->adur > ctx->vdur)
        WB32(ctx->adur*90000);
    else
        WB32(ctx->vdur*90000);
    WB32(0x00010000);
    WB16(0x0100);
    jbufwriter_dump(ctx->wt, 10, 0);
    //matrix
    WB32(0x00010000);
    WB32(0);
    WB32(0);
    WB32(0);
    WB32(0x00010000);
    WB32(0);
    WB32(0);
    WB32(0);
    WB32(0x40000000);
    jbufwriter_dump(ctx->wt, 24, 0);
    //next_track_ID
    WB32(3);

    return SUCCESS;
}

static int jmm_mp4_muxer_tkhd(jmm_mp4_muxer_ctx *ctx, jmm_mp4_muxer_entry *entry)
{
    if ((ctx==NULL) || (entry==NULL))
        return ERROR_FAIL;

    //size
    WB32(0x5c);
    //fourcc
    write_fourcc('t','k','h','d');
    //full box
    //version
    W8(0);
    //flags -> enabled | in_moive
    WB24(0x3);
    //creation_time
    //modification_time
    WB32(ctx->creation_time);
    WB32(ctx->creation_time);
    if (entry == ctx->aentry)
    {
        //track_ID
        WB32(2);
        WB32(0);
        //duration
        WB32(ctx->adur*90000);
    }
    else
    {
        //track_ID
        WB32(1);
        WB32(0);
        //duration
        WB32(ctx->vdur*90000);
    }
    jbufwriter_dump(ctx->wt, 12, 0);
    if (entry == ctx->aentry)
        WB16(0x0100);
    else
        WB16(0);
    WB16(0);
    //matrix
    WB32(0x00010000);
    WB32(0);
    WB32(0);
    WB32(0);
    WB32(0x00010000);
    WB32(0);
    WB32(0);
    WB32(0);
    WB32(0x40000000);
    //width height 16.16 fixed-point
    WB16(ctx->vinfo.width);
    WB16(0);
    WB16(ctx->vinfo.height);
    WB16(0);

    return SUCCESS;
}

static int jmm_mp4_muxer_mdhd(jmm_mp4_muxer_ctx *ctx, jmm_mp4_muxer_entry *entry)
{
    if ((ctx==NULL) || (entry==NULL))
        return ERROR_FAIL;

    //size
    WB32(0x20);
    //fourcc
    write_fourcc('m','d','h','d');
    //full box
    WB32(0);
    //creation_time
    //modification_time
    WB32(ctx->creation_time);
    WB32(ctx->creation_time);
    if (entry == ctx->aentry)
    {
        //timescale
        WB32(ctx->ainfo.samplerate);
        //duration
        WB32(ctx->adur*ctx->ainfo.samplerate);
    }
    else
    {
        //timescale
        WB32(90000);
        //duration
        WB32(ctx->vdur*90000);
    }
    //language ISO-639-2/T language code
    //und
    WB16(0x55c4);
    WB16(0);

    return SUCCESS;
}

static int jmm_mp4_muxer_hdlr(jmm_mp4_muxer_ctx *ctx, jmm_mp4_muxer_entry *entry)
{
    if ((ctx==NULL) || (entry==NULL))
        return ERROR_FAIL;

    //size
    int slot = jbufwriter_tell(ctx->wt);
    WB32(0);
    //fourcc
    write_fourcc('h','d','l','r');

    //full box
    WB32(0);
    //pre_defined
    WB32(0);
    //handler_type
    if (entry == ctx->aentry)
        write_fourcc('s','o','u','n');
    else
        write_fourcc('v','i','d','e');
    jbufwriter_dump(ctx->wt, 12, 0);

    //name -> :)
    const char *my_name = "This file is generated by jmm_mp4_muxer(mm library writed by chuanjiong :) )";
    jbufwriter_write(ctx->wt, (uint8_t *)my_name, strlen(my_name));
    W8(0);

    int cur = jbufwriter_tell(ctx->wt);
    jbufwriter_seek(ctx->wt, slot, SEEK_SET);
    WB32(cur-slot);
    jbufwriter_seek(ctx->wt, 0, SEEK_END);

    return SUCCESS;
}

static int jmm_mp4_muxer_mhd(jmm_mp4_muxer_ctx *ctx, jmm_mp4_muxer_entry *entry)
{
    if ((ctx==NULL) || (entry==NULL))
        return ERROR_FAIL;

    if (entry == ctx->aentry)
    {
        //smhd
        //size
        WB32(0x10);
        //fourcc
        write_fourcc('s','m','h','d');
        //full box
        WB32(0);
        WB16(0);
        WB16(0);
    }
    else
    {
        //vmhd
        //size
        WB32(0x14);
        //fourcc
        write_fourcc('v','m','h','d');
        //full box
        WB32(1);
        WB16(0);
        WB16(0);
        WB16(0);
        WB16(0);
    }

    return SUCCESS;
}

static int jmm_mp4_muxer_ase(jmm_mp4_muxer_ctx *ctx, jmm_mp4_muxer_entry *entry)
{
    if ((ctx==NULL) || (entry==NULL))
        return ERROR_FAIL;

    //mp4a box
    WB32(0x4b);
    write_fourcc('m','p','4','a');
    jbufwriter_dump(ctx->wt, 6, 0);
    WB16(1);

    jbufwriter_dump(ctx->wt, 8, 0);
    //channelcount
    WB16(ctx->ainfo.channels);
    //samplesize
    WB16(16);
    WB32(0);
    //samplerate << 16
    WB32(ctx->ainfo.samplerate<<16);

    //esds box
    WB32(0x27);
    write_fourcc('e','s','d','s');
    //full box
    WB32(0);

    //FIXME
    uint8_t buf[32] = {3, 0x19, 0, 2, 0, 4, 0x11, 0x40, 0x15, 0};
    jbufwriter_write(ctx->wt, buf, 20);
    W8(0x05);
    W8(ctx->asc->size); //assume size < 128
    jbufwriter_write(ctx->wt, ctx->asc->data, ctx->asc->size);

    WB24(0x060102);

    return SUCCESS;
}

static int jmm_mp4_muxer_vse(jmm_mp4_muxer_ctx *ctx, jmm_mp4_muxer_entry *entry)
{
    if ((ctx==NULL) || (entry==NULL))
        return ERROR_FAIL;

    //size
    int slot = jbufwriter_tell(ctx->wt);
    WB32(0);
    //fourcc
    write_fourcc('a','v','c','1');

    jbufwriter_dump(ctx->wt, 6, 0);
    WB16(1);

    jbufwriter_dump(ctx->wt, 16, 0);

    //width & height
    WB16(ctx->vinfo.width);
    WB16(ctx->vinfo.height);

    WB32(0x00480000);
    WB32(0x00480000);

    WB32(0);
    //frame_count
    WB16(1);

    //compressorname
    jbufwriter_dump(ctx->wt, 32, 0);

    WB16(0x18);
    //pre_defined = -1
    WB16(0xffff);

    //avcC
    //size
    WB32(ctx->avcc->size+8);
    write_fourcc('a','v','c','C');
    jbufwriter_write(ctx->wt, ctx->avcc->data, ctx->avcc->size);

    int cur = jbufwriter_tell(ctx->wt);
    jbufwriter_seek(ctx->wt, slot, SEEK_SET);
    WB32(cur-slot);
    jbufwriter_seek(ctx->wt, 0, SEEK_END);

    return SUCCESS;
}

static int jmm_mp4_muxer_stsd(jmm_mp4_muxer_ctx *ctx, jmm_mp4_muxer_entry *entry)
{
    if ((ctx==NULL) || (entry==NULL))
        return ERROR_FAIL;

    //size
    int slot = jbufwriter_tell(ctx->wt);
    WB32(0);
    //fourcc
    write_fourcc('s','t','s','d');
    //full box
    WB32(0);

    //entry_count
    WB32(1);

    if (entry == ctx->aentry)
    {
        //AudioSampleEntry
        jmm_mp4_muxer_ase(ctx, entry);
    }
    else
    {
        //VisualSampleEntry
        jmm_mp4_muxer_vse(ctx, entry);
    }

    int cur = jbufwriter_tell(ctx->wt);
    jbufwriter_seek(ctx->wt, slot, SEEK_SET);
    WB32(cur-slot);
    jbufwriter_seek(ctx->wt, 0, SEEK_END);

    return SUCCESS;
}

static int jmm_mp4_muxer_stts(jmm_mp4_muxer_ctx *ctx, jmm_mp4_muxer_entry *entry)
{
    if ((ctx==NULL) || (entry==NULL))
        return ERROR_FAIL;

    int i, count, timebase;

    //size
    int slot = jbufwriter_tell(ctx->wt);
    WB32(0);
    //fourcc
    write_fourcc('s','t','t','s');
    //full box
    WB32(0);

    //entry_count
    if (entry == ctx->aentry)
    {
        count = ctx->aidx+1;
        timebase = ctx->ainfo.samplerate;
    }
    else
    {
        count = ctx->vidx+1;
        timebase = 90000;
    }
    WB32(count);

    for (i=0; i<count; i++)
    {
        //sample_count
        WB32(1);
        //sample_delta
        WB32(entry[i].duration*timebase/1000000);
    }

    int cur = jbufwriter_tell(ctx->wt);
    jbufwriter_seek(ctx->wt, slot, SEEK_SET);
    WB32(cur-slot);
    jbufwriter_seek(ctx->wt, 0, SEEK_END);

    return SUCCESS;
}

static int jmm_mp4_muxer_stsz(jmm_mp4_muxer_ctx *ctx, jmm_mp4_muxer_entry *entry)
{
    if ((ctx==NULL) || (entry==NULL))
        return ERROR_FAIL;

    int i, count;

    //size
    int slot = jbufwriter_tell(ctx->wt);
    WB32(0);
    //fourcc
    write_fourcc('s','t','s','z');
    //full box
    WB32(0);

    //sample_size
    WB32(0);
    //sample_count
    if (entry == ctx->aentry)
        count = ctx->aidx+1;
    else
        count = ctx->vidx+1;
    WB32(count);

    for (i=0; i<count; i++)
        WB32(entry[i].size);

    int cur = jbufwriter_tell(ctx->wt);
    jbufwriter_seek(ctx->wt, slot, SEEK_SET);
    WB32(cur-slot);
    jbufwriter_seek(ctx->wt, 0, SEEK_END);

    return SUCCESS;
}

static int jmm_mp4_muxer_stsc(jmm_mp4_muxer_ctx *ctx, jmm_mp4_muxer_entry *entry)
{
    if ((ctx==NULL) || (entry==NULL))
        return ERROR_FAIL;

    //size
    int slot = jbufwriter_tell(ctx->wt);
    WB32(0);
    //fourcc
    write_fourcc('s','t','s','c');
    //full box
    WB32(0);

    //entry_count
    WB32(1);
    //first_chunk
    WB32(1);
    //samples_per_chunk
    WB32(1);
    //sample_description_index
    WB32(1);

    int cur = jbufwriter_tell(ctx->wt);
    jbufwriter_seek(ctx->wt, slot, SEEK_SET);
    WB32(cur-slot);
    jbufwriter_seek(ctx->wt, 0, SEEK_END);

    return SUCCESS;
}

#if 0
static int jmm_mp4_muxer_stco(jmm_mp4_muxer_ctx *ctx, jmm_mp4_muxer_entry *entry)
{
    if ((ctx==NULL) || (entry==NULL))
        return ERROR_FAIL;

    int i, count;

    //size
    int slot = jbufwriter_tell(ctx->wt);
    WB32(0);
    //fourcc
    write_fourcc('s','t','c','o');
    //full box
    WB32(0);

    //entry_count
    if (entry == ctx->aentry)
        count = ctx->aidx+1;
    else
        count = ctx->vidx+1;
    WB32(count);

    for (i=0; i<count; i++)
        WB32(entry[i].offset & 0xffffffff);

    int cur = jbufwriter_tell(ctx->wt);
    jbufwriter_seek(ctx->wt, slot, SEEK_SET);
    WB32(cur-slot);
    jbufwriter_seek(ctx->wt, 0, SEEK_END);

    return SUCCESS;
}
#endif

static int jmm_mp4_muxer_co64(jmm_mp4_muxer_ctx *ctx, jmm_mp4_muxer_entry *entry)
{
    if ((ctx==NULL) || (entry==NULL))
        return ERROR_FAIL;

    int i, count;

    //size
    int slot = jbufwriter_tell(ctx->wt);
    WB32(0);
    //fourcc
    write_fourcc('c','o','6','4');
    //full box
    WB32(0);

    //entry_count
    if (entry == ctx->aentry)
        count = ctx->aidx+1;
    else
        count = ctx->vidx+1;
    WB32(count);

    for (i=0; i<count; i++)
    {
        WB32(entry[i].offset>>32);
        WB32(entry[i].offset&0xffffffff);
    }

    int cur = jbufwriter_tell(ctx->wt);
    jbufwriter_seek(ctx->wt, slot, SEEK_SET);
    WB32(cur-slot);
    jbufwriter_seek(ctx->wt, 0, SEEK_END);

    return SUCCESS;
}

static int jmm_mp4_muxer_ctts(jmm_mp4_muxer_ctx *ctx, jmm_mp4_muxer_entry *entry)
{
    if ((ctx==NULL) || (entry==NULL))
        return ERROR_FAIL;

    if (entry == ctx->aentry)
        return SUCCESS;

    //size
    int slot = jbufwriter_tell(ctx->wt);
    WB32(0);
    //fourcc
    write_fourcc('c','t','t','s');
    //full box
    WB32(0);

    //entry_count
    WB32(ctx->vidx+1);

    int i;
    for (i=0; i<=ctx->vidx; i++)
    {
        //sample_count
        WB32(1);
        //sample_offset
        WB32((entry[i].pts-entry[i].dts)*9/100);
    }

    int cur = jbufwriter_tell(ctx->wt);
    jbufwriter_seek(ctx->wt, slot, SEEK_SET);
    WB32(cur-slot);
    jbufwriter_seek(ctx->wt, 0, SEEK_END);

    return SUCCESS;
}

static int jmm_mp4_muxer_stss(jmm_mp4_muxer_ctx *ctx, jmm_mp4_muxer_entry *entry)
{
    if ((ctx==NULL) || (entry==NULL))
        return ERROR_FAIL;

    if (entry == ctx->aentry)
        return SUCCESS;

    //size
    int slot = jbufwriter_tell(ctx->wt);
    WB32(0);
    //fourcc
    write_fourcc('s','t','s','s');
    //full box
    WB32(0);

    //entry_count
    int slot1 = jbufwriter_tell(ctx->wt);
    WB32(0);

    int i, count;
    for (i=0,count=0; i<=ctx->vidx; i++)
        if (entry[i].key)
        {
            WB32(i+1);
            count++;
        }

    jbufwriter_seek(ctx->wt, slot1, SEEK_SET);
    WB32(count);
    jbufwriter_seek(ctx->wt, 0, SEEK_END);

    int cur = jbufwriter_tell(ctx->wt);
    jbufwriter_seek(ctx->wt, slot, SEEK_SET);
    WB32(cur-slot);
    jbufwriter_seek(ctx->wt, 0, SEEK_END);

    return SUCCESS;
}

static int jmm_mp4_muxer_stbl(jmm_mp4_muxer_ctx *ctx, jmm_mp4_muxer_entry *entry)
{
    if ((ctx==NULL) || (entry==NULL))
        return ERROR_FAIL;

    //size
    int slot = jbufwriter_tell(ctx->wt);
    WB32(0);
    //fourcc
    write_fourcc('s','t','b','l');

    //stsd
    jmm_mp4_muxer_stsd(ctx, entry);

    //stts
    jmm_mp4_muxer_stts(ctx, entry);

    //stsz
    jmm_mp4_muxer_stsz(ctx, entry);

    //stsc
    jmm_mp4_muxer_stsc(ctx, entry);

    //stco
    //jmm_mp4_muxer_stco(ctx, entry);
    //co64
    jmm_mp4_muxer_co64(ctx, entry);

    //ctts
    jmm_mp4_muxer_ctts(ctx, entry);

    //stss
    jmm_mp4_muxer_stss(ctx, entry);

    int cur = jbufwriter_tell(ctx->wt);
    jbufwriter_seek(ctx->wt, slot, SEEK_SET);
    WB32(cur-slot);
    jbufwriter_seek(ctx->wt, 0, SEEK_END);

    return SUCCESS;
}

static int jmm_mp4_muxer_dinf(jmm_mp4_muxer_ctx *ctx, jmm_mp4_muxer_entry *entry)
{
    if ((ctx==NULL) || (entry==NULL))
        return ERROR_FAIL;

    //dinf
    //size
    WB32(0x24);
    //fourccc
    write_fourcc('d','i','n','f');

    //dref
    WB32(0x1c);
    write_fourcc('d','r','e','f');
    //full box
    WB32(0);
    //entry_count
    WB32(1);

    //url
    WB32(0xc);
    write_fourcc('u','r','l',' ');
    //full box
    WB32(1);

    return SUCCESS;
}

static int jmm_mp4_muxer_minf(jmm_mp4_muxer_ctx *ctx, jmm_mp4_muxer_entry *entry)
{
    if ((ctx==NULL) || (entry==NULL))
        return ERROR_FAIL;

    //size
    int slot = jbufwriter_tell(ctx->wt);
    WB32(0);
    //fourcc
    write_fourcc('m','i','n','f');

    //mhd
    jmm_mp4_muxer_mhd(ctx, entry);

    //stbl
    jmm_mp4_muxer_stbl(ctx, entry);

    //dinf
    jmm_mp4_muxer_dinf(ctx, entry);

    int cur = jbufwriter_tell(ctx->wt);
    jbufwriter_seek(ctx->wt, slot, SEEK_SET);
    WB32(cur-slot);
    jbufwriter_seek(ctx->wt, 0, SEEK_END);

    return SUCCESS;
}

static int jmm_mp4_muxer_mdia(jmm_mp4_muxer_ctx *ctx, jmm_mp4_muxer_entry *entry)
{
    if ((ctx==NULL) || (entry==NULL))
        return ERROR_FAIL;

    //size
    int slot = jbufwriter_tell(ctx->wt);
    WB32(0);
    //fourcc
    write_fourcc('m','d','i','a');

    //mdhd
    jmm_mp4_muxer_mdhd(ctx, entry);

    //hdlr
    jmm_mp4_muxer_hdlr(ctx, entry);

    //minf
    jmm_mp4_muxer_minf(ctx, entry);

    int cur = jbufwriter_tell(ctx->wt);
    jbufwriter_seek(ctx->wt, slot, SEEK_SET);
    WB32(cur-slot);
    jbufwriter_seek(ctx->wt, 0, SEEK_END);

    return SUCCESS;
}

static int jmm_mp4_muxer_trak(jmm_mp4_muxer_ctx *ctx, jmm_mp4_muxer_entry *entry)
{
    if ((ctx==NULL) || (entry==NULL))
        return ERROR_FAIL;

    //size
    int slot = jbufwriter_tell(ctx->wt);
    WB32(0);
    //fourcc
    write_fourcc('t','r','a','k');

    //tkhd
    jmm_mp4_muxer_tkhd(ctx, entry);

    //mdia
    jmm_mp4_muxer_mdia(ctx, entry);

    int cur = jbufwriter_tell(ctx->wt);
    jbufwriter_seek(ctx->wt, slot, SEEK_SET);
    WB32(cur-slot);
    jbufwriter_seek(ctx->wt, 0, SEEK_END);

    return SUCCESS;
}

static void jmm_mp4_muxer_close(jhandle h)
{
    if (h == NULL)
        return;

    jmm_mp4_muxer_ctx *ctx = (jmm_mp4_muxer_ctx *)h;

    int i;
    int64_t cur;

    //mdat size
    cur = jbufwriter_tell(ctx->wt);
    jbufwriter_seek(ctx->wt, ctx->mdat_size_slot, SEEK_SET);
    WB32((cur-ctx->mdat_size_slot+8)>>32);
    WB32((cur-ctx->mdat_size_slot+8)&0xffffffff);
    jbufwriter_seek(ctx->wt, 0, SEEK_END);

    if ((ctx->aidx<2) && (ctx->vidx<2))
        goto clean;

    //last a&v entry duration
    if (ctx->aidx > 1)
    {
        ctx->aidx--;
        ctx->aentry[ctx->aidx].duration = ctx->aentry[ctx->aidx-1].duration;
        //calc a stream duration
        for (i=0; i<=ctx->aidx; i++)
            ctx->adur += ctx->aentry[i].duration;
        ctx->adur /= 1000000;
    }
    if (ctx->vidx > 1)
    {
        ctx->vidx--;
        ctx->ventry[ctx->vidx].duration = ctx->ventry[ctx->vidx-1].duration;
        //calc v stream duration
        for (i=0; i<=ctx->vidx; i++)
            ctx->vdur += ctx->ventry[i].duration;
        ctx->vdur /= 1000000;
    }

    //parse asc
    if (ctx->asc)
        jmm_aac_asc_parse(ctx->asc, &(ctx->ainfo));

    //parse avcc
    if (ctx->avcc)
        jmm_avc_avcc_parse(ctx->avcc, &(ctx->vinfo));

    //moov
    //size slot
    int64_t slot = jbufwriter_tell(ctx->wt);
    WB32(0);
    write_fourcc('m','o','o','v');

    jmm_mp4_muxer_mvhd(ctx);

    if (ctx->vidx > 0)
        jmm_mp4_muxer_trak(ctx, ctx->ventry);

    if (ctx->aidx > 0)
        jmm_mp4_muxer_trak(ctx, ctx->aentry);

    //moov size
    cur = jbufwriter_tell(ctx->wt);
    jbufwriter_seek(ctx->wt, slot, SEEK_SET);
    WB32(cur-slot);

clean:
    //free resource
    jdynarray_free(ctx->ventry);
    jdynarray_free(ctx->aentry);

    if (ctx->avcc)
        jmm_packet_free(ctx->avcc);
    if (ctx->asc)
        jmm_packet_free(ctx->asc);

    jbufwriter_close(ctx->wt);

    jfree(ctx);
}

static int jmm_mp4_muxer_write(jhandle h, jmm_packet *packet)
{
    if ((h==NULL) || (packet==NULL))
        return ERROR_FAIL;

    jmm_mp4_muxer_ctx *ctx = (jmm_mp4_muxer_ctx *)h;

    //wait asc
    if (!ctx->asc)
        if ((packet->type==JMM_CODEC_TYPE_AAC) && (packet->fmt!=JMM_BS_FMT_AAC_ASC))
        {
            jwarn("[jmm_mp4_muxer] drop aac pkt: %d\n", packet->fmt);
            return ERROR_FAIL;
        }

    //wait avcc
    if (!ctx->avcc)
        if ((packet->type==JMM_CODEC_TYPE_AVC) && (packet->fmt!=JMM_BS_FMT_AVC_AVCC))
        {
            jwarn("[jmm_mp4_muxer] drop avc pkt: %d\n", packet->fmt);
            return ERROR_FAIL;
        }

    jbool copy = jfalse;
    jmm_packet *pkt = packet;
    if ((packet->type==JMM_CODEC_TYPE_AAC) && (packet->fmt==JMM_BS_FMT_AAC_ADTS))
    {
        pkt = jmm_aac_adts2es(packet, jtrue);
        copy = jtrue;
    }
    else if ((packet->type==JMM_CODEC_TYPE_AVC) && (packet->fmt==JMM_BS_FMT_AVC_ANNEXB))
    {
        pkt = jmm_avc_annexb2mp4(packet, jtrue);
        copy = jtrue;
    }

    if (pkt == NULL)
        return ERROR_FAIL;

    if ((pkt->type==JMM_CODEC_TYPE_AAC) && (pkt->fmt==JMM_BS_FMT_AAC_ASC))
    {
        if (ctx->asc)
            jmm_packet_free(ctx->asc);
        ctx->asc = jmm_packet_clone(pkt);
        return SUCCESS;
    }
    else if ((pkt->type==JMM_CODEC_TYPE_AVC) && (pkt->fmt==JMM_BS_FMT_AVC_AVCC))
    {
        if (ctx->avcc)
            jmm_packet_free(ctx->avcc);
        ctx->avcc = jmm_packet_clone(pkt);
        return SUCCESS;
    }

    if (pkt->type == JMM_CODEC_TYPE_AAC)
    {
        //a entry
        jdynarray_index(ctx->aentry, ctx->aidx);
        ctx->aentry[ctx->aidx].key = pkt->key;
        ctx->aentry[ctx->aidx].dts = pkt->dts;
        ctx->aentry[ctx->aidx].pts = pkt->pts;
        ctx->aentry[ctx->aidx].size = pkt->size;
        ctx->aentry[ctx->aidx].offset = jbufwriter_tell(ctx->wt);

        if (ctx->aidx != 0)
        {
            //prev sample's duration
            ctx->aentry[ctx->aidx-1].duration = ctx->aentry[ctx->aidx].dts - ctx->aentry[ctx->aidx-1].dts;
        }

        jbufwriter_write(ctx->wt, pkt->data, pkt->size);

        ctx->aidx++;
    }
    else if (pkt->type == JMM_CODEC_TYPE_AVC)
    {
        //v entry
        jdynarray_index(ctx->ventry, ctx->vidx);
        ctx->ventry[ctx->vidx].key = pkt->key;
        ctx->ventry[ctx->vidx].dts = pkt->dts;
        ctx->ventry[ctx->vidx].pts = pkt->pts;
        ctx->ventry[ctx->vidx].size = pkt->size;
        ctx->ventry[ctx->vidx].offset = jbufwriter_tell(ctx->wt);

        if (ctx->vidx != 0)
        {
            //prev sample's duration
            ctx->ventry[ctx->vidx-1].duration = ctx->ventry[ctx->vidx].dts - ctx->ventry[ctx->vidx-1].dts;
        }

        jbufwriter_write(ctx->wt, pkt->data, pkt->size);

        ctx->vidx++;
    }

    if (copy)
        jmm_packet_free(pkt);

    return SUCCESS;
}

const jmm_muxer jmm_mp4_muxer = {
    jmm_mp4_muxer_open,
    jmm_mp4_muxer_close,
    jmm_mp4_muxer_write
};


