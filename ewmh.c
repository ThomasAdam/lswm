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

/* Messages for EWMH. */

#include <string.h>
#include <xcb/xcb_atom.h>
#include "lswm.h"

/* Client-specific atoms, which aren't initialised by the EWMH API. */
struct x_atoms	 cwmh_atoms[] = {
	{ "WM_PROTOCOLS",			XCB_ATOM_NONE },
	{ "_MOTIF_WM_HINTS",			XCB_ATOM_NONE },
	{ "WM_STATE",				XCB_ATOM_NONE },
	{ "WM_CHANGE_STATE",			XCB_ATOM_NONE },
	{ "WM_DELETE_WINDOW",			XCB_ATOM_NONE },
	{ "WM_TAKE_FOCUS",			XCB_ATOM_NONE }
};

static xcb_atom_t	 atom_from_string(const char *);

static xcb_atom_t
atom_from_string(const char *atom_name)
{
	xcb_intern_atom_cookie_t	 c;
	xcb_intern_atom_reply_t		*r;
	xcb_atom_t			 atom = XCB_ATOM_NONE;

	c = xcb_intern_atom(dpy, 0, strlen(atom_name), atom_name);
	r = xcb_intern_atom_reply(dpy, c, NULL);
	if (r) {
		atom = r->atom;
		free(r);
	}
	return (atom);
}

xcb_atom_t
x_atom_by_name(const char *name)
{
	int		 i;
	xcb_atom_t	 atom = XCB_ATOM_NONE;

	for (i = 0; i < nitems(cwmh_atoms); i++) {
		if (strcmp(name, cwmh_atoms[i].name) == 0) {
			atom = cwmh_atoms[i].atom;
			break;
		}
	}
	return (atom);
}

void
x_atoms_init(void)
{
	int		 i;
	xcb_atom_t	 atom, utf8_string;
	xcb_window_t	 child_win;

	ewmh = xmalloc(sizeof(xcb_ewmh_connection_t));

	if (xcb_ewmh_init_atoms_replies(ewmh,
	    xcb_ewmh_init_atoms(dpy, ewmh), NULL) == 0)
		log_fatal("Unable to create EWMH atoms");

	xcb_ewmh_set_wm_name(ewmh, current_screen->root, 4, "lswm");

	xcb_atom_t ewmh_atoms_supported[] = {
		ewmh->_NET_WM_DESKTOP,
		ewmh->_NET_SUPPORTING_WM_CHECK,
		ewmh->_NET_SUPPORTED,
		ewmh->_NET_CURRENT_DESKTOP,
		ewmh->_NET_ACTIVE_WINDOW,
		ewmh->_NET_DESKTOP_NAMES,
		ewmh->_NET_NUMBER_OF_DESKTOPS,
		ewmh->_NET_CLIENT_LIST,
		ewmh->_NET_WM_STATE,
		ewmh->_NET_WM_STATE_FULLSCREEN,
		ewmh->_NET_WM_STATE_DEMANDS_ATTENTION,
		ewmh->_NET_WM_WINDOW_TYPE,
		ewmh->_NET_WM_WINDOW_TYPE_UTILITY,
		ewmh->_NET_WM_WINDOW_TYPE_TOOLBAR,
		ewmh->_NET_WM_WINDOW_TYPE_DIALOG,
		ewmh->_NET_WM_WINDOW_TYPE_DOCK,
		ewmh->_NET_WM_WINDOW_TYPE_NOTIFICATION,
		ewmh->UTF8_STRING
	};

	/* Initialise all client atoms as well. */
	for (i = 0; i < nitems(cwmh_atoms); i++)
		cwmh_atoms[i].atom = atom_from_string(cwmh_atoms[i].name);

	/* Create a window on the root window to select atoms on.  According
	 * to the EWMH spec, such a window has to be present for as long as
	 * the WM is running.
	 */
	child_win = xcb_generate_id(dpy);
	xcb_create_window(dpy, XCB_COPY_FROM_PARENT, child_win,
			  current_screen->root,
			  0, 0, 1, 1,
			  0,
			  XCB_WINDOW_CLASS_INPUT_ONLY,
			  XCB_COPY_FROM_PARENT,
			  0,
			  NULL);

	xcb_change_property(dpy, XCB_PROP_MODE_REPLACE, child_win,
			    ewmh->_NET_SUPPORTING_WM_CHECK, XCB_ATOM_WINDOW,
			    32, 1, &child_win);

	xcb_change_property(dpy, XCB_PROP_MODE_REPLACE, child_win,
			    ewmh->_NET_WM_NAME, ewmh->UTF8_STRING, 8,
			    strlen(PROGNAME), PROGNAME);

	xcb_change_property(dpy, XCB_PROP_MODE_REPLACE, current_screen->root,
			    ewmh->_NET_SUPPORTED, XCB_ATOM_ATOM, 32, 18,
			    ewmh_atoms_supported);

	/* Tell XCB about the atoms we support. */
	xcb_ewmh_set_supported(ewmh, default_screen,
	    nitems(ewmh_atoms_supported), ewmh_atoms_supported);
}

#warning "ewmh_set_active_window() needs implementing"
void
ewmh_set_active_window(void)
{
	return;
}


#warning "ewmh_set_no_of_desktops() needs implementing"
void
ewmh_set_no_of_desktops(void)
{
	return;
}
