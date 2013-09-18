/*
 * Copyright (c) 2013 Thomas Adam <thomas@xteddy.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* Routines for handling XRandR extensions. */

#include <time.h>
#include <string.h>
#include <xcb/randr.h>
#include "lswm.h"

static void randr_create_outputs(xcb_randr_output_t *, int, xcb_timestamp_t);
static struct monitor	*monitor_create_randr_monitor(xcb_randr_output_t, 
					xcb_randr_get_crtc_info_reply_t *,
					const char *);
static struct monitor	*monitor_find_by_id(xcb_randr_output_t);
static struct monitor	*monitor_find_by_name(const char *);

void
randr_maybe_init(void)
{
	xcb_randr_get_screen_resources_current_reply_t	*res;
	xcb_randr_output_t				*outputs;
	xcb_randr_get_screen_resources_current_cookie_t	 res_ck;
	int						 len;

	res_ck = xcb_randr_get_screen_resources_current(dpy,
			current_screen->root);
	res = xcb_randr_get_screen_resources_current_reply(dpy, res_ck, NULL);

	if (res == NULL)
	{
		log_msg("No RANDR extension available.  "
			"Falling back to single screen.");

		goto single_screen;
	}

	len = xcb_randr_get_screen_resources_current_outputs_length(res);
	outputs = xcb_randr_get_screen_resources_current_outputs(res);

	log_msg("RandR:  found %d outputs", len);

	if (len > 0)
		randr_create_outputs(outputs, len, res->config_timestamp);
	free(res);

single_screen:
	log_fatal("Needs implementing!");
}

static struct monitor*
monitor_create_randr_monitor(xcb_randr_output_t id,
			     xcb_randr_get_crtc_info_reply_t *info,
			     const char *name)
{
	struct monitor	*new;

	new = xmalloc(sizeof *new);
	memset(new, 0, sizeof *new);

	new->id = id;
	new->name = strdup(name);
	new->size.x = info->x;
	new->size.y = info->y;
	new->size.w = info->width;
	new->size.h = info->height;

	/* TODO: Add to monitor queue. */

	return (new);
}

/*
 * Walk through all the RANDR outputs (number of outputs == len) there
 * was at time timestamp.
 */
static void
randr_create_outputs(xcb_randr_output_t *outputs, int len,
		     xcb_timestamp_t timestamp)
{
	char					*name;
	xcb_randr_get_crtc_info_cookie_t	 crtc_info_ck;
	xcb_randr_get_crtc_info_reply_t		*crtc = NULL;
	xcb_randr_get_output_info_reply_t	*output;
	xcb_randr_get_output_info_cookie_t	 info_ck[len];
	int					 i;

	for (i = 0; i < len; i++)
	{
		info_ck[i] = xcb_randr_get_output_info(dpy, outputs[i],
				timestamp);
	}

	/* Loop through all outputs. */
	for (i = 0; i < len; i ++)
	{
		output = xcb_randr_get_output_info_reply(dpy, info_ck[i], NULL);

		if (output == NULL)
			continue;

		/* If the output width/height is zero, then treat this output
		 * as disabled, and move on to the next.
		 */
		if (output->mm_width == 0 && output->mm_height == 0)
			continue;

		xasprintf(&name, "%.*s",
			xcb_randr_get_output_info_name_length(output),
			xcb_randr_get_output_info_name(output));

		log_msg("RandR:  Name: %s", name);
		log_msg("RandR:  ID:   %d" , outputs[i]);
		log_msg("RandR:  Size: %d x %d mm",
			output->mm_width, output->mm_height);

		if (output->crtc == XCB_NONE)
			continue;

		crtc_info_ck = xcb_randr_get_crtc_info(dpy, output->crtc,
				timestamp);
		crtc = xcb_randr_get_crtc_info_reply(dpy, crtc_info_ck,
				NULL);
		if (crtc == NULL)
			return;

		if (crtc->x == 0 && crtc->y == 0)
			continue;

		log_msg("RandR:  CRTC: at %d, %d, size: %dx%d",
			crtc->x, crtc->y, crtc->width, crtc->height);

		free(output);
	}
}

