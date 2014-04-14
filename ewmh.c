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

/*#include <xcb/xcb_ewmh.h>*/
#include <string.h>
#include <xcb/xcb_atom.h>
#include "lswm.h"

struct ewmh_hints	ewmh_atoms[] = {
	{ "_NET_WM_DESKTOP", XCB_ATOM_NONE },
	{ "_NET_SUPPORTING_WM_CHECK", XCB_ATOM_NONE },
	{ "_NET_SUPPORTED", XCB_ATOM_NONE },
	{ "_NET_CURRENT_DESKTOP", XCB_ATOM_NONE },
	{ "_NET_ACTIVE_WINDOW", XCB_ATOM_NONE },
	{ "_NET_DESKTOP_NAMES", XCB_ATOM_NONE },
	{ "_NET_NUMBER_OF_DESKTOPS", XCB_ATOM_NONE },
	{ "_NET_CLIENT_LIST", XCB_ATOM_NONE },
	{ "_NET_WM_STATE", XCB_ATOM_NONE },
	{ "_NET_WM_STATE_FULLSCREEN", XCB_ATOM_NONE },
	{ "_NET_WM_STATE_DEMANDS_ATTENTION", XCB_ATOM_NONE },
	{ "_NET_WM_WINDOW_TYPE", XCB_ATOM_NONE },
	{ "_NET_WM_WINDOW_TYPE_UTILITY", XCB_ATOM_NONE },
	{ "_NET_WM_WINDOW_TYPE_TOOLBAR", XCB_ATOM_NONE },
	{ "_NET_WM_WINDOW_TYPE_DIALOG", XCB_ATOM_NONE },
	{ "_NET_WM_WINDOW_TYPE_DOCK", XCB_ATOM_NONE },
	{ "_NET_WM_WINDOW_TYPE_NOTIFICATION", XCB_ATOM_NONE },
	{ "_UTF8_STRING", XCB_ATOM_NONE }
};

static xcb_atom_t	 ewmh_atom_from_string(const char *);
static xcb_atom_t	 ewmh_atom_by_name(const char *);

static xcb_atom_t
ewmh_atom_from_string(const char *atom_name)
{
	xcb_intern_atom_cookie_t	c;
	xcb_intern_atom_reply_t		*r;
	xcb_atom_t			atom = XCB_ATOM_NONE;

	c = xcb_intern_atom(dpy, 0, strlen(atom_name), atom_name);
	r = xcb_intern_atom_reply(dpy, c, NULL);
	if (r) {
		atom = r->atom;
		free(r);
	}
	return (atom);
}

static xcb_atom_t
ewmh_atom_by_name(const char *name)
{
	int		 i;
	xcb_atom_t	 atom = XCB_ATOM_NONE;

	for (i = 0; i < nitems(ewmh_atoms); i++) {
		if (strcmp(name, ewmh_atoms[i].name) == 0) {
			atom = ewmh_atoms[i].atom;
			break;
		}
	}
	return (atom);
}

void
ewmh_init(void)
{
	int		 i;
	xcb_atom_t	 atom, utf8_string;
	/*
	 * TA:   Until such time that OpenBSD has xcb-util updated, we cannot
	 * use this code; hand-roll for now.
	 */
#if 0
	xcb_ewmh_connection_t	 ewmh;
	if (xcb_ewmh_init_atoms_replies(&ewmh,
	    xcb_ewmh_init_atoms(dpy, &ewmh), NULL) == 0)
		log_fatal("Unable to create EWMH atoms");

	xcb_ewmh_set_wm_name(&ewmh, current_screen->root, 4, "lswm");
#endif

	/* Initialise all declared atoms. */
	for (i = 0; i < nitems(ewmh_atoms); i++) {
		ewmh_atoms[i].atom = ewmh_atom_from_string(ewmh_atoms[i].name);
	}

	/* Create a window on the root window to select atoms on.  According
	 * to the EWMH spec, such a window has to be present for as long as
	 * the WM is running.
	 */
	utf8_string = ewmh_atom_by_name("_UTF8_STRING");

	xcb_window_t child_win = xcb_generate_id(dpy);
	xcb_create_window(dpy, XCB_COPY_FROM_PARENT, child_win,
			  current_screen->root,
			  0, 0, 1, 1,
			  0,
			  XCB_WINDOW_CLASS_INPUT_ONLY,
			  XCB_COPY_FROM_PARENT,
			  0,
			  NULL);

	atom = ewmh_atom_by_name("_NET_SUPPORTING_WM_CHECK");
	xcb_change_property(dpy, XCB_PROP_MODE_REPLACE, child_win,
			    atom, XCB_ATOM_WINDOW,
			    32, 1, &child_win);

	atom = ewmh_atom_by_name("_NET_WM_NAME");
	xcb_change_property(dpy, XCB_PROP_MODE_REPLACE, child_win,
			    atom, utf8_string, 8,
			    strlen(PROGNAME), PROGNAME);

	atom = ewmh_atom_by_name("_NET_SUPPORTING_WM_CHECK");
	xcb_change_property(dpy, XCB_PROP_MODE_REPLACE, current_screen->root,
			    atom, XCB_ATOM_WINDOW,
			    32, 1, &child_win);

	atom = ewmh_atom_by_name("_NET_WM_NAME");
	xcb_change_property(dpy, XCB_PROP_MODE_REPLACE, current_screen->root,
			    atom, utf8_string, 8,
			    strlen(PROGNAME), PROGNAME);

	atom = ewmh_atom_by_name("_NET_SUPPORTED");
	xcb_change_property(dpy, XCB_PROP_MODE_REPLACE, current_screen->root,
			    atom, XCB_ATOM_ATOM, 32, 18,
			    ewmh_atoms);

	/* Tell XCB about the atoms we support. */
	xcb_delete_property(dpy, current_screen->root, atom);
	for (i = 0; i < nitems(ewmh_atoms); i++)
		xcb_change_property(dpy, XCB_PROP_MODE_APPEND,
				    current_screen->root,
				    atom, XCB_ATOM_ATOM, 32, 1,
				    &ewmh_atoms[i].atom);
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
