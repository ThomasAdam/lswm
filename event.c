/*
 * Copyright (c) 2013 Thomas Adam <thomas@xteddu.org>
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

/* Routines to handle the main event loop. */

#include <string.h>
#include <xcb/xcb_keysyms.h>
#include <X11/Xlib.h>
#include <X11/keysymdef.h>
#include "lswm.h"

#define CLEANMASK(mask) (mask & ~(XCB_MOD_MASK_LOCK))

static void	 (*events[XCB_NO_OPERATION])(xcb_generic_event_t *);
static void	 register_events(void);

static void	 handle_key_press(xcb_generic_event_t *);
static void	 handle_button_press(xcb_generic_event_t *);

static void
register_events(void)
{
	memset(events, 0, sizeof *events);

	events[XCB_KEY_PRESS] = handle_key_press;
	events[XCB_BUTTON_PRESS] = handle_button_press;
}

static void
handle_button_press(xcb_generic_event_t *ev)
{
	xcb_button_press_event_t	*bp_ev = (xcb_button_press_event_t *)ev;

	log_msg("BUTTON PRESS: %d, state: %d", bp_ev->detail, bp_ev->state);
}

static void
handle_key_press(xcb_generic_event_t *ev)
{
	xcb_key_press_event_t	*kp_ev = (xcb_key_press_event_t *)ev;
	xcb_keysym_t		 keysym;
	xcb_key_symbols_t       *all_keysyms;
	struct binding		*kb;
	u_int			 clean_mask, mod_clean;
	struct cmd		*cmd;
	struct cmd_q		*cmdq = cmdq_new();

	if ((all_keysyms = xcb_key_symbols_alloc(dpy)) == NULL)
		log_fatal("Couldn't find keysyms...");

	keysym = xcb_key_press_lookup_keysym(all_keysyms, kp_ev, 0);
	clean_mask = kp_ev->state & ~(XCB_MOD_MASK_LOCK);

	TAILQ_FOREACH(kb, &global_bindings, entry) {
		if (kb->type != TYPE_KEY)
			continue;

		mod_clean = kb->modifier & ~(XCB_MOD_MASK_LOCK);
		log_msg("KP: %d, K: %d, M: %d (%d)", keysym, kb->p.key,
				mod_clean, clean_mask);
		if (keysym == kb->p.key && kp_ev->state == clean_mask) {
			cmdq_run(cmdq, kb->cmd_list);
		}
	}
}

void
event_loop(void)
{
	xcb_generic_event_t	*ev;
	u_int			 rt;

	register_events();

	while ((ev = xcb_wait_for_event(dpy)) != NULL) {
		rt = ev->response_type & ~0x80;

		if (events[rt] != NULL)
			events[rt](ev);
		free(ev);
	}
}

