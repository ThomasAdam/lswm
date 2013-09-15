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

#ifndef _LSWM__H_
#define _LSWM__H_

#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/randr.h>
#include <xcb/xcb_ewmh.h>

#define PROGNAME	"lswm"
#define VERSION		"0.1"
#define VER_STR		PROGNAME " " VERSION

struct geometry {
	/* Actual geometry coordinates. */
	int	 x;
	int	 y;
	int	 w;
	int	 h;
	
	/* The window's border width */
	int	 bw;

	/* Hints for controlling window size. */
	struct {
		int	 min_w;
		int	 min_h;
		int	 max_w;
		int	 max_h;
		int	 base_w;
		int	 base_h;
		int	 win_gravity_hint;
		int	 win_gravity;
	} hints;
};

struct ewmh;
struct client;
struct monitor;

xcb_connection_t	*dpy;
int			 default_screen;
int                     log_level;

/* log.c */
void    log_file(void);
void    log_close(void);
void    log_msg(const char *, ...);
void    log_fatal(const char *, ...);

/* wrapper-lib.c */
int     xasprintf(char **, const char *, ...);
void   *xmalloc(size_t);
#endif
