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
/**
 * Probe AVI image
 *
 * @v image		AVI image
 * @ret rc		Return status code
 */
static int avi_probe ( struct image *image ) {
    AVFormatContext *pFormatContext = avformat_alloc_context();
    if (!pFormatContext)
    {
        printf("ERROR could not allocate memory for Format Context\n");
        return -1;
    }
	avformat_free_context(pFormatContext);
    printf("%s\n", image->name);
	return 0;
}

/** AVI image type */
struct image_type avi_image_type __image_type ( PROBE_NORMAL ) = {
	.name = "AVI",
	.probe = avi_probe,
	// .pixbuf = avi_pixbuf,
};
