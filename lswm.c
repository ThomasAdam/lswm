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

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <pwd.h>
#include "lswm.h"

static void	 print_usage(void);
static void	 set_display(const char *);
static int	 check_for_existing_wm(void);
static void	 event_loop(void);

char		*cfg_file = NULL;

#define NO_OF_DESKTOPS 10

int main(int argc, char **argv)
{
	int			 opt, i;
	char			*display_opt = NULL;
	xcb_screen_iterator_t	 iter;
	struct monitor		*m;
	struct passwd		*pw;
	struct cmd_q		*cfg_cmd_q;
	char			*name, *home, *causes;
	u_int			 a;

	while ((opt = getopt(argc, argv, "Vd:vf:")) != -1) {
		switch (opt) {
		/* Print the version and exit. */
		case 'V':
			printf("%s\n", VER_STR);
			exit(0);
			break;
		/* Set the DISPLAY we intend to use; overrides environment. */
		case 'd':
			display_opt = strdup(optarg);
			break;
		case 'v':
			log_level++;
			break;
		case 'f':
			cfg_file = strdup(optarg);
			break;
		default:
			print_usage();
			break;
		}
	}
	argc -= optind;
	argv += optind;

	if (log_level > 0)
		log_file();

	/* Config file. */
	if (cfg_file == NULL) {
		home = getenv("HOME");
		if (home == NULL || *home == '\0') {
			pw = getpwuid(getuid());
			if (pw != NULL)
				home = pw->pw_dir;
		}
		xasprintf(&cfg_file, "%s/" LSWM_CONFIG, home);
		if (access(cfg_file, R_OK) != 0 && errno == ENOENT) {
			free(cfg_file);
			cfg_file = NULL;
		}
	}

	if (cfg_file != NULL) {
		cfg_cmd_q = cmdq_new();
		if (load_cfg(cfg_file, cfg_cmd_q, &causes) == -1) {
			for (a = 0; a < ARRAY_LENGTH(&cfg_causes); a++) {
				log_msg("Config error: '%s'",
				    ARRAY_ITEM(&cfg_causes, a));
			}
		}
	}

	current_screen = NULL;
	TAILQ_INIT(&monitor_q);

	if (display_opt != NULL) {
		set_display(display_opt);
		free(display_opt);
	}

	dpy = xcb_connect(NULL, &default_screen);
	if (xcb_connection_has_error(dpy)) {
		log_fatal("Couldn't open display '%s'\n", getenv("DISPLAY"));
		return (1);
	}

	/* Get the current screen. */
	iter = xcb_setup_roots_iterator(xcb_get_setup(dpy));
	for (; iter.rem; --default_screen, xcb_screen_next(&iter)) {
		if (default_screen == 0) {
			current_screen = iter.data;
			break;
		}
	}

	/* Check to see if another WM is running, and bail if it is. */
	if (check_for_existing_wm() != 0)
		log_fatal("There's already a WM running");

	randr_maybe_init();
	ewmh_init();

	TAILQ_FOREACH(m, &monitor_q, entry) {
		for (i = 0; i < NO_OF_DESKTOPS; i++) {
			xasprintf(&name, "%s:%d", m->name, i);
			desktop_setup(m, name);
			free(name);
		}
	}

	client_scan_windows();

	/* Go over all monitors, print the active desktop, and any clients
	 * which are on them.
	 */
	TAILQ_FOREACH(m, &monitor_q, entry) {
		struct desktop *d;
		log_msg("M: %s", m->name);
		TAILQ_FOREACH(d, &m->desktops_q, entry) {
			struct client *c;
			log_msg("\tD: %s", d->name);
			TAILQ_FOREACH(c, &d->clients_q, entry) {
				log_msg("\t\tC: I have window: 0x%x", c->win);
			}
		}
	}

	setup_key_bindings();
	event_loop();

	log_close();

	xcb_disconnect(dpy);

	return (0);
}

static void
event_loop(void)
{
	xcb_generic_event_t	*ev;

	while ((ev = xcb_wait_for_event(dpy)) != NULL) {
		switch (ev->response_type & ~0x80) {
		case XCB_KEY_PRESS:
			log_msg("Key pressed...");
			break;
		default:
			log_msg("Got some other event...");
			break;
		}
	}
	free(ev);
}

static void
set_display(const char *dsp)
{
	char		*dpy_str;
	const char	*err_str;

	errno = 0;

	if (dsp == NULL)
		log_fatal("DISPLAY is NULL");

	/* We suppose the highest value DISPLAY can ever be is INT_MAX.
	 * Likely safe for now at any rate!
	 */
	xasprintf(&dpy_str, ":%d", strtonum(dsp, 0, INT_MAX, &err_str));

	if (err_str != NULL) {
		log_fatal("Couldn't convert %s to num - %s",
		    dsp, err_str);
	}

	if (setenv("DISPLAY", dpy_str, 1) == -1) {
		free((char *)dsp);
		free(dpy_str);
		log_fatal("Couldn't update DISPLAY because: %s",
		    strerror(errno));
	}
}

static int
check_for_existing_wm(void)
{
	unsigned int		 values[1];
	xcb_generic_error_t	*error;

	values[0] = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
		    XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT;

	error = xcb_request_check(dpy, xcb_change_window_attributes_checked(
				dpy, current_screen->root, XCB_CW_EVENT_MASK,
				values));
	xcb_flush(dpy);

	return (error != NULL) ? 1 : 0;
}

static void
print_usage(void)
{
	fprintf(stderr, "%s [-Vv] [-d DISPLAY] [-f file]\n", PROGNAME);
	exit(1);
}
