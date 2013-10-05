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

#include <stdbool.h>
#include <string.h>
#include "lswm.h"

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
client_manage_client(struct client *c, bool needs_map)
{
	struct geometry			 c_geom;
	struct rectangle		 r;
	struct monitor			*m;
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

	free(geom_r);

	/* Add this to the set of geometries. */
	if (TAILQ_EMPTY(&c->geometries_q))
		TAILQ_INSERT_HEAD(&c->geometries_q, &c_geom, entry);
	else
		TAILQ_INSERT_TAIL(&c->geometries_q, &c_geom, entry);

	/* Add the client to our list.  Its position will dictate which
	 * desktop and hence monitor it is on.
	 */
	if ((m = monitor_at_xy(r.x, r.y)) == NULL)
		log_fatal("No monitor found at x: %d, y: %d", r.x, r.y);

	/* XXX: How do we handle clients destined for different
	 * monitors/desks?  For now, use the last active desktop.  Can use
	 * _NET_WM_DESKTOP
	 */
	if (m->active_desktop == NULL)
		m->active_desktop = TAILQ_FIRST(&m->desktops_q);

	if (TAILQ_EMPTY(&m->active_desktop->clients_q))
		TAILQ_INSERT_HEAD(&m->active_desktop->clients_q, c, entry);
	else
		TAILQ_INSERT_TAIL(&m->active_desktop->clients_q, c, entry);

	/* Borders. */
	client_set_bw(c, &c_geom);
	client_set_border_colour(c, 0);
}

void
client_set_bw(struct client *c, struct geometry *g)
{
	uint32_t values[1];
	uint32_t mask = 0;

	values[0] = g->bw;

	mask |= XCB_CONFIG_WINDOW_BORDER_WIDTH;
	xcb_configure_window(dpy, c->win, mask, &values[0]);
	xcb_flush(dpy);
}

uint32_t client_get_colour(const char *colour)
{
	xcb_alloc_named_color_reply_t	*col_r;
	xcb_colormap_t			 cmap;
	xcb_generic_error_t		*error;
	xcb_alloc_named_color_cookie_t	 col_ck;
	uint32_t			 pixel;

	cmap = current_screen->default_colormap;
	col_ck = xcb_alloc_named_color(dpy, cmap, strlen(colour), colour);
	col_r = xcb_alloc_named_color_reply(dpy, col_ck, &error);
	if (error != NULL)
		log_fatal("Couldn't get pixel value for colour %s", colour);

	pixel = col_r->pixel;
	free(col_r);

	return (pixel);
}

void
client_set_border_colour(struct client *c, int type)
{
	uint32_t	 values[1];

	values[0] = type == FOCUS_BORDER                ?
		client_get_colour(CONFIG_FOCUS_COLOUR)	:
		client_get_colour(CONFIG_NFOCUS_COLOUR);

	xcb_change_window_attributes(dpy, c->win, XCB_CW_BORDER_PIXEL,
			             values);
}

void
client_scan_windows(void)
{
	xcb_query_tree_reply_t			*reply;
	xcb_get_window_attributes_reply_t	*attr;
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

			client_manage_client(client, true);

		}
		free(attr);
	}
	free(reply);
	xcb_flush(dpy);
}
