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

/* Routines for handling key bindings. */

#include <string.h>
#include <ctype.h>
#include <X11/Xlib.h>
#include <xkbcommon/xkbcommon.h>
#include "lswm.h"

static void		 add_binding(u_int, union pressed, u_int, const char *);

void
setup_bindings(void)
{
	u_int		 i, j, l, mouse, mbutton, modifiers;
	xkb_keysym_t	 keysym;
	u_int		 modifiers_array[] = { 0, XCB_MOD_MASK_LOCK };
	union pressed	 pressed;
	const struct keys {
		const char	*modifier_string;
		const char	*key_name;
		const char	*command_string;
		u_int		 type;
	} all_bindings[] = {
		{ "CM",	"a", "move", TYPE_KEY },
		{ "4S", "q", "move", TYPE_KEY },
		{ "C", "s", "move", TYPE_KEY },
		{ "4", "1", "move", TYPE_MOUSE },
	};

	for (i = 0; i < nitems(all_bindings); i++) {
		switch (all_bindings[i].type) {
		case TYPE_KEY: {
			/* Rather than force the user to enter in
			 * XK_$X directly, acceptthe single string notations,
			 * and use those to lookup proper keysyms directly.  
			 * Note that xcb-util doesn't seem to have the
			 * equivalent of XStringToKeysym() so we've got to
			 * default to a mixture of * X11/Xlib and XCB.  APIs!
			 * Yaaaay!
			 */
			keysym = xkb_keysym_from_name(all_bindings[i].key_name,
				0);
			pressed.key = keysym;
			break;
		}
		case TYPE_MOUSE: {
			mouse = strtonum(all_bindings[i].key_name, 0,
					INT_MAX, NULL);
			switch (mouse) {
			case 1: {
					mbutton = XCB_BUTTON_INDEX_1;
					break;
				}
			case 2: {
					mbutton = XCB_BUTTON_INDEX_2;
					break;
				}
			case 3: {
					mbutton = XCB_BUTTON_INDEX_3;
					break;
				}
			case 4: {
					mbutton = XCB_BUTTON_INDEX_4;
					break;
				}
			case 5: {
					mbutton = XCB_BUTTON_INDEX_5;
					break;
				}
			}
			pressed.button = mbutton;
		}
		}

		for (j = 0; j < strlen(all_bindings[i].modifier_string); j++)
		{
			char c = toupper(all_bindings[i].modifier_string[j]);
			switch (c) {
			case 'C':
				modifiers |= ControlMask;
				break;
			case 'M':
				modifiers |= Mod1Mask;
				break;
			case 'S':
				modifiers |= ShiftMask;
				break;
			case '4':
				modifiers |= Mod4Mask;
				break;
			}
		}
		for (l = 0; l < nitems(modifiers_array); l++) {
			add_binding(modifiers | modifiers_array[l],
					pressed, all_bindings[i].type,
					all_bindings[i].command_string);
		}
	}
	grab_all_bindings(current_screen->root);
}

void
print_key_bindings(void)
{
	struct binding	*kb;

	TAILQ_FOREACH(kb, &global_bindings, entry)
		log_msg("KEY: <<%d>> <<%d>>...", kb->modifier, kb->p.key);
}

void
grab_all_bindings(xcb_window_t win)
{
	struct binding	*kb;

	uint32_t values[] = {
		XCB_EVENT_MASK_EXPOSURE|XCB_EVENT_MASK_BUTTON_PRESS|
		XCB_EVENT_MASK_BUTTON_RELEASE|XCB_EVENT_MASK_POINTER_MOTION|
		XCB_EVENT_MASK_ENTER_WINDOW|XCB_EVENT_MASK_LEAVE_WINDOW|
                XCB_EVENT_MASK_KEY_PRESS|XCB_EVENT_MASK_KEY_RELEASE
	};
	xcb_change_window_attributes(dpy, win, XCB_CW_EVENT_MASK, values);

	TAILQ_FOREACH(kb, &global_bindings, entry) {
		switch (kb->type) {
		case TYPE_KEY:
			log_msg("Grabbing key with keysym: '%d' 0x%x",
					kb->p.key, win);
			xcb_grab_key(dpy, 1, win, kb->modifier, kb->p.key,
				     XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
			break;
		case TYPE_MOUSE:
			break;
		}
	}
}

static void
add_binding(u_int modifiers, union pressed p, u_int type, const char *cmd)
{
	struct cmd_list		*cmds;
	struct binding		*kb;
	char			*cause;

	if ((cmd_string_parse(cmd, &cmds, NULL, -1, &cause)) == -1) {
		log_msg("Couldn't get valid command: %s", cause);
		return;
	}
	kb = xmalloc(sizeof *kb);
	memset(kb, 0, sizeof *kb);

	kb->modifier = modifiers;
	kb->p = p;
	kb->type = type;
	kb->cmd_list = cmds;

	if (TAILQ_EMPTY(&global_bindings))
		TAILQ_INSERT_HEAD(&global_bindings, kb, entry);
	else
		TAILQ_INSERT_TAIL(&global_bindings, kb, entry);
}
