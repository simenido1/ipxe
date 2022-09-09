/*
 * Copyright (C) 2022 Georgii Simenido <simenido1@gmail.com>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * You can also choose to distribute this program under the terms of
 * the Unmodified Binary Distribution Licence (as given in the file
 * COPYING.UBDL), provided that you have satisfied its requirements.
 */

FILE_LICENCE(GPL2_OR_LATER_OR_UBDL);

#include <ipxe/avi.h>
#include <string.h>
#include <ipxe/umalloc.h>
#include "libavutil/avutil.h"

static struct avi_image_context
{
    int size;
    int internal_position;
    intptr_t start_ptr;
    int video_stream_index;
    AVFormatContext *pFormatContext;
    AVCodecContext *pCodecContext;
    struct pixel_buffer *pixbuf;
    double framerate;
    AVFrame *rgbFrame;
    AVFrame *pFrame;
    AVPacket *pPacket;
    struct SwsContext * sws_ctx;
} avi_image_context;

void avi_image_context_clear(void)
{
    avi_image_context.size = 0;
    avi_image_context.internal_position = 0;
    avi_image_context.start_ptr = 0;
    avi_image_context.video_stream_index = -1;
    avi_image_context.framerate = 0;
    if (avi_image_context.pFormatContext != NULL)
    {
        if (avi_image_context.pFormatContext->pb != NULL)
        {
            avio_context_free(&(avi_image_context.pFormatContext->pb));
        }
        avformat_free_context(avi_image_context.pFormatContext);
        avi_image_context.pFormatContext = NULL;
    }
    if (avi_image_context.pCodecContext != NULL)
    {
        avcodec_free_context(&avi_image_context.pCodecContext);
        avi_image_context.pCodecContext = NULL;
    }
    if (avi_image_context.pixbuf != NULL)
    {
        ufree(avi_image_context.pixbuf->data);
        free(avi_image_context.pixbuf);
        avi_image_context.pixbuf = NULL;
    }
    if (avi_image_context.rgbFrame != NULL)
    {
        av_frame_unref(avi_image_context.rgbFrame);
        av_frame_free(&avi_image_context.rgbFrame);
        avi_image_context.rgbFrame = NULL;
    }
    if (avi_image_context.pFrame != NULL)
    {
        av_frame_unref(avi_image_context.pFrame);
        av_frame_free(&avi_image_context.pFrame);
        avi_image_context.pFrame = NULL;
    }
    if (avi_image_context.pPacket != NULL)
    {
        av_packet_unref(avi_image_context.pPacket);
        av_packet_free(&avi_image_context.pPacket);
        avi_image_context.pPacket = NULL;
    }
    if (avi_image_context.sws_ctx != NULL)
    {
        sws_freeContext(avi_image_context.sws_ctx);
        avi_image_context.sws_ctx = NULL;
    }
}
// Read buffer for avio_alloc_context
int read_buffer(void *opaque, uint8_t *buf, int buf_size)
{
    if (avi_image_context.internal_position >= avi_image_context.size)
    {
        return AVERROR_EOF; // EOF
    }
    int actual_bytes_read = buf_size;
    if (avi_image_context.internal_position + buf_size > avi_image_context.size)
    {
        actual_bytes_read = avi_image_context.size - avi_image_context.internal_position;
    }
    memcpy(buf, avi_image_context.start_ptr + avi_image_context.internal_position, actual_bytes_read);
    //copy_from_user(buf, avi_image_context.start_ptr, avi_image_context.internal_position, actual_bytes_read);
    avi_image_context.internal_position += actual_bytes_read;
    // printf("actual_bytes_read=%d, internal_position=%d, size=%d\n", actual_bytes_read, image_internal_position, image_size);
    return actual_bytes_read;
}

int64_t seek_buffer(void *opaque, int64_t offset, int whence)
{
    int64_t start_pos = 0;
    switch (whence)
    {
    case SEEK_SET:
        start_pos = 0;
        break;
    case SEEK_CUR:
        start_pos = avi_image_context.internal_position;
        break;
    case SEEK_END:
        start_pos = avi_image_context.size;
        break;
    case AVSEEK_SIZE:
        return avi_image_context.size;
        break;
    default:
        // printf("seek_buffer incorrect whence:%d\n", whence);
        return -1;
        break;
    }
    int64_t final_pos = start_pos + offset;
    if (final_pos < 0)
    {
        // final_ptr = opaque;
        return -1;
    }
    if (final_pos > avi_image_context.size)
    {
        // final_ptr = image_end;
        return -1;
    }
    avi_image_context.internal_position = final_pos;
    return 0;
}

static AVFrame *avi_alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
{
    AVFrame *picture;
    int ret;
    picture = av_frame_alloc();
    if (!picture)
        return NULL;
    picture->format = pix_fmt;
    picture->width = width;
    picture->height = height;
    /* allocate the buffers for the frame data */
    ret = av_frame_get_buffer(picture, 0);
    if (ret < 0)
    {
        //printf("Cannot allocate buffers for picture!\n");
        return NULL;
    }
    return picture;
}

int fill_rgb_frame(void)
{
    int ret = sws_scale(avi_image_context.sws_ctx, (const uint8_t *const *)avi_image_context.pFrame->data,
                        avi_image_context.pFrame->linesize, 0,avi_image_context.pFrame->height,
                        avi_image_context.rgbFrame->data, avi_image_context.rgbFrame->linesize);
    return ret < 0 ? ret : 0;
}

/**
 * Probe AVI image
 *
 * @v image		AVI image
 * @ret rc		Return status code
 */
static int avi_image_probe(struct image *image)
{
    int ret = 0;
    av_log_set_level(AV_LOG_ERROR); //TRACE, DEBUG, VERBOSE, INFO, WARNING, ERROR, FATAL, PANIC, QUIET
    avi_image_context_clear();
    avi_image_context.size = image->len;
    avi_image_context.internal_position = 0;
    avi_image_context.start_ptr = image->data;
    AVFormatContext *pFormatContext = avformat_alloc_context();
    if (!pFormatContext)
    {
       // printf("ERROR! Can't allocate AVFormatContext\n");
        goto error_alloc_format_ctx;
    }
    // setup pb field with custom io stream.
    const int avio_buffer_size = 8192 /* + 64 */;
    unsigned char *avio_buffer = (unsigned char *)av_malloc(avio_buffer_size);
    AVIOContext *avio = avio_alloc_context(avio_buffer, avio_buffer_size, 0, NULL, read_buffer, NULL, seek_buffer);
    pFormatContext->pb = avio;
    // pFormatContext->flags |= AVFMT_FLAG_CUSTOM_IO;
    if (!pFormatContext->pb)
    {
        //printf("ERROR! Can't allocate AVIOContext\n");
        goto error_alloc_avio_ctx;
    }
    if ((ret = avformat_open_input(&pFormatContext, NULL, NULL, NULL)) != 0)
    {
        //printf("ERROR! Can't open file with avformat_open_input(). Error code: %d\n", ret);
        goto error_open_input;
    }
    // printf("internal_position=%d\n", image_internal_position);
    if (avformat_find_stream_info(pFormatContext, NULL) < 0)
    {
        printf("ERROR! Can't get the stream info\n");
        goto error_find_stream_info;
    }
    // printf("internal_position=%d\n", image_internal_position);
    //  Initialize the codec paramters for subsequent usage.
    AVCodec *pCodec = NULL;
    AVCodecParameters *pCodecParameters = NULL;

    for (int i = 0; i < pFormatContext->nb_streams; i++)
    {
        AVCodecParameters *pLocalCodecParameters = pFormatContext->streams[i]->codecpar;
        if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            if (avi_image_context.video_stream_index == -1)
            {
                avi_image_context.video_stream_index = i;
                pCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);
                if (!pCodec)
                {
                    printf("Error! Can't find codec for %s, %s\n",
                           av_get_media_type_string(pLocalCodecParameters->codec_type),
                           avcodec_get_name(pLocalCodecParameters->codec_id));
                    goto error_find_decoder;
                }
                pCodecParameters = pLocalCodecParameters;
                avi_image_context.framerate = (double)pFormatContext->streams[i]->r_frame_rate.num / (double)pFormatContext->streams[i]->r_frame_rate.den;
            }
        }
    }
    if (avi_image_context.video_stream_index == -1)
    {
        printf("Error! Video stream not found!\n");
        goto error_video_not_found;
    }

    AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
    if (pCodecContext == NULL)
    {
        //printf("ERROR! Cannot allocate memory for pCodecContext\n");
        goto error_codec_ctx;
    }

    if (avcodec_parameters_to_context(pCodecContext, pCodecParameters) < 0)
    {
        //printf("ERROR! failed to copy codec params to codec context\n");
        goto error_parameters_to_codec_ctx;
    }

    if (avcodec_open2(pCodecContext, pCodec, NULL) < 0)
    {
        printf("failed to open codec through avcodec_open2\n");
        goto error_open_codec;
    }
    avi_image_context.pixbuf = alloc_pixbuf(pCodecContext->width, pCodecContext->height);
    if (!avi_image_context.pixbuf)
    {
        //printf("failed to alloc memory for pixbuf\n");
        goto error_pixbuf_alloc;
    }
    avi_image_context.rgbFrame = avi_alloc_picture(AV_PIX_FMT_0RGB32, pCodecContext->width, pCodecContext->height);
    if (!avi_image_context.rgbFrame)
    {
        //printf("failed to alloc memory for rgbFrame\n");
        goto error_rgbframe_alloc;
    }
    avi_image_context.pFrame = av_frame_alloc();
    if (!avi_image_context.pFrame)
    {
        //printf("failed to allocate memory for pFrame\n");
        goto error_pframe_alloc;
    }
    avi_image_context.pPacket = av_packet_alloc();
    if (!avi_image_context.pPacket)
    {
        //printf("failed to allocate memory for pPacket\n");
        goto error_packet_alloc;
    }
    avi_image_context.sws_ctx = sws_getContext(pCodecContext->width, pCodecContext->height, pCodecContext->pix_fmt, pCodecContext->width, pCodecContext->height,
                                            AV_PIX_FMT_0RGB32, SWS_FAST_BILINEAR, 0, 0, 0);
    if (!avi_image_context.sws_ctx)
    {
        //printf("Failed to allocate memory for sws_ctx!\n");
        goto error_sws_alloc;
    }
    // here ends the avi probe!
    avi_image_context.pFormatContext = pFormatContext;
    avi_image_context.pCodecContext = pCodecContext;
    return 0;

error_sws_alloc:
error_packet_alloc:
error_pframe_alloc:
error_rgbframe_alloc:
    pixbuf_put(avi_image_context.pixbuf);
error_pixbuf_alloc:
error_open_codec:
error_parameters_to_codec_ctx:
    avcodec_free_context(&pCodecContext);
error_codec_ctx:
error_video_not_found:
error_find_decoder:
error_find_stream_info:
error_open_input:
    avio_context_free(&avio);
error_alloc_avio_ctx:
    avformat_close_input(&pFormatContext);
error_alloc_format_ctx:
    avi_image_context_clear();
    return -1;
}

/**
 * Convert AVI frame to pixel buffer
 *
 * @v image		AVI image
 * @v pixbuf		Pixel buffer to fill in
 * @ret rc		Return status code
 */

static int avi_pixbuf(struct image *image, struct pixel_buffer **pixbuf)
{
    int response = 0;
start:
    while ((response = av_read_frame(avi_image_context.pFormatContext, avi_image_context.pPacket)) >= 0)
    {
        // if it's the video stream
        if (avi_image_context.pPacket->stream_index == avi_image_context.video_stream_index)
        {
            response = avcodec_send_packet(avi_image_context.pCodecContext, avi_image_context.pPacket);
            if (response < 0)
            {
                break;
                goto error_pix_fmt;
            }
            else
            {
                response = avcodec_receive_frame(avi_image_context.pCodecContext, avi_image_context.pFrame);
                if (response >= 0)
                {
                    // if (fill_rgb_frame(avi_image_context.pFrame) < 0)
                    // {
                    //     break;
                    //     goto error_pix_fmt;
                    // }
                    fill_rgb_frame();
                    copy_to_user(avi_image_context.pixbuf->data, 0, avi_image_context.rgbFrame->data[0],
                                 4 * avi_image_context.rgbFrame->width * avi_image_context.rgbFrame->height);
                    //*pixbuf = pixbuf_get(*pixbuf);
                    //*pixbuf = avi_image_context.pixbuf;
                    *pixbuf = pixbuf_get(avi_image_context.pixbuf);
                    //av_frame_unref(avi_image_context.pFrame);
                    //av_frame_unref(avi_image_context.rgbFrame); //????? realloc buffers later?
                    av_packet_unref(avi_image_context.pPacket);
                    break;
                }
            }
        }
        av_packet_unref(avi_image_context.pPacket);
    }
    if (response == AVERROR_EOF) // EOF was reached, need to go to start of video
    {
        // avio_seek(avi_image_context.pFormatContext->pb, 0, SEEK_SET);
        if (avformat_seek_file(avi_image_context.pFormatContext, avi_image_context.video_stream_index, 0, 0,
                               avi_image_context.pFormatContext->streams[avi_image_context.video_stream_index]->duration, 0) >= 0)
        {
            goto start;
        }
    }
    return response < 0 ? response : 0;

error_pix_fmt:
    av_frame_unref(avi_image_context.pFrame);
    //av_frame_unref(avi_image_context.rgbFrame);  //????? realloc buffers later?
    av_packet_unref(avi_image_context.pPacket);
    return -1;
}

//int avi_get_next_frame(struct pixel_buffer **pixbuf)
//{
//    return avi_pixbuf(NULL, pixbuf);
//}

double avi_get_framerate(void)
{
    return avi_image_context.framerate;
}

int avi_get_width(void)
{
    return avi_image_context.pFrame->width;
}

int avi_get_height(void)
{
    return avi_image_context.pFrame->height;
}

int avi_get_next_frame(struct pixel_buffer **pixbuf)
{
    int response = 0;
    start:
    while ((response = av_read_frame(avi_image_context.pFormatContext, avi_image_context.pPacket)) >= 0)
    {
        // if it's the video stream
        if (avi_image_context.pPacket->stream_index == avi_image_context.video_stream_index)
        {
            response = avcodec_send_packet(avi_image_context.pCodecContext, avi_image_context.pPacket);
            if (response < 0)
            {
                break;
                goto error_pix_fmt;
            }
            else
            {
                response = avcodec_receive_frame(avi_image_context.pCodecContext, avi_image_context.pFrame);
                if (response >= 0)
                {
                    // if (fill_rgb_frame(avi_image_context.pFrame) < 0)
                    // {
                    //     break;
                    //     goto error_pix_fmt;
                    // }
                    fill_rgb_frame();
                    copy_to_user((*pixbuf)->data, 0, avi_image_context.rgbFrame->data[0],
                                 4 * avi_image_context.rgbFrame->width * avi_image_context.rgbFrame->height);
                    //*pixbuf = pixbuf_get(*pixbuf);
                    //*pixbuf = avi_image_context.pixbuf;
                    //*pixbuf = pixbuf_get(avi_image_context.pixbuf);
                    //av_frame_unref(avi_image_context.pFrame);
                    //av_frame_unref(avi_image_context.rgbFrame); //????? realloc buffers later?
                    av_packet_unref(avi_image_context.pPacket);
                    break;
                }
            }
        }
        av_packet_unref(avi_image_context.pPacket);
    }
    if (response == AVERROR_EOF) // EOF was reached, need to go to start of video
    {
        // avio_seek(avi_image_context.pFormatContext->pb, 0, SEEK_SET);
        if (avformat_seek_file(avi_image_context.pFormatContext, avi_image_context.video_stream_index, 0, 0,
                               avi_image_context.pFormatContext->streams[avi_image_context.video_stream_index]->duration, 0) >= 0)
        {
            goto start;
        }
    }
    return response < 0 ? response : 0;

    error_pix_fmt:
    av_frame_unref(avi_image_context.pFrame);
    //av_frame_unref(avi_image_context.rgbFrame);  //????? realloc buffers later?
    av_packet_unref(avi_image_context.pPacket);
    return -1;
}

/** AVI image type */
struct image_type avi_image_type __image_type(PROBE_NORMAL) = {
    .name = "AVI",
    .probe = avi_image_probe,
    .pixbuf = avi_pixbuf,
};