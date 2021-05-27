/*
 * AU muxer and demuxer
 * Copyright (c) 2001 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/*
 * First version by Francois Revol revol@free.fr
 *
 * Reference documents:
 * http://www.opengroup.org/public/pubs/external/auformat.html
 * http://www.goice.co.jp/member/mo/formats/au.html
 */

#include "avformat.h"
#include "raw.h"
#include "riff.h"

/* if we don't know the size in advance */
#define AU_UNKNOWN_SIZE ((uint32_t)(~0))

/* The ffmpeg codecs we support, and the IDs they have in the file */
static const AVCodecTag codec_au_tags[] = {
    { CODEC_ID_PCM_MULAW, 1 },
    { CODEC_ID_PCM_S8, 2 },
    { CODEC_ID_PCM_S16BE, 3 },
    { CODEC_ID_PCM_S24BE, 4 },
    { CODEC_ID_PCM_S32BE, 5 },
    { CODEC_ID_PCM_F32BE, 6 },
    { CODEC_ID_PCM_F64BE, 7 },
    { CODEC_ID_PCM_ALAW, 27 },
    { CODEC_ID_NONE, 0 },
};

#if CONFIG_AU_MUXER
/* AUDIO_FILE header */
static int put_au_header(ByteIOContext *pb, AVCodecContext *enc)
{
    if(!enc->codec_tag)
        return -1;
    put_tag(pb, ".snd");       /* magic number */
    put_be32(pb, 24);           /* header size */
    put_be32(pb, AU_UNKNOWN_SIZE); /* data size */
    put_be32(pb, (uint32_t)enc->codec_tag);     /* codec ID */
    put_be32(pb, enc->sample_rate);
    put_be32(pb, (uint32_t)enc->channels);
    return 0;
}

static int au_write_header(AVFormatContext *s)
{
    ByteIOContext *pb = s->pb;

    s->priv_data = NULL;

    /* format header */
    if (put_au_header(pb, s->streams[0]->codec) < 0) {
        return -1;
    }

    put_flush_packet(pb);

    return 0;
}

static int au_write_packet(AVFormatContext *s, AVPacket *pkt)
{
    ByteIOContext *pb = s->pb;
    put_buffer(pb, pkt->data, pkt->size);
    return 0;
}

static int au_write_trailer(AVFormatContext *s)
{
    ByteIOContext *pb = s->pb;
    int64_t file_size;

    if (!url_is_streamed(s->pb)) {

        /* update file size */
        file_size = url_ftell(pb);
        url_fseek(pb, 8, SEEK_SET);
        put_be32(pb, (uint32_t)(file_size - 24));
        url_fseek(pb, file_size, SEEK_SET);

        put_flush_packet(pb);
    }

    return 0;
}
#endif /* CONFIG_AU_MUXER */

static int au_probe(AVProbeData *p)
{
    /* check file header */
    if (p->buf[0] == '.' && p->buf[1] == 's' &&
        p->buf[2] == 'n' && p->buf[3] == 'd')
        return AVPROBE_SCORE_MAX;
    else
        return 0;
}

/* au input */
static int au_read_header(AVFormatContext *s,
                          AVFormatParameters *ap)
{
    int size;
    unsigned int tag;
    ByteIOContext *pb = s->pb;
    unsigned int id, channels, rate;
    enum CodecID codec;
    AVStream *st;

    /* check ".snd" header */
    tag = get_le32(pb);
    if (tag != MKTAG('.', 's', 'n', 'd'))
        return -1;
    size = get_be32(pb); /* header size */
    get_be32(pb); /* data size */

    id = get_be32(pb);
    rate = get_be32(pb);
    channels = get_be32(pb);

    codec = ff_codec_get_id(codec_au_tags, id);

    if (size >= 24) {
        /* skip unused data */
        url_fseek(pb, size - 24, SEEK_CUR);
    }

    /* now we are ready: build format streams */
    st = av_new_stream(s, 0);
    if (!st)
        return -1;
    st->codec->codec_type = AVMEDIA_TYPE_AUDIO;
    st->codec->codec_tag = id;
    st->codec->codec_id = codec;
    st->codec->channels = channels;
    st->codec->sample_rate = rate;
    av_set_pts_info(st, 64, 1, rate);
    return 0;
}

#define BLOCK_SIZE 1024

static int au_read_packet(AVFormatContext *s,
                          AVPacket *pkt)
{
    int ret;

    ret= av_get_packet(s->pb, pkt, BLOCK_SIZE *
                       s->streams[0]->codec->channels *
                       av_get_bits_per_sample(s->streams[0]->codec->codec_id) >> 3);
    if (ret < 0)
        return ret;
    pkt->stream_index = 0;

    /* note: we need to modify the packet size here to handle the last
       packet */
    pkt->size = ret;
    return 0;
}

#if CONFIG_AU_DEMUXER
AVInputFormat au_demuxer = {
    "au",
    NULL_IF_CONFIG_SMALL("SUN AU format"),
    0,
    au_probe,
    au_read_header,
    au_read_packet,
    NULL,
    pcm_read_seek,
    .codec_tag= (const AVCodecTag* const []){codec_au_tags, 0},
};
#endif

#if CONFIG_AU_MUXER
AVOutputFormat au_muxer = {
    "au",
    NULL_IF_CONFIG_SMALL("SUN AU format"),
    "audio/basic",
    "au",
    0,
    CODEC_ID_PCM_S16BE,
    CODEC_ID_NONE,
    au_write_header,
    au_write_packet,
    au_write_trailer,
    .codec_tag= (const AVCodecTag* const []){codec_au_tags, 0},
};
#endif //CONFIG_AU_MUXER
