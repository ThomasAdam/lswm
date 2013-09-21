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
#include "lswm.h"

static void	 print_usage(void);
static void	 set_display(const char *);

int main(int argc, char **argv)
{
	int			 opt;
	char			*display_opt = NULL;
	xcb_screen_iterator_t	 iter;

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
			/* TODO:  config file! */
			break;
		default:
			print_usage();
			break;
		}
	}
	argc -= optind;
	argv += optind;

	current_screen = NULL;
	TAILQ_INIT(&monitor_q);

	if (log_level > 0)
		log_file();

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

	randr_maybe_init();
	//desktops_setup();

	log_close();

	return (0);
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

static void
print_usage(void)
{
	fprintf(stderr, "%s [-Vv] [-d DISPLAY] [-f file]\n", PROGNAME);
	exit(1);
}
