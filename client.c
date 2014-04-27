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

/* The currently focused client. */
static struct client	*cur_client;

/* Forward declarations. */
static void	 client_focus_model(struct client *);
static void	 client_handle_initial_atoms(struct client *);

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

struct client *
client_get_current(void)
{
	return (cur_client);
}

struct client *
client_find_by_window(xcb_window_t win)
{
	struct client	*c, *find_client = NULL;
	struct monitor	*m;
	struct desktop	*d;

	TAILQ_FOREACH(m, &monitor_q, entry) {
	       TAILQ_FOREACH(d, &m->desktops_q, entry) {
		       TAILQ_FOREACH(c, &d->clients_q, entry) {
			       if (c->win == win) {
				       find_client = c;
				       break;
			       }
		       }
	       }
	}
	return (find_client);
}

static void
client_focus_model(struct client *c)
{
	/* XXX
	 * We should be respecting PASSIVE/{GLOBALLY,LOCALLY}_ACTIVE input
	 * here---but for now, just set the fact that the requested windows
	 * wants input selection regardless; when the client is made active,
	 * this hint is applied.
	 */
	if (c->xwmh.input && c->xwmh.flags & XCB_ICCCM_WM_HINT_INPUT)
		c->flags |= CLIENT_INPUT_FOCUS;
}

static void
client_handle_initial_atoms(struct client *c)
{
	/* This aggregates together all of the client Atoms we wish to check for
	 * when a window is initially mapped.  Each of these routines are also
	 * responded to by events.
	 */
	client_wm_hints(c);
	client_wm_protocols(c);
	client_mwm_hints(c);
	client_get_size_hints(c);
	client_set_name(c);
}

void
client_set_name(struct client *c)
{
	xcb_generic_error_t			*error;
	xcb_get_property_cookie_t		 p_cookie;
	xcb_get_property_reply_t		*r = NULL;

	p_cookie = xcb_get_property(dpy, 0, c->win, ewmh->_NET_WM_NAME,
				    XCB_GET_PROPERTY_TYPE_ANY, 0, UINT_MAX);

	if (error) {
		log_msg("Couldn't get client's NET_WM_NAME");
		log_msg("    Trying with WM_NAME instead...");

		free(r);
		p_cookie = xcb_get_property(dpy, 0, c->win, XCB_ATOM_WM_NAME,
			     XCB_GET_PROPERTY_TYPE_ANY, 0, UINT_MAX);

		r = xcb_get_property_reply(dpy, p_cookie, &error);
	}

	if (r != NULL && r->type != XCB_NONE && r->length > 0) {
		free(c->name);
		c->name = strndup(xcb_get_property_value(r),
			    xcb_get_property_value_length(r));
	} else
		c->name = xstrdup("");

	if (c->name == NULL) {
		/* xstrdup() would have croaked for us, this is in the case of
		 * strndup() failing.
		 */
		log_fatal("Couldn't do anything for c->name:  NULL");
	}
	free(r);
	log_msg("Got client name of:  <<%s>>", c->name);
}

void
client_wm_protocols(struct client *c)
{
	xcb_icccm_get_wm_protocols_reply_t	 protocols;
	xcb_atom_t				 wm_protocols = XCB_ATOM_NONE;
	xcb_atom_t				 p_atom = XCB_ATOM_NONE;
	int					 reply = 0;
	u_int					 i;

	/* Check the atom exists. */
	if ((wm_protocols = ewmh->WM_PROTOCOLS) == XCB_ATOM_NONE)
		return;

	reply = xcb_icccm_get_wm_protocols_reply(dpy,
			xcb_icccm_get_wm_protocols(dpy, c->win, wm_protocols),
			    &protocols, NULL);

	if (reply) {
		/* Fill out the client flags with the things we got back. */
		for (i = 0; i < protocols.atoms_len; i++) {
			p_atom = protocols.atoms[i];

			if (p_atom == x_atom_by_name("WM_DELETE_WINDOW"))
				c->flags |= CLIENT_DELETE_WINDOW;

			if (p_atom == x_atom_by_name("WM_TAKE_FOCUS"))
				c->flags |= CLIENT_INPUT_FOCUS;
		}
		xcb_icccm_get_wm_protocols_reply_wipe(&protocols);
	}
}

void
client_wm_hints(struct client *c)
{
	int reply = xcb_icccm_get_wm_hints_reply(
		dpy, xcb_icccm_get_wm_hints(dpy, c->win),
		&c->xwmh, NULL);

	if (reply == 0)
		return;

	client_focus_model(c);
}

#warning "client_mwm_hints() needs implementing..."
void
client_mwm_hints(struct client *c)
{
	return;
}

void
client_get_size_hints(struct client *c)
{
	xcb_size_hints_t	 shints;
	int			 reply = 0;

	reply = xcb_icccm_get_wm_normal_hints_reply(dpy,
		    xcb_icccm_get_wm_normal_hints(dpy, c->win), &shints, NULL);

	if (reply == 0)
		return;

	/* Beat up our copy of the various hints we're tracking.  Note that the
	 * ICCCM is quite clear about how these hints are to be used.
	 */
	switch (shints.flags) {
	case XCB_ICCCM_SIZE_HINT_P_MIN_SIZE:
		c->hints.min_w = shints.min_width;
		c->hints.min_h = shints.min_height;
		break;
	case XCB_ICCCM_SIZE_HINT_P_MAX_SIZE:
		c->hints.max_w = shints.max_width;
		c->hints.max_h = shints.max_height;
		break;
	case XCB_ICCCM_SIZE_HINT_P_RESIZE_INC:
		c->hints.inc_w = shints.width_inc;
		c->hints.inc_h = shints.height_inc;
		break;
	case XCB_ICCCM_SIZE_HINT_BASE_SIZE:
		c->hints.base_w = shints.base_width;
		c->hints.base_h = shints.base_height;
		break;
	case XCB_ICCCM_SIZE_HINT_P_ASPECT:
		/* TODO. */
		break;
	case XCB_ICCCM_SIZE_HINT_P_WIN_GRAVITY:
		/* TODO */
		break;
	default:
		break;
	}

	/* At most, the amount of resize increment is 1 (ICCCM!) */
	c->hints.inc_w = MAX(1, c->hints.inc_w);
	c->hints.inc_h = MAX(1, c->hints.inc_h);
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

	/* Set the application's Class/resource hint here---applications at this
	 * point are still in the Withdrawn state, and might still have changed
	 * their XClassHint.
	 */
	(void)xcb_icccm_get_wm_class_reply(dpy,
	    xcb_icccm_get_wm_class(dpy, c->win), &c->xch, NULL);

	/* Check the client for any Atom hints. */
	client_handle_initial_atoms(c);

	/* Borders. */
	client_set_bw(c, &c_geom);
	client_set_border_colour(c, 0);

	grab_all_bindings(c->win);
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
