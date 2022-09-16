#ifndef _IPXE_AVI_H
#define _IPXE_AVI_H

/** @file
 *
 * Audio Video Interlace (AVI) format
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <ipxe/image.h>
#include <ffmpeg/libavcodec/avcodec.h>
#include <ffmpeg/libavformat/avformat.h>
#include <ffmpeg/libswscale/swscale.h>
#include <stdio.h>
#include <stdint.h>
#include <ipxe/pixbuf.h>

extern struct image_type avi_image_type __image_type ( PROBE_NORMAL );
int avi_get_next_frame(struct pixel_buffer * pixbuf); //pixel buffer to be filled with avcodec
double avi_get_framerate(void);
void avi_image_context_clear(void);
int avi_get_height(void);
int avi_get_width(void);
#endif /* _IPXE_AVI_H */