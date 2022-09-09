/*
 * Copyright (C) 2011 Michael Brown <mbrown@fensystems.co.uk>.
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

/** @file
 *
 * Prompt for keypress
 *
 */
#include <errno.h>
#include <stdio.h>
#include <ipxe/console.h>
#include <usr/prompt.h>
#include <stdlib.h>
#include <string.h>
#include <ipxe/pixbuf.h>
#include <ipxe/image.h>
#include <usr/imgmgmt.h>
#include <ipxe/ansicol.h>
#include <ipxe/avi.h>
#include <unistd.h>
/**
 * Prompt for keypress
 *
 * @v text		Prompt string
 * @v timeout		Timeout period, in ticks (0=indefinite)
 * @v key		Key to wait for (0=any key)
 * @ret rc		Return status code
 *
 * Returns success if the specified key was pressed within the
 * specified timeout period.
 */
extern int vesafb_update_pixbuf(struct pixel_buffer *pixbuf); //function to update background image with new frame (pcbios)
extern int efifb_update_pixbuf(struct pixel_buffer * pixbuf); //function to update background image with new frame (efi)
extern void efifb_finish(void);

#define EFI //(define here EFI or PCBIOS or nothing)
int update_console_framebuffer(struct pixel_buffer * pb)
{
	#ifdef PCBIOS
	return vesafb_update_pixbuf(pb);
	#endif
	#ifdef EFI
	return efifb_update_pixbuf(pb);
	#endif
	return 1;
}

int prompt(const char *text, unsigned long timeout, int key, const char *variable, const char *video)
{
	int key_pressed = -1;
	// struct console_configuration console_config;
	if (video)
	{
		char *command;

		asprintf(&command, "console --picture=%s --keep", video);
		if (system(command) != 0)
		{
            free(command);
            unregister_image(find_image(video));
            goto video_error;
		}
		free(command);

		double framerate = avi_get_framerate();
		if (framerate <= 0)
		{
			// printf("avi framerate error!");
			goto video_error;
		}
		/* Display prompt */
		printf("%s", text);
		unsigned long start = currticks();
		int ret = 0;
		int indexOfFrame = 0;
        struct pixel_buffer *pixbuf = pixbuf_get(alloc_pixbuf(avi_get_width(), avi_get_height()));
		while (((timeout == 0) || (currticks() - start) < timeout) && key_pressed < 0  && ret >= 0)
		{
			if ((ret = avi_get_next_frame(&pixbuf)) != 0)
			{
				printf("avi_get_next_frame error!, frame=%d\n", indexOfFrame);
			}
			else
			{
				indexOfFrame++;
				if ((ret = update_console_framebuffer(pixbuf)) != 0)
				{
					printf("update_console_framebuffer error!, frame=%d\n", indexOfFrame);
				}
			}
			//pixbuf_put(pixbuf);
			// usleep(1000000 / framerate);
			key_pressed = getkey(1000 / framerate);
		}
	#ifdef EFI
	efifb_finish();
	#endif
	unregister_image(find_image(video));
	}
	else
	{
	video_error:
		/* Display prompt */
		printf("%s", text);

		/* Wait for key */
		key_pressed = getkey(timeout);
	}
	/* Clear the prompt line */
	while (*(text++))
		printf("\b \b");

	/* Check for timeout */
	if (key_pressed < 0)
		return -ETIMEDOUT;

	/* Write key value to variable */
	if (variable != NULL && variable != "")
	{
		char *command;
		int rc;
		if ((rc = asprintf(&command, "set %s %i", variable, key_pressed)) < 0)
		{
			return rc;
		}
		if ((rc = system(command)) != 0)
		{ // execute "set" command
			return rc;
		}
		free(command); // free memory used for command
	}
	/* Check for correct key pressed */
	if (key && (key_pressed != key))
		return -ECANCELED;

	return 0;
}
