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

extern struct image_type avi_image_type __image_type ( PROBE_NORMAL );

#endif /* _IPXE_AVI_H */