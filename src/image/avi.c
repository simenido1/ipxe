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

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <ipxe/avi.h>
#include <string.h>

static int image_size = 0;
static int image_internal_position = 0;
static intptr_t image_start = 0;

// static int my_read (void *opaque, unsigned char *buf, int size)
// {
// aviobuffercontext* op = (aviobuffercontext*) opaque;
// int len = size;
// if (op->pos + size > Op->totalsize)
// {
// Len = op->totalsize-op->pos;
// }
// memcpy (buf, Op->ptr + Op->pos, Len);
// if (Op->pos + len >= op->realsize)
// Op->realsize + = Len;

// Op->pos + = Len;

// return Len;
// }
//Read buffer for avio_alloc_context
int read_buffer(void *opaque, uint8_t *buf, int buf_size){
	// if(!feof(fp_open)){
	// 	int true_size=fread(buf,1,buf_size,fp_open);
	// 	return true_size;
	// }else{
	// 	return -1;
	// }
    //printf("%p\n", opaque);
    //intptr_t actual_ptr = image_start + image_internal_position;
    if (image_internal_position >= image_size)
    {
        //printf("read_buffer_eof\n");
        return AVERROR_EOF; //EOF
    }
    int actual_bytes_read = buf_size;
    if (image_internal_position + buf_size > image_size)
    {
        // printf("read_buffer_eof\n");
        // return AVERROR_EOF; //EOF
        actual_bytes_read = image_size - image_internal_position;
    }
    // if (actual_ptr + buf_size <= image_end)
    // {
    //     //printf("%s\n", buf);
    //     //printf("%d\n", buf_size);
    //     actual_bytes_read = buf_size;

    // }
    // else if (actual_ptr < image_end)
    // {
    //     //printf("avi 48\n");
    //     actual_bytes_read = image_end - actual_ptr;
    // }
    //memcpy(buf, image_start + image_internal_position, actual_bytes_read);
    copy_from_user(buf, image_start, image_internal_position, actual_bytes_read);
    image_internal_position += actual_bytes_read;
    //printf("actual_bytes_read=%d, internal_position=%d, size=%d\n", actual_bytes_read, image_internal_position, image_size);
    return actual_bytes_read;
}

// int seek_buffer(void *opaque, int64_t offset, int whence)
// {
//     int start_pos = 0;
//     switch (whence)
//     {
//         case SEEK_SET:
//             start_pos = 0;
//             break;
//         case SEEK_CUR:
//             start_pos = image_internal_position;
//             break;
//         case SEEK_END:
//             start_pos = image_size;
//             break;
//         case AVSEEK_SIZE:
//             return image_size;
//             break;
//         default:
//             printf("seek_buffer incorrect whence:%d\n", whence);
//             return -1;
//             break;
//     }
//     int final_pos = start_pos + offset;
//     if (final_pos < 0) 
//     {
//         //final_ptr = opaque;
//         printf("final_pos=%d\n", final_pos);
//         return -1;
//     }
//     if (final_pos > image_size)
//     {
//         //final_ptr = image_end;
//         printf("final_pos > size\n");
//         return -1;
//     }
//     image_internal_position = final_pos;
//     return 0;
// }

/**
 * Probe AVI image
 *
 * @v image		AVI image
 * @ret rc		Return status code
 */
static int avi_image_probe ( struct image *image ) {
    int ret = 0;
    image_size = image->len;
    image_internal_position = 0;
    image_start = image->data;
    AVFormatContext *pFormatContext = avformat_alloc_context();
    if (!pFormatContext)
    {
        printf("ERROR! Can't allocate AVFormatContext\n");
        return -1;
    }
    //setup pb field with custom io stream.
    const int avio_buffer_size = 8192 /* + 64 */;
    unsigned char* avio_buffer = (unsigned char*)av_malloc(avio_buffer_size);
    AVIOContext * avio = avio_alloc_context(avio_buffer, avio_buffer_size, 0, NULL, read_buffer, NULL, NULL);
    pFormatContext->pb = avio;
    //pFormatContext->flags |= AVFMT_FLAG_CUSTOM_IO;
    if (!pFormatContext->pb) {
        printf("ERROR! Can't allocate AVIOContext\n");
        return -1;
    }
    if ((ret = avformat_open_input(&pFormatContext, NULL, NULL, NULL)) != 0) {
        printf("ERROR! Can't open file with avformat_open_input(). Error code: %d\n", ret);
        return -1;
    }
    //printf("internal_position=%d\n", image_internal_position);
    if (avformat_find_stream_info(pFormatContext, NULL) < 0) {
        printf("ERROR! Can't get the stream info");
        return -1;
    }
    //printf("internal_position=%d\n", image_internal_position);
    // Initialize the codec paramters for subsequent usage. 
    AVCodec* pCodec = NULL;
    AVCodecParameters* pCodecParameters = NULL;
    int videoStreamIndex = -1;

for (int i = 0; i < pFormatContext->nb_streams; i++)
    {
//        AVCodecParameters *pLocalCodecParameters = NULL;
        // Read the codec parameters corresponding to each stream.
        AVCodecParameters * pLocalCodecParameters = pFormatContext->streams[i]->codecpar;

        //const AVCodec *pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);
    //    if (!avcodec_find_decoder(pLocalCodecParameters->codec_id))
    //    {
    //        printf("Error! Can't find codec for %s, %s\n",
    //               av_get_media_type_string(pLocalCodecParameters->codec_type),
    //               avcodec_get_name(pLocalCodecParameters->codec_id));
    //        //return -1;
    //    }
        if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            if (videoStreamIndex == -1)
            {
                videoStreamIndex = i;
                pCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);
                if (!pCodec)
                {
                    printf("Error! Can't find codec for %s, %s\n",
                           av_get_media_type_string(pLocalCodecParameters->codec_type),
                           avcodec_get_name(pLocalCodecParameters->codec_id));
                    return -1;
                }
//                codec_type = av_get_media_type_string(pLocalCodecParameters->codec_type);
//                codec_name = avcodec_get_name(pLocalCodecParameters->codec_id);
                pCodecParameters = pLocalCodecParameters;
            }
        }
    }

    AVCodecContext* pCodecContext = avcodec_alloc_context3(pCodec);
    if (pCodecContext == NULL)
    {
        printf("ERROR! Cannot allocate memory for pCodecContext");
        return -1;
    }

    if (avcodec_parameters_to_context(pCodecContext, pCodecParameters) < 0)
    {
        printf("ERROR! failed to copy codec params to codec context\n");
        return -1;
    }

    if (avcodec_open2(pCodecContext, pCodec, NULL) < 0)
    {
        printf("failed to open codec through avcodec_open2\n");
        return -1;
    }
    AVFrame* pFrame = av_frame_alloc();
    if (!pFrame)
    {
        printf("failed to allocate memory for AVFrame\n");
        return -1;
    }

    AVPacket* pPacket = av_packet_alloc();
    if (!pPacket)
    {
        printf("failed to allocate memory for AVPacket\n");
        return -1;
    }
    int indexOfFrame = 0;
    while (av_read_frame(pFormatContext, pPacket) >= 0)
    {
        // if it's the video stream
        if (pPacket->stream_index == videoStreamIndex) {
            int response = avcodec_send_packet(pCodecContext, pPacket);
            // if (response < 0)
            // {
            //     //printf("avi 244, response=%d\n", response);
            //     break;
            // }
            // else
            // {
                while (response >= 0)
                {
                response = avcodec_receive_frame(pCodecContext, pFrame);
                //printf("avi 240, response=%d\n", response);
                if (response >= 0)
                {
                    indexOfFrame++;
                    //SaveToJPEG(pFrame, argv[2], indexOfFrame);
                    printf("data %p, type = %d, quality=%d\n", pFrame->data, pFrame->pict_type, pFrame->quality);
                }
            }
        }
        av_packet_unref(pPacket);
        //Limit the number of output frame to be 5.
        if (indexOfFrame == 5)
        {
            break;
        }

    }
    avio_context_free(&pFormatContext->pb);
    avformat_close_input(&pFormatContext);
    av_frame_free(&pFrame);
    av_packet_free(&pPacket);
    avcodec_free_context(&pCodecContext);
	return 0;
}

/** AVI image type */
struct image_type avi_image_type __image_type ( PROBE_NORMAL ) = {
	.name = "AVI",
	.probe = avi_image_probe,
	// .pixbuf = avi_pixbuf,
};

// //Write File
// int write_buffer(void *opaque, uint8_t *buf, int buf_size){
// 	if(!feof(fp_write)){
// 		int true_size=fwrite(buf,1,buf_size,fp_write);
// 		return true_size;
// 	}else{
// 		return -1;
// 	}
// }

