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

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <xcb/randr.h>
#include "lswm.h"

static void randr_create_outputs(xcb_randr_output_t *, int, xcb_timestamp_t);
static void		 monitor_create_randr_monitor(xcb_randr_output_t *,
				struct rectangle, const char *);
static struct monitor	*monitor_find_by_id(xcb_randr_output_t);
static struct monitor	*monitor_find_by_name(const char *);
static struct monitor	*monitor_find_duplicate(xcb_randr_output_t,
				const char *);

void
randr_maybe_init(void)
{
	struct rectangle				 size;
	xcb_randr_get_screen_resources_current_reply_t	*res;
	xcb_randr_output_t				*outputs;
	xcb_randr_get_screen_resources_current_cookie_t	 res_ck;
	const xcb_query_extension_reply_t		*ext;
	int						 len;

	/* Check to see if we have an RandR extension defined, and if not,
	 * assume a single screen.  Note that not acquiring screen resource,
	 * even if RandR present is also a failing, and we assume a single
	 * screen also.
	 */
	ext = xcb_get_extension_data(dpy, &xcb_randr_id);

	res_ck = xcb_randr_get_screen_resources_current(dpy,
			current_screen->root);
	res = xcb_randr_get_screen_resources_current_reply(dpy, res_ck, NULL);

	if (res == NULL || !ext->present)
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

	/* If we end up here, then we should select for RandR events on the
	 * root window and react accordingly.
	 */
	randr_start = ext->first_event;
	log_msg("RandR:  randr_start is %d", randr_start);

	xcb_randr_select_input(dpy, current_screen->root,
			XCB_RANDR_NOTIFY_MASK_CRTC_CHANGE |
			XCB_RANDR_NOTIFY_MASK_OUTPUT_CHANGE |
			XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE |
			XCB_RANDR_NOTIFY_MASK_OUTPUT_PROPERTY);
	xcb_flush(dpy);

	return;

single_screen:
	/* Use the root window. */
	size.x = 0;
	size.y = 0;
	size.w = current_screen->width_in_pixels;
	size.h = current_screen->height_in_pixels;

	monitor_create_randr_monitor(NULL, size, "monitor");
}

static void
monitor_create_randr_monitor(xcb_randr_output_t *id, struct rectangle info,
			     const char *name)
{
	struct monitor	*new;

	new = xmalloc(sizeof *new);
	memset(new, 0, sizeof *new);

	TAILQ_INIT(&new->desktops_q);

	new->id = (id == NULL) ? XCB_NONE : *id;
	new->name = strdup(name);
	new->changed = false;
	memcpy(&new->size, &info, sizeof(struct rectangle));

	if (TAILQ_EMPTY(&monitor_q))
		TAILQ_INSERT_HEAD(&monitor_q, new, entry);
	else
		TAILQ_INSERT_TAIL(&monitor_q, new, entry);
}

/*
 * Walk through all the RANDR outputs (number of outputs == len) there
 * was at time timestamp.
 */
static void
randr_create_outputs(xcb_randr_output_t *outputs, int len,
		     xcb_timestamp_t timestamp)
{
	struct rectangle			 size;
	struct monitor				*m;
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
	for (i = 0; i < len; i++)
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

		log_msg("RandR:  CRTC: at %d, %d, size: %dx%d",
			crtc->x, crtc->y, crtc->width, crtc->height);

		size.x = crtc->x;
		size.y = crtc->y;
		size.w = crtc->width;
		size.h = crtc->height;

		if ((m = monitor_find_duplicate(outputs[i], name)) == NULL)
			monitor_create_randr_monitor(&outputs[i], size, name);
		else {
			m->size.x = size.x;
			m->size.y = size.y;
			m->size.w = size.w;
			m->size.h = size.h;

			m->changed = true;
		}

		free(name);
		free(output);
	}

}

static struct monitor *
monitor_find_by_id(xcb_randr_output_t id)
{
	struct monitor	*mon;

	TAILQ_FOREACH(mon, &monitor_q, entry) {
		if (mon->id == id)
			break;
	}

	return (mon);
}

/* Given x/y coordinates on a screen, find which monitor that position
 * is contained within.
 */
struct monitor *
monitor_at_xy(int x, int y)
{
	struct monitor		*m = NULL;
	struct rectangle	 r;

	TAILQ_FOREACH(m, &monitor_q, entry) {
		r = m->size;

		if ((x >= r.x && x <= r.x + r.w) &&
		    (y >= r.y && y <= r.y + r.h))
			break;
	}
	return (m);
}

static struct monitor *
monitor_find_by_name(const char *name)
{
	struct monitor	*mon;

	TAILQ_FOREACH(mon, &monitor_q, entry) {
		if (strcmp(mon->name, name) == 0)
			break;
	}

	return (mon);
}

static struct monitor *
monitor_find_duplicate(xcb_randr_output_t id, const char *name)
{
	struct monitor	*m2, *m_find;

	TAILQ_FOREACH(m2, &monitor_q, entry) {
		if (m2 == NULL)
			continue;

		if ((m_find = monitor_find_by_id(id)) == NULL ||
		    ((m_find = monitor_find_by_name(name)) == NULL))
				continue;

		log_msg("Found a duplicate: %s -> %s", m2->name,
			m_find->name);

		return (m2);
	}

	return (NULL);
}
