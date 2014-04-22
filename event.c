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
static void	 handle_motion_notify(xcb_generic_event_t *);
static void	 handle_map_request(xcb_generic_event_t *);
static void	 handle_configure_notify(xcb_generic_event_t *);
static void	 handle_enter_notify(xcb_generic_event_t *);
static void	 handle_leave_notify(xcb_generic_event_t *);

static void
register_events(void)
{
	memset(events, 0, sizeof *events);

	events[XCB_KEY_PRESS] = handle_key_press;
	events[XCB_BUTTON_PRESS] = handle_button_press;
	events[XCB_MOTION_NOTIFY] = handle_motion_notify;
	events[XCB_MAP_NOTIFY] = handle_map_request;
	events[XCB_CONFIGURE_NOTIFY] = handle_configure_notify;
	events[XCB_ENTER_NOTIFY] = handle_enter_notify;
	//events[XCB_LEAVE_NOTIFY] = handle_leave_notify;
}

static void
handle_leave_notify(xcb_generic_event_t *ev)
{
	xcb_leave_notify_event_t	*lne = (xcb_leave_notify_event_t *)ev;
	struct client			*c;

	log_msg("In handle_leave_notify()");

	if (lne->event == current_screen->root)
		return;

	c = client_find_by_window(lne->event);
	if (c != NULL) {
		c->previous = 1;
		c->current = 0;
		client_active(c);
	}

	log_msg("LeaveNotify: c: %p\n", c);
}

static void
handle_enter_notify(xcb_generic_event_t *ev)
{
	xcb_enter_notify_event_t	*ene = (xcb_enter_notify_event_t *)ev;
	struct client			*c;

	log_msg("In handle_enter_notify...");

	c = client_find_by_window(ene->event);

	if (ene->event == current_screen->root) {
		log_msg("EnterNotify:  Root Window.  Bailing.");
		return;
	}

	if (ene->mode != XCB_NOTIFY_MODE_NORMAL) {
		log_msg("\tNOT handling this request due to mode...");
		return;
	}

	/* Now ascertain this window, and set its focus accordingly. */
	c->current = 1;
	c->previous = 0;
	client_active(c);
}

static void
handle_configure_notify(xcb_generic_event_t *ev)
{
	xcb_configure_request_event_t	*cr_ev;
	struct client			*c;
	int				 i, mask, values[10];

	cr_ev = (xcb_configure_request_event_t *)ev;
	i = mask = 0;

	/* No client found means the window isn't mapped yet. */
	c = client_find_by_window(cr_ev->window);

	/* Work out what this event is telling us so we can inform the client of
	 * its size, etc.
	 */
	switch (cr_ev->value_mask) {
	case XCB_CONFIG_WINDOW_X:
		mask |= XCB_CONFIG_WINDOW_X;
		values[i++] = cr_ev->x;
		break;
	case XCB_CONFIG_WINDOW_Y:
		mask |= XCB_CONFIG_WINDOW_Y;
		values[i++] = cr_ev->y;
		break;
	case XCB_CONFIG_WINDOW_WIDTH:
		mask |= XCB_CONFIG_WINDOW_WIDTH;
		values[i++] = cr_ev->width;
		break;
	case XCB_CONFIG_WINDOW_HEIGHT:
		mask |= XCB_CONFIG_WINDOW_HEIGHT;
		values[i++] = cr_ev->height;
		break;
	case XCB_CONFIG_WINDOW_BORDER_WIDTH:
		mask |= XCB_CONFIG_WINDOW_BORDER_WIDTH;
		values[i++] = cr_ev->border_width;
		break;
	case XCB_CONFIG_WINDOW_STACK_MODE:
		mask |= XCB_CONFIG_WINDOW_STACK_MODE;
		values[i++] = cr_ev->stack_mode;
		break;
	case XCB_CONFIG_WINDOW_SIBLING:
		mask |= XCB_CONFIG_WINDOW_STACK_MODE;
		values[i++] = cr_ev->sibling;
		break;
	}
	xcb_configure_window(dpy, cr_ev->window, mask, values);

	if (c != NULL)
		client_update_configure(c);
}

static void
handle_map_request(xcb_generic_event_t *ev)
{
	return;
}

static void
handle_motion_notify(xcb_generic_event_t *ev)
{
	return;
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

		if (events[rt] != NULL) {
			events[rt](ev);
			xcb_flush(dpy);
		}
		free(ev);
	}
}

