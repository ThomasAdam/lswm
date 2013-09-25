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

/* Routines for handling clients. */

#include <string.h>
#include "lswm.h"

#define CONFIG_BW 4

struct client *
client_create(xcb_window_t win)
{
	struct client	*new;

	new = xmalloc(sizeof *new);
	memset(new, 0, sizeof *new);

	TAILQ_INIT(&new->geometries_q);

	new->win = win;

	return (new);
}

void
client_manage_client(struct client *c)
{
	struct geometry			 c_geom;
	struct rectangle		 r;
	xcb_get_geometry_reply_t	*geom_r;

	if (c == NULL)
		log_fatal("Tried to manage a NULL client");

	/* Get the window's geometry. */
	geom_r = xcb_get_geometry_reply(dpy,
		xcb_get_geometry(dpy, c->win), NULL);

	if (geom_r == NULL)
		log_fatal("Window '0x%x' has no geometry", c->win);
	log_msg("Window '0x%x' has geom: %ux%u+%d+%d",
		c->win, geom_r->width, geom_r->height, geom_r->x, geom_r->y);

	r.x = geom_r->x;
	r.y = geom_r->y;
	r.w = geom_r->width;
	r.h = geom_r->height;

	memcpy(&c_geom.coords, &r, sizeof(struct rectangle));
	c_geom.bw = CONFIG_BW;

	/* Add this to the set of geometries. */
	if (TAILQ_EMPTY(&c->geometries_q))
		TAILQ_INSERT_HEAD(&c->geometries_q, &c_geom, entry);
	else
		TAILQ_INSERT_TAIL(&c->geometries_q, &c_geom, entry);

	/* Add the client to our list.  Its position will dictate which
	 * desktop and hence monitor it is on.
	 */
	/* XXX: How do we handle clients destined for different
	 * monitors/desks?
	 */

	free(geom_r);
}

void
client_scan_windows(void)
{
	xcb_query_tree_reply_t			*reply;
	xcb_get_window_attributes_reply_t 	*attr;
	xcb_window_t				*children;
	int					 i;
	int					 len;
	struct client				*client;

	/* Get all children. */
	reply = xcb_query_tree_reply(dpy,
	xcb_query_tree(dpy, current_screen->root), 0);
	if (reply == NULL)
		log_fatal("Couldn't get a list of windows");

	len = xcb_query_tree_children_length(reply);
	children = xcb_query_tree_children(reply);

	/* Set up all windows on this root. */
	for (i = 0; i < len; i ++)
	{
		attr = xcb_get_window_attributes_reply(dpy,
				xcb_get_window_attributes(dpy, children[i]),
				NULL);

		if (attr == NULL) {
			log_msg("Couldn't get attributes for window %d",
				children[i]);
			continue;
		}

		if (!attr->override_redirect &&
		    attr->map_state == XCB_MAP_STATE_VIEWABLE) {
			if ((client = client_create(children[i])) == NULL)
				log_fatal("Couldn't handle creating client");

			client_manage_client(client);

		}
		free(attr);
	}
	free(reply);
	xcb_flush(dpy);
}
