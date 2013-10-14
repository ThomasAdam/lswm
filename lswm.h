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

#include <stdbool.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/randr.h>
#include <xcb/xcb_ewmh.h>
#include "compat/queue.h"
#include "config.h"

#define PROGNAME	"lswm"
#define VERSION		"0.1"
#define VER_STR		PROGNAME " " VERSION

#ifndef nitems
#define nitems(n) (sizeof(n) / sizeof((*n)))
#endif

#ifdef NO_STRTONUM
   long long strtonum(const char *, long long, long long, const char **);
#endif

#define FOCUS_BORDER 0
#define UNFOCUS_BORDER 1

struct arg {
	char	*key;
	char	*value;

	TAILQ_ENTRY(arg)	entry;
};
TAILQ_HEAD(args, arg);

/* Information used to register commands lswm understands. */
struct cmd {
	/* The name of the command. */
	const char	*name;

	/* The arguments it accepts. */
	const char	*args_template;

	/* The queue of commands parsed for this command. */
	struct args	 args_q;

	/* The function to run. */
	int		 (*exec)(void);
};

struct rectangle {
	int	 x;
	int	 y;
	int	 w;
	int	 h;
};

struct geometry {
	/* Actual geometry coordinates. */
	struct rectangle	 coords;

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

	TAILQ_ENTRY(geometry)	 entry;
};
TAILQ_HEAD(geometries, geometry);

struct client {
	xcb_window_t	 	 win;

	enum {
		NORMAL = 0,
		MAXIMISED,
		MAXIMISED_VERT,
		MAXIMISED_HORIZ,
		FULLSCREEN
	} state;

	struct geometries	 geometries_q;

	TAILQ_ENTRY(client)	 entry;
};
TAILQ_HEAD(clients, client);

struct desktop {
	/* The name of thie desktop. */
	char			*name;

	/* The list of clients on this desktop. */
	struct clients		 clients_q;

	/* Next entry in the list. */
	TAILQ_ENTRY(desktop)	 entry;
};
TAILQ_HEAD(desktops, desktop);

struct monitor {
	const char		*name;
	xcb_randr_output_t	 id;
	struct rectangle	 size;
	bool			 changed;

	/* The active desktop; the one currently displayed. */
	struct desktop		*active_desktop;

	/* The list of all desktops on this monitor. */
	struct desktops		 desktops_q;

	TAILQ_ENTRY(monitor)	 entry;
};
TAILQ_HEAD(monitors, monitor);

/* Bindings for key/mouse. */
struct binding {
};

struct monitors		 monitor_q;

extern struct cmd	*cmd_table[];
extern struct cmd	 cmd_rename;

xcb_connection_t	*dpy;
xcb_screen_t		*current_screen;
int			 default_screen;
int                      log_level;
int			 randr_start;

/* log.c */
void    log_file(void);
void    log_close(void);
void    log_msg(const char *, ...);
void    log_fatal(const char *, ...);

/* wrapper-lib.c */
int      xasprintf(char **, const char *, ...);
void	*xmalloc(size_t);
int	 xsprintf(char *, const char *, ...);

/* randr.c */
void		 randr_maybe_init(void);
struct monitor	*monitor_at_xy(int, int);

/* desktop.c */
void		 desktop_setup(struct monitor *, const char *);
struct desktop	*desktop_create(void);
void		 add_desktop_to_monitor(struct monitor *, struct desktop *);
void		 desktop_set_name(struct desktop *, const char *);
inline int	 desktop_count_all_desktops(void);

/* client.c */
void	 	 client_scan_windows(void);
struct client	*client_create(xcb_window_t);
void		 client_manage_client(struct client *, bool);
void		 client_set_bw(struct client *, struct geometry *);
void		 client_set_border_colour(struct client *, int);
uint32_t	 client_get_colour(const char *);

/* cmd.c */
struct cmd	*find_cmd(const char *);


/* ewmh.c */
void	 ewmh_init(void);
void	 ewmh_set_active_window(void);
void	 ewmh_set_no_of_desktops(void);

#endif
