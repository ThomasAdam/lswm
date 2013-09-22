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

#include <xcb/xcb_ewmh.h>
#include <string.h>
#include "lswm.h"

void
ewmh_init(void)
{
	xcb_ewmh_connection_t	 ewmh;
	if (xcb_ewmh_init_atoms_replies(&ewmh,
	    xcb_ewmh_init_atoms(dpy, &ewmh), NULL) == 0)
		log_fatal("Unable to create EWMH atoms");

	xcb_ewmh_set_wm_name(&ewmh, current_screen->root, 4, "lswm");

	/* Set the list of atoms. */
	xcb_atom_t	 root_atoms[] = {
		ewmh._NET_WM_DESKTOP,
		ewmh._NET_SUPPORTING_WM_CHECK,
		ewmh._NET_SUPPORTED,
		ewmh._NET_CURRENT_DESKTOP,
		ewmh._NET_ACTIVE_WINDOW,
		ewmh._NET_DESKTOP_NAMES,
		ewmh._NET_NUMBER_OF_DESKTOPS,
		ewmh._NET_CLIENT_LIST,
		ewmh._NET_WM_STATE,
		ewmh._NET_WM_STATE_FULLSCREEN,
		ewmh._NET_WM_STATE_DEMANDS_ATTENTION,
		ewmh._NET_WM_WINDOW_TYPE,
		ewmh._NET_WM_WINDOW_TYPE_UTILITY,
		ewmh._NET_WM_WINDOW_TYPE_TOOLBAR,
		ewmh._NET_WM_WINDOW_TYPE_DIALOG,
		ewmh._NET_WM_WINDOW_TYPE_DOCK,
		ewmh._NET_WM_WINDOW_TYPE_NOTIFICATION
	};

	/* Create a window on the root window to select atoms on.  According
	 * to the EWMH spec, such a window has to be present for as long as
	 * the WM is running.
	 */
	xcb_window_t child_win = xcb_generate_id(dpy);
	xcb_create_window(dpy, XCB_COPY_FROM_PARENT, child_win,
			  current_screen->root,
			  0, 0, 1, 1,
			  0,
			  XCB_WINDOW_CLASS_INPUT_ONLY,
			  XCB_COPY_FROM_PARENT,
			  0,
			  NULL);

	xcb_change_property(dpy, XCB_PROP_MODE_REPLACE, child_win,
			    ewmh._NET_SUPPORTING_WM_CHECK, XCB_ATOM_WINDOW,
			    32, 1, &child_win);
	xcb_change_property(dpy, XCB_PROP_MODE_REPLACE, child_win,
			    ewmh._NET_WM_NAME, ewmh.UTF8_STRING, 8,
			    strlen(PROGNAME), PROGNAME);
	xcb_change_property(dpy, XCB_PROP_MODE_REPLACE, current_screen->root,
			    ewmh._NET_SUPPORTING_WM_CHECK, XCB_ATOM_WINDOW,
			    32, 1, &child_win);
	xcb_change_property(dpy, XCB_PROP_MODE_REPLACE, current_screen->root,
			    ewmh._NET_WM_NAME, ewmh.UTF8_STRING, 8,
			    strlen(PROGNAME), PROGNAME);
	xcb_change_property(dpy, XCB_PROP_MODE_REPLACE, current_screen->root,
			    ewmh._NET_SUPPORTED, XCB_ATOM_ATOM, 32, 18,
			    root_atoms);

	xcb_ewmh_set_supported(&ewmh, default_screen, nitems(root_atoms),
			       root_atoms);
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
