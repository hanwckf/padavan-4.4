/*
 * Interface to xvidcore for mpeg4 encoding
 * Copyright (c) 2004 Adam Thayer <krevnik@comcast.net>
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

/**
 * @file
 * Interface to xvidcore for MPEG-4 compliant encoding.
 * @author Adam Thayer (krevnik@comcast.net)
 */

#include <xvid.h>
#include <unistd.h>
#include "avcodec.h"
#include "libavutil/intreadwrite.h"
#include "libxvid_internal.h"

/**
 * Buffer management macros.
 */
#define BUFFER_SIZE                 1024
#define BUFFER_REMAINING(x)         (BUFFER_SIZE - strlen(x))
#define BUFFER_CAT(x)               (&((x)[strlen(x)]))

/* For PPC Use */
int has_altivec(void);

/**
 * Structure for the private Xvid context.
 * This stores all the private context for the codec.
 */
struct xvid_context {
    void *encoder_handle;          /** Handle for Xvid encoder */
    int xsize, ysize;              /** Frame size */
    int vop_flags;                 /** VOP flags for Xvid encoder */
    int vol_flags;                 /** VOL flags for Xvid encoder */
    int me_flags;                  /** Motion Estimation flags */
    int qscale;                    /** Do we use constant scale? */
    int quicktime_format;          /** Are we in a QT-based format? */
    AVFrame encoded_picture;       /** Encoded frame information */
    char *twopassbuffer;           /** Character buffer for two-pass */
    char *old_twopassbuffer;       /** Old character buffer (two-pass) */
    char *twopassfile;             /** second pass temp file name */
    unsigned char *intra_matrix;   /** P-Frame Quant Matrix */
    unsigned char *inter_matrix;   /** I-Frame Quant Matrix */
};

/**
 * Structure for the private first-pass plugin.
 */
struct xvid_ff_pass1 {
    int     version;                /** Xvid version */
    struct xvid_context *context;        /** Pointer to private context */
};

/* Prototypes - See function implementation for details */
int xvid_strip_vol_header(AVCodecContext *avctx, unsigned char *frame, unsigned int header_len, unsigned int frame_len);
int xvid_ff_2pass(void *ref, int opt, void *p1, void *p2);
void xvid_correct_framerate(AVCodecContext *avctx);

/**
 * Creates the private context for the encoder.
 * All buffers are allocated, settings are loaded from the user,
 * and the encoder context created.
 *
 * @param avctx AVCodecContext pointer to context
 * @return Returns 0 on success, -1 on failure
 */
static av_cold int xvid_encode_init(AVCodecContext *avctx)  {
    int xerr, i;
    int xvid_flags = avctx->flags;
    struct xvid_context *x = avctx->priv_data;
    uint16_t *intra, *inter;
    int fd;

    xvid_plugin_single_t single;
    struct xvid_ff_pass1 rc2pass1;
    xvid_plugin_2pass2_t rc2pass2;
    xvid_gbl_init_t xvid_gbl_init;
    xvid_enc_create_t xvid_enc_create;
    xvid_enc_plugin_t plugins[7];

    /* Bring in VOP flags from ffmpeg command-line */
    x->vop_flags = XVID_VOP_HALFPEL; /* Bare minimum quality */
    if( xvid_flags & CODEC_FLAG_4MV )
        x->vop_flags |= XVID_VOP_INTER4V; /* Level 3 */
    if( avctx->trellis
        )
        x->vop_flags |= XVID_VOP_TRELLISQUANT; /* Level 5 */
    if( xvid_flags & CODEC_FLAG_AC_PRED )
        x->vop_flags |= XVID_VOP_HQACPRED; /* Level 6 */
    if( xvid_flags & CODEC_FLAG_GRAY )
        x->vop_flags |= XVID_VOP_GREYSCALE;

    /* Decide which ME quality setting to use */
    x->me_flags = 0;
    switch( avctx->me_method ) {
       case ME_FULL:   /* Quality 6 */
           x->me_flags |=  XVID_ME_EXTSEARCH16
                       |   XVID_ME_EXTSEARCH8;

       case ME_EPZS:   /* Quality 4 */
           x->me_flags |=  XVID_ME_ADVANCEDDIAMOND8
                       |   XVID_ME_HALFPELREFINE8
                       |   XVID_ME_CHROMA_PVOP
                       |   XVID_ME_CHROMA_BVOP;

       case ME_LOG:    /* Quality 2 */
       case ME_PHODS:
       case ME_X1:
           x->me_flags |=  XVID_ME_ADVANCEDDIAMOND16
                       |   XVID_ME_HALFPELREFINE16;

       case ME_ZERO:   /* Quality 0 */
       default:
           break;
    }

    /* Decide how we should decide blocks */
    switch( avctx->mb_decision ) {
       case 2:
           x->vop_flags |= XVID_VOP_MODEDECISION_RD;
           x->me_flags |=  XVID_ME_HALFPELREFINE8_RD
                       |   XVID_ME_QUARTERPELREFINE8_RD
                       |   XVID_ME_EXTSEARCH_RD
                       |   XVID_ME_CHECKPREDICTION_RD;
       case 1:
           if( !(x->vop_flags & XVID_VOP_MODEDECISION_RD) )
               x->vop_flags |= XVID_VOP_FAST_MODEDECISION_RD;
           x->me_flags |=  XVID_ME_HALFPELREFINE16_RD
                       |   XVID_ME_QUARTERPELREFINE16_RD;

       default:
           break;
    }

    /* Bring in VOL flags from ffmpeg command-line */
    x->vol_flags = 0;
    if( xvid_flags & CODEC_FLAG_GMC ) {
        x->vol_flags |= XVID_VOL_GMC;
        x->me_flags |= XVID_ME_GME_REFINE;
    }
    if( xvid_flags & CODEC_FLAG_QPEL ) {
        x->vol_flags |= XVID_VOL_QUARTERPEL;
        x->me_flags |= XVID_ME_QUARTERPELREFINE16;
        if( x->vop_flags & XVID_VOP_INTER4V )
            x->me_flags |= XVID_ME_QUARTERPELREFINE8;
    }

    memset(&xvid_gbl_init, 0, sizeof(xvid_gbl_init));
    xvid_gbl_init.version = XVID_VERSION;
    xvid_gbl_init.debug = 0;

#if ARCH_PPC
    /* Xvid's PPC support is borked, use libavcodec to detect */
#if HAVE_ALTIVEC
    if( has_altivec() ) {
        xvid_gbl_init.cpu_flags = XVID_CPU_FORCE | XVID_CPU_ALTIVEC;
    } else
#endif
        xvid_gbl_init.cpu_flags = XVID_CPU_FORCE;
#else
    /* Xvid can detect on x86 */
    xvid_gbl_init.cpu_flags = 0;
#endif

    /* Initialize */
    xvid_global(NULL, XVID_GBL_INIT, &xvid_gbl_init, NULL);

    /* Create the encoder reference */
    memset(&xvid_enc_create, 0, sizeof(xvid_enc_create));
    xvid_enc_create.version = XVID_VERSION;

    /* Store the desired frame size */
    xvid_enc_create.width = x->xsize = avctx->width;
    xvid_enc_create.height = x->ysize = avctx->height;

    /* Xvid can determine the proper profile to use */
    /* xvid_enc_create.profile = XVID_PROFILE_S_L3; */

    /* We don't use zones */
    xvid_enc_create.zones = NULL;
    xvid_enc_create.num_zones = 0;

    xvid_enc_create.num_threads = avctx->thread_count;

    xvid_enc_create.plugins = plugins;
    xvid_enc_create.num_plugins = 0;

    /* Initialize Buffers */
    x->twopassbuffer = NULL;
    x->old_twopassbuffer = NULL;
    x->twopassfile = NULL;

    if( xvid_flags & CODEC_FLAG_PASS1 ) {
        memset(&rc2pass1, 0, sizeof(struct xvid_ff_pass1));
        rc2pass1.version = XVID_VERSION;
        rc2pass1.context = x;
        x->twopassbuffer = av_malloc(BUFFER_SIZE);
        x->old_twopassbuffer = av_malloc(BUFFER_SIZE);
        if( x->twopassbuffer == NULL || x->old_twopassbuffer == NULL ) {
            av_log(avctx, AV_LOG_ERROR,
                "Xvid: Cannot allocate 2-pass log buffers\n");
            return -1;
        }
        x->twopassbuffer[0] = x->old_twopassbuffer[0] = 0;

        plugins[xvid_enc_create.num_plugins].func = xvid_ff_2pass;
        plugins[xvid_enc_create.num_plugins].param = &rc2pass1;
        xvid_enc_create.num_plugins++;
    } else if( xvid_flags & CODEC_FLAG_PASS2 ) {
        memset(&rc2pass2, 0, sizeof(xvid_plugin_2pass2_t));
        rc2pass2.version = XVID_VERSION;
        rc2pass2.bitrate = avctx->bit_rate;

        fd = av_tempfile("xvidff.", &(x->twopassfile));
        if( fd == -1 ) {
            av_log(avctx, AV_LOG_ERROR,
                "Xvid: Cannot write 2-pass pipe\n");
            return -1;
        }

        if( avctx->stats_in == NULL ) {
            av_log(avctx, AV_LOG_ERROR,
                "Xvid: No 2-pass information loaded for second pass\n");
            return -1;
        }

        if( strlen(avctx->stats_in) >
              write(fd, avctx->stats_in, strlen(avctx->stats_in)) ) {
            close(fd);
            av_log(avctx, AV_LOG_ERROR,
                "Xvid: Cannot write to 2-pass pipe\n");
            return -1;
        }

        close(fd);
        rc2pass2.filename = x->twopassfile;
        plugins[xvid_enc_create.num_plugins].func = xvid_plugin_2pass2;
        plugins[xvid_enc_create.num_plugins].param = &rc2pass2;
        xvid_enc_create.num_plugins++;
    } else if( !(xvid_flags & CODEC_FLAG_QSCALE) ) {
        /* Single Pass Bitrate Control! */
        memset(&single, 0, sizeof(xvid_plugin_single_t));
        single.version = XVID_VERSION;
        single.bitrate = avctx->bit_rate;

        plugins[xvid_enc_create.num_plugins].func = xvid_plugin_single;
        plugins[xvid_enc_create.num_plugins].param = &single;
        xvid_enc_create.num_plugins++;
    }

    /* Luminance Masking */
    if( 0.0 != avctx->lumi_masking ) {
        plugins[xvid_enc_create.num_plugins].func = xvid_plugin_lumimasking;
        plugins[xvid_enc_create.num_plugins].param = NULL;
        xvid_enc_create.num_plugins++;
    }

    /* Frame Rate and Key Frames */
    xvid_correct_framerate(avctx);
    xvid_enc_create.fincr = avctx->time_base.num;
    xvid_enc_create.fbase = avctx->time_base.den;
    if( avctx->gop_size > 0 )
        xvid_enc_create.max_key_interval = avctx->gop_size;
    else
        xvid_enc_create.max_key_interval = 240; /* Xvid's best default */

    /* Quants */
    if( xvid_flags & CODEC_FLAG_QSCALE ) x->qscale = 1;
    else x->qscale = 0;

    xvid_enc_create.min_quant[0] = avctx->qmin;
    xvid_enc_create.min_quant[1] = avctx->qmin;
    xvid_enc_create.min_quant[2] = avctx->qmin;
    xvid_enc_create.max_quant[0] = avctx->qmax;
    xvid_enc_create.max_quant[1] = avctx->qmax;
    xvid_enc_create.max_quant[2] = avctx->qmax;

    /* Quant Matrices */
    x->intra_matrix = x->inter_matrix = NULL;
    if( avctx->mpeg_quant )
       x->vol_flags |= XVID_VOL_MPEGQUANT;
    if( (avctx->intra_matrix || avctx->inter_matrix) ) {
       x->vol_flags |= XVID_VOL_MPEGQUANT;

       if( avctx->intra_matrix ) {
           intra = avctx->intra_matrix;
           x->intra_matrix = av_malloc(sizeof(unsigned char) * 64);
       } else
           intra = NULL;
       if( avctx->inter_matrix ) {
           inter = avctx->inter_matrix;
           x->inter_matrix = av_malloc(sizeof(unsigned char) * 64);
       } else
           inter = NULL;

       for( i = 0; i < 64; i++ ) {
           if( intra )
               x->intra_matrix[i] = (unsigned char)intra[i];
           if( inter )
               x->inter_matrix[i] = (unsigned char)inter[i];
       }
    }

    /* Misc Settings */
    xvid_enc_create.frame_drop_ratio = 0;
    xvid_enc_create.global = 0;
    if( xvid_flags & CODEC_FLAG_CLOSED_GOP )
        xvid_enc_create.global |= XVID_GLOBAL_CLOSED_GOP;

    /* Determines which codec mode we are operating in */
    avctx->extradata = NULL;
    avctx->extradata_size = 0;
    if( xvid_flags & CODEC_FLAG_GLOBAL_HEADER ) {
        /* In this case, we are claiming to be MPEG4 */
        x->quicktime_format = 1;
        avctx->codec_id = CODEC_ID_MPEG4;
    } else {
        /* We are claiming to be Xvid */
        x->quicktime_format = 0;
        if(!avctx->codec_tag)
            avctx->codec_tag = AV_RL32("xvid");
    }

    /* Bframes */
    xvid_enc_create.max_bframes = avctx->max_b_frames;
    xvid_enc_create.bquant_offset = 100 * avctx->b_quant_offset;
    xvid_enc_create.bquant_ratio = 100 * avctx->b_quant_factor;
    if( avctx->max_b_frames > 0  && !x->quicktime_format ) xvid_enc_create.global |= XVID_GLOBAL_PACKED;

    /* Create encoder context */
    xerr = xvid_encore(NULL, XVID_ENC_CREATE, &xvid_enc_create, NULL);
    if( xerr ) {
        av_log(avctx, AV_LOG_ERROR, "Xvid: Could not create encoder reference\n");
        return -1;
    }

    x->encoder_handle = xvid_enc_create.handle;
    avctx->coded_frame = &x->encoded_picture;

    return 0;
}

/**
 * Encodes a single frame.
 *
 * @param avctx AVCodecContext pointer to context
 * @param frame Pointer to encoded frame buffer
 * @param buf_size Size of encoded frame buffer
 * @param data Pointer to AVFrame of unencoded frame
 * @return Returns 0 on success, -1 on failure
 */
static int xvid_encode_frame(AVCodecContext *avctx,
                         unsigned char *frame, int buf_size, void *data) {
    int xerr, i;
    char *tmp;
    struct xvid_context *x = avctx->priv_data;
    AVFrame *picture = data;
    AVFrame *p = &(x->encoded_picture);

    xvid_enc_frame_t xvid_enc_frame;
    xvid_enc_stats_t xvid_enc_stats;

    /* Start setting up the frame */
    memset(&xvid_enc_frame, 0, sizeof(xvid_enc_frame));
    xvid_enc_frame.version = XVID_VERSION;
    memset(&xvid_enc_stats, 0, sizeof(xvid_enc_stats));
    xvid_enc_stats.version = XVID_VERSION;
    *p = *picture;

    /* Let Xvid know where to put the frame. */
    xvid_enc_frame.bitstream = frame;
    xvid_enc_frame.length = buf_size;

    /* Initialize input image fields */
    if( avctx->pix_fmt != PIX_FMT_YUV420P ) {
        av_log(avctx, AV_LOG_ERROR, "Xvid: Color spaces other than 420p not supported\n");
        return -1;
    }

    xvid_enc_frame.input.csp = XVID_CSP_PLANAR; /* YUV420P */

    for( i = 0; i < 4; i++ ) {
        xvid_enc_frame.input.plane[i] = picture->data[i];
        xvid_enc_frame.input.stride[i] = picture->linesize[i];
    }

    /* Encoder Flags */
    xvid_enc_frame.vop_flags = x->vop_flags;
    xvid_enc_frame.vol_flags = x->vol_flags;
    xvid_enc_frame.motion = x->me_flags;
    xvid_enc_frame.type = XVID_TYPE_AUTO;

    /* Pixel aspect ratio setting */
    if (avctx->sample_aspect_ratio.num < 1 || avctx->sample_aspect_ratio.num > 255 ||
        avctx->sample_aspect_ratio.den < 1 || avctx->sample_aspect_ratio.den > 255) {
        av_log(avctx, AV_LOG_ERROR, "Invalid pixel aspect ratio %i/%i\n",
               avctx->sample_aspect_ratio.num, avctx->sample_aspect_ratio.den);
        return -1;
    }
    xvid_enc_frame.par = XVID_PAR_EXT;
    xvid_enc_frame.par_width  = avctx->sample_aspect_ratio.num;
    xvid_enc_frame.par_height = avctx->sample_aspect_ratio.den;

    /* Quant Setting */
    if( x->qscale ) xvid_enc_frame.quant = picture->quality / FF_QP2LAMBDA;
    else xvid_enc_frame.quant = 0;

    /* Matrices */
    xvid_enc_frame.quant_intra_matrix = x->intra_matrix;
    xvid_enc_frame.quant_inter_matrix = x->inter_matrix;

    /* Encode */
    xerr = xvid_encore(x->encoder_handle, XVID_ENC_ENCODE,
        &xvid_enc_frame, &xvid_enc_stats);

    /* Two-pass log buffer swapping */
    avctx->stats_out = NULL;
    if( x->twopassbuffer ) {
        tmp = x->old_twopassbuffer;
        x->old_twopassbuffer = x->twopassbuffer;
        x->twopassbuffer = tmp;
        x->twopassbuffer[0] = 0;
        if( x->old_twopassbuffer[0] != 0 ) {
            avctx->stats_out = x->old_twopassbuffer;
        }
    }

    if( 0 <= xerr ) {
        p->quality = xvid_enc_stats.quant * FF_QP2LAMBDA;
        if( xvid_enc_stats.type == XVID_TYPE_PVOP )
            p->pict_type = FF_P_TYPE;
        else if( xvid_enc_stats.type == XVID_TYPE_BVOP )
            p->pict_type = FF_B_TYPE;
        else if( xvid_enc_stats.type == XVID_TYPE_SVOP )
            p->pict_type = FF_S_TYPE;
        else
            p->pict_type = FF_I_TYPE;
        if( xvid_enc_frame.out_flags & XVID_KEYFRAME ) {
            p->key_frame = 1;
            if( x->quicktime_format )
                return xvid_strip_vol_header(avctx, frame,
                    xvid_enc_stats.hlength, xerr);
         } else
            p->key_frame = 0;

        return xerr;
    } else {
        av_log(avctx, AV_LOG_ERROR, "Xvid: Encoding Error Occurred: %i\n", xerr);
        return -1;
    }
}

/**
 * Destroys the private context for the encoder.
 * All buffers are freed, and the Xvid encoder context is destroyed.
 *
 * @param avctx AVCodecContext pointer to context
 * @return Returns 0, success guaranteed
 */
static av_cold int xvid_encode_close(AVCodecContext *avctx) {
    struct xvid_context *x = avctx->priv_data;

    xvid_encore(x->encoder_handle, XVID_ENC_DESTROY, NULL, NULL);

    if( avctx->extradata != NULL )
        av_freep(&avctx->extradata);
    if( x->twopassbuffer != NULL ) {
        av_free(x->twopassbuffer);
        av_free(x->old_twopassbuffer);
    }
    if( x->twopassfile != NULL )
        av_free(x->twopassfile);
    if( x->intra_matrix != NULL )
        av_free(x->intra_matrix);
    if( x->inter_matrix != NULL )
        av_free(x->inter_matrix);

    return 0;
}

/**
 * Routine to create a global VO/VOL header for MP4 container.
 * What we do here is extract the header from the Xvid bitstream
 * as it is encoded. We also strip the repeated headers from the
 * bitstream when a global header is requested for MPEG-4 ISO
 * compliance.
 *
 * @param avctx AVCodecContext pointer to context
 * @param frame Pointer to encoded frame data
 * @param header_len Length of header to search
 * @param frame_len Length of encoded frame data
 * @return Returns new length of frame data
 */
int xvid_strip_vol_header(AVCodecContext *avctx,
                  unsigned char *frame,
                  unsigned int header_len,
                  unsigned int frame_len) {
    int vo_len = 0, i;

    for( i = 0; i < header_len - 3; i++ ) {
        if( frame[i] == 0x00 &&
            frame[i+1] == 0x00 &&
            frame[i+2] == 0x01 &&
            frame[i+3] == 0xB6 ) {
            vo_len = i;
            break;
        }
    }

    if( vo_len > 0 ) {
        /* We need to store the header, so extract it */
        if( avctx->extradata == NULL ) {
            avctx->extradata = av_malloc(vo_len);
            memcpy(avctx->extradata, frame, vo_len);
            avctx->extradata_size = vo_len;
        }
        /* Less dangerous now, memmove properly copies the two
           chunks of overlapping data */
        memmove(frame, &(frame[vo_len]), frame_len - vo_len);
        return frame_len - vo_len;
    } else
        return frame_len;
}

/**
 * Routine to correct a possibly erroneous framerate being fed to us.
 * Xvid currently chokes on framerates where the ticks per frame is
 * extremely large. This function works to correct problems in this area
 * by estimating a new framerate and taking the simpler fraction of
 * the two presented.
 *
 * @param avctx Context that contains the framerate to correct.
 */
void xvid_correct_framerate(AVCodecContext *avctx) {
    int frate, fbase;
    int est_frate, est_fbase;
    int gcd;
    float est_fps, fps;

    frate = avctx->time_base.den;
    fbase = avctx->time_base.num;

    gcd = av_gcd(frate, fbase);
    if( gcd > 1 ) {
        frate /= gcd;
        fbase /= gcd;
    }

    if( frate <= 65000 && fbase <= 65000 ) {
        avctx->time_base.den = frate;
        avctx->time_base.num = fbase;
        return;
    }

    fps = (float)frate / (float)fbase;
    est_fps = roundf(fps * 1000.0) / 1000.0;

    est_frate = (int)est_fps;
    if( est_fps > (int)est_fps ) {
        est_frate = (est_frate + 1) * 1000;
        est_fbase = (int)roundf((float)est_frate / est_fps);
    } else
        est_fbase = 1;

    gcd = av_gcd(est_frate, est_fbase);
    if( gcd > 1 ) {
        est_frate /= gcd;
        est_fbase /= gcd;
    }

    if( fbase > est_fbase ) {
        avctx->time_base.den = est_frate;
        avctx->time_base.num = est_fbase;
        av_log(avctx, AV_LOG_DEBUG,
            "Xvid: framerate re-estimated: %.2f, %.3f%% correction\n",
            est_fps, (((est_fps - fps)/fps) * 100.0));
    } else {
        avctx->time_base.den = frate;
        avctx->time_base.num = fbase;
    }
}

/*
 * Xvid 2-Pass Kludge Section
 *
 * Xvid's default 2-pass doesn't allow us to create data as we need to, so
 * this section spends time replacing the first pass plugin so we can write
 * statistic information as libavcodec requests in. We have another kludge
 * that allows us to pass data to the second pass in Xvid without a custom
 * rate-control plugin.
 */

/**
 * Initializes the two-pass plugin and context.
 *
 * @param param Input construction parameter structure
 * @param handle Private context handle
 * @return Returns XVID_ERR_xxxx on failure, or 0 on success.
 */
static int xvid_ff_2pass_create(xvid_plg_create_t * param,
                                void ** handle) {
    struct xvid_ff_pass1 *x = (struct xvid_ff_pass1 *)param->param;
    char *log = x->context->twopassbuffer;

    /* Do a quick bounds check */
    if( log == NULL )
        return XVID_ERR_FAIL;

    /* We use snprintf() */
    /* This is because we can safely prevent a buffer overflow */
    log[0] = 0;
    snprintf(log, BUFFER_REMAINING(log),
        "# ffmpeg 2-pass log file, using xvid codec\n");
    snprintf(BUFFER_CAT(log), BUFFER_REMAINING(log),
        "# Do not modify. libxvidcore version: %d.%d.%d\n\n",
        XVID_VERSION_MAJOR(XVID_VERSION),
        XVID_VERSION_MINOR(XVID_VERSION),
        XVID_VERSION_PATCH(XVID_VERSION));

    *handle = x->context;
    return 0;
}

/**
 * Destroys the two-pass plugin context.
 *
 * @param ref Context pointer for the plugin
 * @param param Destrooy context
 * @return Returns 0, success guaranteed
 */
static int xvid_ff_2pass_destroy(struct xvid_context *ref,
                                xvid_plg_destroy_t *param) {
    /* Currently cannot think of anything to do on destruction */
    /* Still, the framework should be here for reference/use */
    if( ref->twopassbuffer != NULL )
        ref->twopassbuffer[0] = 0;
    return 0;
}

/**
 * Enables fast encode mode during the first pass.
 *
 * @param ref Context pointer for the plugin
 * @param param Frame data
 * @return Returns 0, success guaranteed
 */
static int xvid_ff_2pass_before(struct xvid_context *ref,
                                xvid_plg_data_t *param) {
    int motion_remove;
    int motion_replacements;
    int vop_remove;

    /* Nothing to do here, result is changed too much */
    if( param->zone && param->zone->mode == XVID_ZONE_QUANT )
        return 0;

    /* We can implement a 'turbo' first pass mode here */
    param->quant = 2;

    /* Init values */
    motion_remove = ~XVID_ME_CHROMA_PVOP &
                    ~XVID_ME_CHROMA_BVOP &
                    ~XVID_ME_EXTSEARCH16 &
                    ~XVID_ME_ADVANCEDDIAMOND16;
    motion_replacements = XVID_ME_FAST_MODEINTERPOLATE |
                          XVID_ME_SKIP_DELTASEARCH |
                          XVID_ME_FASTREFINE16 |
                          XVID_ME_BFRAME_EARLYSTOP;
    vop_remove = ~XVID_VOP_MODEDECISION_RD &
                 ~XVID_VOP_FAST_MODEDECISION_RD &
                 ~XVID_VOP_TRELLISQUANT &
                 ~XVID_VOP_INTER4V &
                 ~XVID_VOP_HQACPRED;

    param->vol_flags &= ~XVID_VOL_GMC;
    param->vop_flags &= vop_remove;
    param->motion_flags &= motion_remove;
    param->motion_flags |= motion_replacements;

    return 0;
}

/**
 * Captures statistic data and writes it during first pass.
 *
 * @param ref Context pointer for the plugin
 * @param param Statistic data
 * @return Returns XVID_ERR_xxxx on failure, or 0 on success
 */
static int xvid_ff_2pass_after(struct xvid_context *ref,
                                xvid_plg_data_t *param) {
    char *log = ref->twopassbuffer;
    char *frame_types = " ipbs";
    char frame_type;

    /* Quick bounds check */
    if( log == NULL )
        return XVID_ERR_FAIL;

    /* Convert the type given to us into a character */
    if( param->type < 5 && param->type > 0 ) {
        frame_type = frame_types[param->type];
    } else {
        return XVID_ERR_FAIL;
    }

    snprintf(BUFFER_CAT(log), BUFFER_REMAINING(log),
        "%c %d %d %d %d %d %d\n",
        frame_type, param->stats.quant, param->stats.kblks, param->stats.mblks,
        param->stats.ublks, param->stats.length, param->stats.hlength);

    return 0;
}

/**
 * Dispatch function for our custom plugin.
 * This handles the dispatch for the Xvid plugin. It passes data
 * on to other functions for actual processing.
 *
 * @param ref Context pointer for the plugin
 * @param cmd The task given for us to complete
 * @param p1 First parameter (varies)
 * @param p2 Second parameter (varies)
 * @return Returns XVID_ERR_xxxx on failure, or 0 on success
 */
int xvid_ff_2pass(void *ref, int cmd, void *p1, void *p2) {
    switch( cmd ) {
        case XVID_PLG_INFO:
        case XVID_PLG_FRAME:
            return 0;

        case XVID_PLG_BEFORE:
            return xvid_ff_2pass_before(ref, p1);

        case XVID_PLG_CREATE:
            return xvid_ff_2pass_create(p1, p2);

        case XVID_PLG_AFTER:
            return xvid_ff_2pass_after(ref, p1);

        case XVID_PLG_DESTROY:
            return xvid_ff_2pass_destroy(ref, p1);

        default:
            return XVID_ERR_FAIL;
    }
}

/**
 * Xvid codec definition for libavcodec.
 */
AVCodec libxvid_encoder = {
    "libxvid",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_MPEG4,
    sizeof(struct xvid_context),
    xvid_encode_init,
    xvid_encode_frame,
    xvid_encode_close,
    .pix_fmts= (const enum PixelFormat[]){PIX_FMT_YUV420P, PIX_FMT_NONE},
    .long_name= NULL_IF_CONFIG_SMALL("libxvidcore MPEG-4 part 2"),
};
