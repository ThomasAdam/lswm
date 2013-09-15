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
#include <unistd.h>
#include "lswm.h"

static void	 print_usage(void);

int main(int argc, char **argv)
{
	int	 opt;
	char	*display;

	while ((opt = getopt(argc, argv, "Vd:vf:")) != -1) {
		switch (opt) {
		/* Print the version and exit. */
		case 'V':
			printf("%s\n", VER_STR);
			exit(0);
			break;
		/* Set the DISPLAY we intend to use; overrides environment. */
		case 'd':
			xasprintf(&display, ":%d", atoi(optarg));
			setenv("DISPLAY", display, 1);
			log_msg("Set DISPLAY as: '%s'\n", getenv("DISPLAY"));
			free(display);
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

	dpy = xcb_connect(NULL, &default_screen);
	if (xcb_connection_has_error(dpy)) {
		log_fatal("Couldn't open display '%s'\n", getenv("DISPLAY"));
		return (1);
	}

	return (0);
}

static void
print_usage(void)
{
	fprintf(stderr, "%s [-Vv] [-d DISPLAY] [-f file]\n", PROGNAME);
	exit(1);
}
