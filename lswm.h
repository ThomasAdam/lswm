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

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/queue.h>
#include <sys/tree.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/randr.h>
#include <X11/keysymdef.h>
#include <xcb/xcb_keysyms.h>
#include "array.h"
#include "config.h"

#define PROGNAME	"lswm"
#define VERSION		"0.1"
#define VER_STR		PROGNAME " " VERSION
#define LSWM_CONFIG	".lswmrc"

/* Definition to shut gcc up about unused arguments. */
#define unused __attribute__ ((unused))

/* Attribute to make gcc check printf-like arguments. */
#define printflike1 __attribute__ ((format (printf, 1, 2)))
#define printflike2 __attribute__ ((format (printf, 2, 3)))
#define printflike3 __attribute__ ((format (printf, 3, 4)))
#define printflike4 __attribute__ ((format (printf, 4, 5)))
#define printflike5 __attribute__ ((format (printf, 5, 6)))

#ifndef nitems
#define nitems(n) (sizeof(n) / sizeof((*n)))
#endif

#ifdef NO_STRTONUM
   long long strtonum(const char *, long long, long long, const char **);
#endif

#ifdef NO_STRLCPY
    size_t strlcpy(char *, const char *, size_t);
#endif

#ifdef NO_FGETLN
    char *fgetln(FILE *, size_t *);
#endif

#define FOCUS_BORDER 0
#define UNFOCUS_BORDER 1

#define TYPE_KEY 0x1
#define TYPE_MOUSE 0x2

/* Parsed arguments structures. */
struct args_entry {
	u_char			 flag;
	char			*value;
	RB_ENTRY(args_entry)	 entry;
};
RB_HEAD(args_tree, args_entry);

struct args {
	struct args_tree	  tree;
	int		 	  argc;
	char	       		**argv;
};

/* Information used to register commands lswm understands. */
struct cmd {
	const struct cmd_entry	*entry;
	struct args		*args;

	char			*file;
	u_int			 line;

	TAILQ_ENTRY(cmd)	 qentry;
};

struct cmd_list {
	int		 	 references;
	TAILQ_HEAD(, cmd) 	 list;
};

/* Command return values. */
enum cmd_retval {
	CMD_RETURN_ERROR = -1,
	CMD_RETURN_NORMAL = 0,
};

/* Command queue entry. */
struct cmd_q_item {
	struct cmd_list		*cmdlist;
	TAILQ_ENTRY(cmd_q_item)	 qentry;
};
TAILQ_HEAD(cmd_q_items, cmd_q_item);

/* Command queue. */
struct cmd_q {
	int			 references;
	int			 dead;

	struct cmd_q_items	 queue;
	struct cmd_q_item	*item;
	struct cmd		*cmd;

	time_t			 time;
	u_int			 number;

	void			 (*emptyfn)(struct cmd_q *);
	void			*data;
};

/* Command definition. */
struct cmd_entry {
	const char	*name;

	const char	*args_template;
	int		 args_lower;
	int		 args_upper;

	const char	*usage;

	/*void		 (*key_binding)(struct cmd *, int);*/
	enum cmd_retval	 (*exec)(struct cmd *, struct cmd_q *);
};

struct x_atoms {
	const char	*name;
	xcb_atom_t	 atom;
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

	TAILQ_ENTRY(geometry)	 entry;
};
TAILQ_HEAD(geometries, geometry);

struct client {
	xcb_window_t	 	 win;
	char			*name;

	/* Hints for controlling window size. */
	struct {
		int	 inc_w;
		int	 inc_h;
		int	 min_w;
		int	 min_h;
		int	 max_w;
		int	 max_h;
		int	 base_w;
		int	 base_h;
		int	 win_gravity_hint;
		int	 win_gravity;
	} hints;

	enum {
		NORMAL = 0,
		MAXIMISED,
		MAXIMISED_VERT,
		MAXIMISED_HORIZ,
		FULLSCREEN
	} state;

#define CLIENT_INPUT_FOCUS	0x1
#define CLIENT_URGENCY		0x2
#define CLIENT_DELETE_WINDOW	0x4
	int			 flags;

	xcb_icccm_wm_hints_t	 xwmh;
	xcb_icccm_get_wm_class_reply_t	 xch;

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
union pressed {
	xcb_keysym_t     key;
	u_int            button;
};

struct binding {
	unsigned int			 modifier;
	unsigned int			 type;

	union pressed p;

	struct cmd_list			*cmd_list;

	TAILQ_ENTRY(binding)		 entry;
};
TAILQ_HEAD(bindings, binding);

struct monitors		 monitor_q;

extern struct cmd_entry	*cmd_table[];
extern struct cmd_entry	 cmd_bindm;
extern struct cmd_entry	 cmd_move;

/* For failures of running commands during config loading. */
extern struct causelist cfg_causes;
/* List of error causes. */
ARRAY_DECL(causelist, char *);

xcb_connection_t	*dpy;
xcb_screen_t		*current_screen;
int			 default_screen;
int                      log_level;
int			 randr_start;
extern char		*cfg_file;
struct bindings		 global_bindings;

/* arguments.c */
int		 args_cmp(struct args_entry *, struct args_entry *);
RB_PROTOTYPE(args_tree, args_entry, entry, args_cmp);
struct args	*args_create(int, ...);
struct args	*args_parse(const char *, int, char **);
void		 args_free(struct args *);
size_t		 args_print(struct args *, char *, size_t);
int		 args_has(struct args *, u_char);
void		 args_set(struct args *, u_char, const char *);
const char	*args_get(struct args *, u_char);
long long	 args_strtonum(
		    struct args *, u_char, long long, long long, char **);

/* events.c */
void	 event_loop(void);

/* keys.c */
void		 setup_bindings(void);
void		 print_bindings(void);
void		 grab_all_bindings(xcb_window_t);

/* log.c */
void    log_file(void);
void    log_close(void);
void    log_msg(const char *, ...);
void    log_fatal(const char *, ...);

/* wrapper-lib.c */
int      xasprintf(char **, const char *, ...);
void	*xmalloc(size_t);
void	*xcalloc(size_t, size_t);
int	 xsprintf(char *, const char *, ...);
char	*xstrdup(const char *);
void	*xrealloc(void *, size_t, size_t);

/* randr.c */
void		 randr_maybe_init(void);
struct monitor	*monitor_at_xy(int, int);

/* desktop.c */
void		 desktop_setup(struct monitor *, const char *);
struct desktop	*desktop_create(void);
void		 add_desktop_to_monitor(struct monitor *, struct desktop *);
void		 desktop_set_name(struct desktop *, const char *);
inline int	 desktop_count_all_desktops(void);

/* cfg.c */
int		 load_cfg(const char *, struct cmd_q *, char **);
void		 cfg_show_causes(void);

/* client.c */
void	 	 client_scan_windows(void);
struct client	*client_create(xcb_window_t);
struct client	*client_find_by_window(xcb_window_t);
struct client	*client_get_current(void);
void		 client_manage_client(struct client *, bool);
void		 client_set_bw(struct client *, struct geometry *);
void		 client_set_border_colour(struct client *, int);
uint32_t	 client_get_colour(const char *);
void		 client_wm_hints(struct client *);
void		 client_wm_protocols(struct client *);
void		 client_mwm_hints(struct client *);
void		 client_get_size_hints(struct client *);
void		 client_set_name(struct client *);

/* cmd.c */
struct cmd_entry	*cmd_find_cmd(const char *);
char			**cmd_copy_argv(int, char *const *);
void			cmd_free_argv(int, char **);
size_t			cmd_print(struct cmd *, char *, size_t);
struct cmd		*cmd_parse(int, char **, const char *, u_int, char **);
void			*cmd_get_context(struct cmd *);

/* cmd-list.c */
struct cmd_list	*cmd_list_parse(int, char **, const char *, u_int, char **);
void		 cmd_list_free(struct cmd_list *);
size_t		 cmd_list_print(struct cmd_list *, char *, size_t);

/* cmd-string.c */
int	 cmd_string_parse(const char *, struct cmd_list **, const char *,
		u_int, char **);

/* cmd-queue.c */
struct cmd_q		*cmdq_new(void);
int			 cmdq_free(struct cmd_q *);
void printflike2	 cmdq_error(struct cmd_q *, const char *, ...);
void			 cmdq_run(struct cmd_q *, struct cmd_list *);
void			 cmdq_append(struct cmd_q *, struct cmd_list *);
int			 cmdq_continue(struct cmd_q *);
void			 cmdq_flush(struct cmd_q *);

/* ewmh.c */
xcb_ewmh_connection_t	*ewmh;
xcb_atom_t		 ewmh_atoms_supported[100];
xcb_atom_t	 x_atom_by_name(const char *);
void		 x_atoms_init(void);
void		 ewmh_set_active_window(void);
void		 ewmh_set_no_of_desktops(void);

#endif
