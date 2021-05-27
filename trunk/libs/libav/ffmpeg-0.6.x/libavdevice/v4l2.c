/*
 * Video4Linux2 grab interface
 * Copyright (c) 2000,2001 Fabrice Bellard
 * Copyright (c) 2006 Luca Abeni
 *
 * Part of this file is based on the V4L2 video capture example
 * (http://v4l2spec.bytesex.org/v4l2spec/capture.c)
 *
 * Thanks to Michael Niedermayer for providing the mapping between
 * V4L2_PIX_FMT_* and PIX_FMT_*
 *
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

#undef __STRICT_ANSI__ //workaround due to broken kernel headers
#include "config.h"
#include "libavformat/avformat.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#if HAVE_SYS_VIDEOIO_H
#include <sys/videoio.h>
#else
#include <asm/types.h>
#include <linux/videodev2.h>
#endif
#include <time.h>
#include <strings.h>

static const int desired_video_buffers = 256;

enum io_method {
    io_read,
    io_mmap,
    io_userptr
};

struct video_data {
    int fd;
    int frame_format; /* V4L2_PIX_FMT_* */
    enum io_method io_method;
    int width, height;
    int frame_size;
    int top_field_first;

    int buffers;
    void **buf_start;
    unsigned int *buf_len;
};

struct buff_data {
    int index;
    int fd;
};

struct fmt_map {
    enum PixelFormat ff_fmt;
    enum CodecID codec_id;
    uint32_t v4l2_fmt;
};

static struct fmt_map fmt_conversion_table[] = {
    {
        .ff_fmt = PIX_FMT_YUV420P,
        .codec_id = CODEC_ID_RAWVIDEO,
        .v4l2_fmt = V4L2_PIX_FMT_YUV420,
    },
    {
        .ff_fmt = PIX_FMT_YUV422P,
        .codec_id = CODEC_ID_RAWVIDEO,
        .v4l2_fmt = V4L2_PIX_FMT_YUV422P,
    },
    {
        .ff_fmt = PIX_FMT_YUYV422,
        .codec_id = CODEC_ID_RAWVIDEO,
        .v4l2_fmt = V4L2_PIX_FMT_YUYV,
    },
    {
        .ff_fmt = PIX_FMT_UYVY422,
        .codec_id = CODEC_ID_RAWVIDEO,
        .v4l2_fmt = V4L2_PIX_FMT_UYVY,
    },
    {
        .ff_fmt = PIX_FMT_YUV411P,
        .codec_id = CODEC_ID_RAWVIDEO,
        .v4l2_fmt = V4L2_PIX_FMT_YUV411P,
    },
    {
        .ff_fmt = PIX_FMT_YUV410P,
        .codec_id = CODEC_ID_RAWVIDEO,
        .v4l2_fmt = V4L2_PIX_FMT_YUV410,
    },
    {
        .ff_fmt = PIX_FMT_RGB555,
        .codec_id = CODEC_ID_RAWVIDEO,
        .v4l2_fmt = V4L2_PIX_FMT_RGB555,
    },
    {
        .ff_fmt = PIX_FMT_RGB565,
        .codec_id = CODEC_ID_RAWVIDEO,
        .v4l2_fmt = V4L2_PIX_FMT_RGB565,
    },
    {
        .ff_fmt = PIX_FMT_BGR24,
        .codec_id = CODEC_ID_RAWVIDEO,
        .v4l2_fmt = V4L2_PIX_FMT_BGR24,
    },
    {
        .ff_fmt = PIX_FMT_RGB24,
        .codec_id = CODEC_ID_RAWVIDEO,
        .v4l2_fmt = V4L2_PIX_FMT_RGB24,
    },
    {
        .ff_fmt = PIX_FMT_BGRA,
        .codec_id = CODEC_ID_RAWVIDEO,
        .v4l2_fmt = V4L2_PIX_FMT_BGR32,
    },
    {
        .ff_fmt = PIX_FMT_GRAY8,
        .codec_id = CODEC_ID_RAWVIDEO,
        .v4l2_fmt = V4L2_PIX_FMT_GREY,
    },
    {
        .ff_fmt = PIX_FMT_NONE,
        .codec_id = CODEC_ID_MJPEG,
        .v4l2_fmt = V4L2_PIX_FMT_MJPEG,
    },
    {
        .ff_fmt = PIX_FMT_NONE,
        .codec_id = CODEC_ID_MJPEG,
        .v4l2_fmt = V4L2_PIX_FMT_JPEG,
    },
};

static int device_open(AVFormatContext *ctx, uint32_t *capabilities)
{
    struct v4l2_capability cap;
    int fd;
    int res, err;
    int flags = O_RDWR;

    if (ctx->flags & AVFMT_FLAG_NONBLOCK) {
        flags |= O_NONBLOCK;
    }
    fd = open(ctx->filename, flags, 0);
    if (fd < 0) {
        av_log(ctx, AV_LOG_ERROR, "Cannot open video device %s : %s\n",
                 ctx->filename, strerror(errno));

        return AVERROR(errno);
    }

    res = ioctl(fd, VIDIOC_QUERYCAP, &cap);
    // ENOIOCTLCMD definition only availble on __KERNEL__
    if (res < 0 && ((err = errno) == 515)) {
        av_log(ctx, AV_LOG_ERROR, "QUERYCAP not implemented, probably V4L device but not supporting V4L2\n");
        close(fd);

        return AVERROR(515);
    }
    if (res < 0) {
        av_log(ctx, AV_LOG_ERROR, "ioctl(VIDIOC_QUERYCAP): %s\n",
                 strerror(errno));
        close(fd);

        return AVERROR(err);
    }
    if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0) {
        av_log(ctx, AV_LOG_ERROR, "Not a video capture device\n");
        close(fd);

        return AVERROR(ENODEV);
    }
    *capabilities = cap.capabilities;

    return fd;
}

static int device_init(AVFormatContext *ctx, int *width, int *height, uint32_t pix_fmt)
{
    struct video_data *s = ctx->priv_data;
    int fd = s->fd;
    struct v4l2_format fmt;
    int res;

    memset(&fmt, 0, sizeof(struct v4l2_format));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = *width;
    fmt.fmt.pix.height = *height;
    fmt.fmt.pix.pixelformat = pix_fmt;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    res = ioctl(fd, VIDIOC_S_FMT, &fmt);
    if ((*width != fmt.fmt.pix.width) || (*height != fmt.fmt.pix.height)) {
        av_log(ctx, AV_LOG_INFO, "The V4L2 driver changed the video from %dx%d to %dx%d\n", *width, *height, fmt.fmt.pix.width, fmt.fmt.pix.height);
        *width = fmt.fmt.pix.width;
        *height = fmt.fmt.pix.height;
    }

    if (pix_fmt != fmt.fmt.pix.pixelformat) {
        av_log(ctx, AV_LOG_DEBUG, "The V4L2 driver changed the pixel format from 0x%08X to 0x%08X\n", pix_fmt, fmt.fmt.pix.pixelformat);
        res = -1;
    }

    return res;
}

static int first_field(int fd)
{
    int res;
    v4l2_std_id std;

    res = ioctl(fd, VIDIOC_G_STD, &std);
    if (res < 0) {
        return 0;
    }
    if (std & V4L2_STD_NTSC) {
        return 0;
    }

    return 1;
}

static uint32_t fmt_ff2v4l(enum PixelFormat pix_fmt, enum CodecID codec_id)
{
    int i;

    for (i = 0; i < FF_ARRAY_ELEMS(fmt_conversion_table); i++) {
        if ((codec_id == CODEC_ID_NONE ||
             fmt_conversion_table[i].codec_id == codec_id) &&
            (pix_fmt == PIX_FMT_NONE ||
             fmt_conversion_table[i].ff_fmt == pix_fmt)) {
            return fmt_conversion_table[i].v4l2_fmt;
        }
    }

    return 0;
}

static enum PixelFormat fmt_v4l2ff(uint32_t v4l2_fmt, enum CodecID codec_id)
{
    int i;

    for (i = 0; i < FF_ARRAY_ELEMS(fmt_conversion_table); i++) {
        if (fmt_conversion_table[i].v4l2_fmt == v4l2_fmt &&
            fmt_conversion_table[i].codec_id == codec_id) {
            return fmt_conversion_table[i].ff_fmt;
        }
    }

    return PIX_FMT_NONE;
}

static enum CodecID fmt_v4l2codec(uint32_t v4l2_fmt)
{
    int i;

    for (i = 0; i < FF_ARRAY_ELEMS(fmt_conversion_table); i++) {
        if (fmt_conversion_table[i].v4l2_fmt == v4l2_fmt) {
            return fmt_conversion_table[i].codec_id;
        }
    }

    return CODEC_ID_NONE;
}

static int mmap_init(AVFormatContext *ctx)
{
    struct video_data *s = ctx->priv_data;
    struct v4l2_requestbuffers req;
    int i, res;

    memset(&req, 0, sizeof(struct v4l2_requestbuffers));
    req.count = desired_video_buffers;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    res = ioctl (s->fd, VIDIOC_REQBUFS, &req);
    if (res < 0) {
        if (errno == EINVAL) {
            av_log(ctx, AV_LOG_ERROR, "Device does not support mmap\n");
        } else {
            av_log(ctx, AV_LOG_ERROR, "ioctl(VIDIOC_REQBUFS)\n");
        }

        return AVERROR(errno);
    }

    if (req.count < 2) {
        av_log(ctx, AV_LOG_ERROR, "Insufficient buffer memory\n");

        return AVERROR(ENOMEM);
    }
    s->buffers = req.count;
    s->buf_start = av_malloc(sizeof(void *) * s->buffers);
    if (s->buf_start == NULL) {
        av_log(ctx, AV_LOG_ERROR, "Cannot allocate buffer pointers\n");

        return AVERROR(ENOMEM);
    }
    s->buf_len = av_malloc(sizeof(unsigned int) * s->buffers);
    if (s->buf_len == NULL) {
        av_log(ctx, AV_LOG_ERROR, "Cannot allocate buffer sizes\n");
        av_free(s->buf_start);

        return AVERROR(ENOMEM);
    }

    for (i = 0; i < req.count; i++) {
        struct v4l2_buffer buf;

        memset(&buf, 0, sizeof(struct v4l2_buffer));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        res = ioctl (s->fd, VIDIOC_QUERYBUF, &buf);
        if (res < 0) {
            av_log(ctx, AV_LOG_ERROR, "ioctl(VIDIOC_QUERYBUF)\n");

            return AVERROR(errno);
        }

        s->buf_len[i] = buf.length;
        if (s->frame_size > 0 && s->buf_len[i] < s->frame_size) {
            av_log(ctx, AV_LOG_ERROR, "Buffer len [%d] = %d != %d\n", i, s->buf_len[i], s->frame_size);

            return -1;
        }
        s->buf_start[i] = mmap (NULL, buf.length,
                        PROT_READ | PROT_WRITE, MAP_SHARED, s->fd, buf.m.offset);
        if (s->buf_start[i] == MAP_FAILED) {
            av_log(ctx, AV_LOG_ERROR, "mmap: %s\n", strerror(errno));

            return AVERROR(errno);
        }
    }

    return 0;
}

static int read_init(AVFormatContext *ctx)
{
    return -1;
}

static void mmap_release_buffer(AVPacket *pkt)
{
    struct v4l2_buffer buf;
    int res, fd;
    struct buff_data *buf_descriptor = pkt->priv;

    if (pkt->data == NULL) {
         return;
    }

    memset(&buf, 0, sizeof(struct v4l2_buffer));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = buf_descriptor->index;
    fd = buf_descriptor->fd;
    av_free(buf_descriptor);

    res = ioctl (fd, VIDIOC_QBUF, &buf);
    if (res < 0) {
        av_log(NULL, AV_LOG_ERROR, "ioctl(VIDIOC_QBUF)\n");
    }
    pkt->data = NULL;
    pkt->size = 0;
}

static int mmap_read_frame(AVFormatContext *ctx, AVPacket *pkt)
{
    struct video_data *s = ctx->priv_data;
    struct v4l2_buffer buf;
    struct buff_data *buf_descriptor;
    int res;

    memset(&buf, 0, sizeof(struct v4l2_buffer));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    /* FIXME: Some special treatment might be needed in case of loss of signal... */
    while ((res = ioctl(s->fd, VIDIOC_DQBUF, &buf)) < 0 && (errno == EINTR));
    if (res < 0) {
        if (errno == EAGAIN) {
            pkt->size = 0;

            return AVERROR(EAGAIN);
        }
        av_log(ctx, AV_LOG_ERROR, "ioctl(VIDIOC_DQBUF): %s\n", strerror(errno));

        return AVERROR(errno);
    }
    assert (buf.index < s->buffers);
    if (s->frame_size > 0 && buf.bytesused != s->frame_size) {
        av_log(ctx, AV_LOG_ERROR, "The v4l2 frame is %d bytes, but %d bytes are expected\n", buf.bytesused, s->frame_size);

        return AVERROR_INVALIDDATA;
    }

    /* Image is at s->buff_start[buf.index] */
    pkt->data= s->buf_start[buf.index];
    pkt->size = buf.bytesused;
    pkt->pts = buf.timestamp.tv_sec * INT64_C(1000000) + buf.timestamp.tv_usec;
    pkt->destruct = mmap_release_buffer;
    buf_descriptor = av_malloc(sizeof(struct buff_data));
    if (buf_descriptor == NULL) {
        /* Something went wrong... Since av_malloc() failed, we cannot even
         * allocate a buffer for memcopying into it
         */
        av_log(ctx, AV_LOG_ERROR, "Failed to allocate a buffer descriptor\n");
        res = ioctl (s->fd, VIDIOC_QBUF, &buf);

        return AVERROR(ENOMEM);
    }
    buf_descriptor->fd = s->fd;
    buf_descriptor->index = buf.index;
    pkt->priv = buf_descriptor;

    return s->buf_len[buf.index];
}

static int read_frame(AVFormatContext *ctx, AVPacket *pkt)
{
    return -1;
}

static int mmap_start(AVFormatContext *ctx)
{
    struct video_data *s = ctx->priv_data;
    enum v4l2_buf_type type;
    int i, res;

    for (i = 0; i < s->buffers; i++) {
        struct v4l2_buffer buf;

        memset(&buf, 0, sizeof(struct v4l2_buffer));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = i;

        res = ioctl (s->fd, VIDIOC_QBUF, &buf);
        if (res < 0) {
            av_log(ctx, AV_LOG_ERROR, "ioctl(VIDIOC_QBUF): %s\n", strerror(errno));

            return AVERROR(errno);
        }
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    res = ioctl (s->fd, VIDIOC_STREAMON, &type);
    if (res < 0) {
        av_log(ctx, AV_LOG_ERROR, "ioctl(VIDIOC_STREAMON): %s\n", strerror(errno));

        return AVERROR(errno);
    }

    return 0;
}

static void mmap_close(struct video_data *s)
{
    enum v4l2_buf_type type;
    int i;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    /* We do not check for the result, because we could
     * not do anything about it anyway...
     */
    ioctl(s->fd, VIDIOC_STREAMOFF, &type);
    for (i = 0; i < s->buffers; i++) {
        munmap(s->buf_start[i], s->buf_len[i]);
    }
    av_free(s->buf_start);
    av_free(s->buf_len);
}

static int v4l2_set_parameters( AVFormatContext *s1, AVFormatParameters *ap )
{
    struct video_data *s = s1->priv_data;
    struct v4l2_input input;
    struct v4l2_standard standard;
    int i;

    if(ap->channel>=0) {
        /* set tv video input */
        memset (&input, 0, sizeof (input));
        input.index = ap->channel;
        if(ioctl (s->fd, VIDIOC_ENUMINPUT, &input) < 0) {
            av_log(s1, AV_LOG_ERROR, "The V4L2 driver ioctl enum input failed:\n");
            return AVERROR(EIO);
        }

        av_log(s1, AV_LOG_DEBUG, "The V4L2 driver set input_id: %d, input: %s\n",
               ap->channel, input.name);
        if(ioctl (s->fd, VIDIOC_S_INPUT, &input.index) < 0 ) {
            av_log(s1, AV_LOG_ERROR, "The V4L2 driver ioctl set input(%d) failed\n",
                   ap->channel);
            return AVERROR(EIO);
        }
    }

    if(ap->standard) {
        av_log(s1, AV_LOG_DEBUG, "The V4L2 driver set standard: %s\n",
               ap->standard );
        /* set tv standard */
        memset (&standard, 0, sizeof (standard));
        for(i=0;;i++) {
            standard.index = i;
            if (ioctl(s->fd, VIDIOC_ENUMSTD, &standard) < 0) {
                av_log(s1, AV_LOG_ERROR, "The V4L2 driver ioctl set standard(%s) failed\n",
                       ap->standard);
                return AVERROR(EIO);
            }

            if(!strcasecmp(standard.name, ap->standard)) {
                break;
            }
        }

        av_log(s1, AV_LOG_DEBUG, "The V4L2 driver set standard: %s, id: %"PRIu64"\n",
               ap->standard, (uint64_t)standard.id);
        if (ioctl(s->fd, VIDIOC_S_STD, &standard.id) < 0) {
            av_log(s1, AV_LOG_ERROR, "The V4L2 driver ioctl set standard(%s) failed\n",
                   ap->standard);
            return AVERROR(EIO);
        }
    }

    return 0;
}

static uint32_t device_try_init(AVFormatContext *s1,
                                const AVFormatParameters *ap,
                                int *width,
                                int *height,
                                enum CodecID *codec_id)
{
    uint32_t desired_format = fmt_ff2v4l(ap->pix_fmt, s1->video_codec_id);

    if (desired_format == 0 ||
        device_init(s1, width, height, desired_format) < 0) {
        int i;

        desired_format = 0;
        for (i = 0; i<FF_ARRAY_ELEMS(fmt_conversion_table); i++) {
            if (s1->video_codec_id == CODEC_ID_NONE ||
                fmt_conversion_table[i].codec_id == s1->video_codec_id) {
                desired_format = fmt_conversion_table[i].v4l2_fmt;
                if (device_init(s1, width, height, desired_format) >= 0) {
                    break;
                }
                desired_format = 0;
            }
        }
    }
    if (desired_format != 0) {
        *codec_id = fmt_v4l2codec(desired_format);
        assert(*codec_id != CODEC_ID_NONE);
    }

    return desired_format;
}

static int v4l2_read_header(AVFormatContext *s1, AVFormatParameters *ap)
{
    struct video_data *s = s1->priv_data;
    AVStream *st;
    int res;
    uint32_t desired_format, capabilities;
    enum CodecID codec_id;

    st = av_new_stream(s1, 0);
    if (!st) {
        return AVERROR(ENOMEM);
    }
    av_set_pts_info(st, 64, 1, 1000000); /* 64 bits pts in us */

    s->width  = ap->width;
    s->height = ap->height;

    capabilities = 0;
    s->fd = device_open(s1, &capabilities);
    if (s->fd < 0) {
        return AVERROR(EIO);
    }
    av_log(s1, AV_LOG_VERBOSE, "[%d]Capabilities: %x\n", s->fd, capabilities);

    if (!s->width && !s->height) {
        struct v4l2_format fmt;

        av_log(s1, AV_LOG_VERBOSE, "Querying the device for the current frame size\n");
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(s->fd, VIDIOC_G_FMT, &fmt) < 0) {
            av_log(s1, AV_LOG_ERROR, "ioctl(VIDIOC_G_FMT): %s\n", strerror(errno));
            return AVERROR(errno);
        }
        s->width  = fmt.fmt.pix.width;
        s->height = fmt.fmt.pix.height;
        av_log(s1, AV_LOG_VERBOSE, "Setting frame size to %dx%d\n", s->width, s->height);
    }

    desired_format = device_try_init(s1, ap, &s->width, &s->height, &codec_id);
    if (desired_format == 0) {
        av_log(s1, AV_LOG_ERROR, "Cannot find a proper format for "
               "codec_id %d, pix_fmt %d.\n", s1->video_codec_id, ap->pix_fmt);
        close(s->fd);

        return AVERROR(EIO);
    }
    if (avcodec_check_dimensions(s1, s->width, s->height) < 0)
        return AVERROR(EINVAL);
    s->frame_format = desired_format;

    if( v4l2_set_parameters( s1, ap ) < 0 )
        return AVERROR(EIO);

    st->codec->pix_fmt = fmt_v4l2ff(desired_format, codec_id);
    s->frame_size = avpicture_get_size(st->codec->pix_fmt, s->width, s->height);
    if (capabilities & V4L2_CAP_STREAMING) {
        s->io_method = io_mmap;
        res = mmap_init(s1);
        if (res == 0) {
            res = mmap_start(s1);
        }
    } else {
        s->io_method = io_read;
        res = read_init(s1);
    }
    if (res < 0) {
        close(s->fd);

        return AVERROR(EIO);
    }
    s->top_field_first = first_field(s->fd);

    st->codec->codec_type = AVMEDIA_TYPE_VIDEO;
    st->codec->codec_id = codec_id;
    st->codec->width = s->width;
    st->codec->height = s->height;
    st->codec->time_base.den = ap->time_base.den;
    st->codec->time_base.num = ap->time_base.num;
    st->codec->bit_rate = s->frame_size * 1/av_q2d(st->codec->time_base) * 8;

    return 0;
}

static int v4l2_read_packet(AVFormatContext *s1, AVPacket *pkt)
{
    struct video_data *s = s1->priv_data;
    int res;

    if (s->io_method == io_mmap) {
        av_init_packet(pkt);
        res = mmap_read_frame(s1, pkt);
    } else if (s->io_method == io_read) {
        if (av_new_packet(pkt, s->frame_size) < 0)
            return AVERROR(EIO);

        res = read_frame(s1, pkt);
    } else {
        return AVERROR(EIO);
    }
    if (res < 0) {
        return res;
    }

    if (s1->streams[0]->codec->coded_frame) {
        s1->streams[0]->codec->coded_frame->interlaced_frame = 1;
        s1->streams[0]->codec->coded_frame->top_field_first = s->top_field_first;
    }

    return pkt->size;
}

static int v4l2_read_close(AVFormatContext *s1)
{
    struct video_data *s = s1->priv_data;

    if (s->io_method == io_mmap) {
        mmap_close(s);
    }

    close(s->fd);
    return 0;
}

AVInputFormat v4l2_demuxer = {
    "video4linux2",
    NULL_IF_CONFIG_SMALL("Video4Linux2 device grab"),
    sizeof(struct video_data),
    NULL,
    v4l2_read_header,
    v4l2_read_packet,
    v4l2_read_close,
    .flags = AVFMT_NOFILE,
};
