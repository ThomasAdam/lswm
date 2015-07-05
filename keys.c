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
#include <X11/keysym.h>
#include <xkbcommon/xkbcommon.h>
#include <xcb/xcb_keysyms.h>
#include "lswm.h"

static void		 add_binding(u_int, union pressed, u_int, const char *);
static void		 print_key_bindings(void);
static u_int		 find_numlock(void);
static xcb_keycode_t	*get_keycodes(xcb_keysym_t);

static u_int
find_numlock(void)
{
	xcb_get_modifier_mapping_reply_t	*mm_reply;
	xcb_keycode_t				*modmap, *numlock, kc;
	u_int					 i, j, k, numlockmask;

	if ((mm_reply = xcb_get_modifier_mapping_reply(dpy,
	    xcb_get_modifier_mapping_unchecked(dpy), NULL)) == NULL) {
		log_fatal("Couldn't acquire modifier map");
		exit (1);
	}

	if ((modmap = xcb_get_modifier_mapping_keycodes(mm_reply)) == NULL) {
		log_fatal("Couldn't acquire modmap");
		exit (1);
	}

	numlock = get_keycodes(XK_Num_Lock);

	for (i = 0; i < 8; i++) {
		for (j = 0; j < mm_reply->keycodes_per_modifier; j++) {
			kc = modmap[i * mm_reply->keycodes_per_modifier + j];
			if (kc == XCB_NO_SYMBOL)
				continue;
			if (numlock != NULL) {
				for (k = 0; numlock[k] != XCB_NO_SYMBOL; k++) {
					if (numlock[k] == kc) {
						numlockmask = 1 << i;
						break;
					}
				}
			}
		}
	}
	free (numlock);
	free (mm_reply);

	return (numlockmask);
}

static
xcb_keycode_t* get_keycodes(xcb_keysym_t keysym)
{
	xcb_key_symbols_t	*keysyms;
	xcb_keycode_t		*keycode;

	if ((keysyms = xcb_key_symbols_alloc(dpy)) == NULL)
		return (NULL);

	keycode = xcb_key_symbols_get_keycode(keysyms, keysym);
	xcb_key_symbols_free(keysyms);

	return (keycode);
}

void
setup_bindings(void)
{
	u_int		 i, j, l, mouse, mbutton, modifiers;
	u_int		 modifiers_array[4], numlock;
	xcb_keysym_t	 keysym;

	union pressed	 pressed;
	const struct keys {
		const char	*modifier_string;
		const char	*key_name;
		const char	*command_string;
		u_int		 type;
	} all_bindings[] = {
		{ "CM",	"a", "move", TYPE_KEY },
		{ "4", "q", "move", TYPE_KEY },
		{ "C", "s", "move", TYPE_KEY },
		{ "4", "1", "move", TYPE_MOUSE },
	};

	numlock = find_numlock();
	modifiers_array[0] = 0;
	modifiers_array[1] = XCB_MOD_MASK_LOCK;
	modifiers_array[2] = numlock;
	modifiers_array[3] = numlock | XCB_MOD_MASK_LOCK;

	for (i = 0; i < nitems(all_bindings); i++) {
		switch (all_bindings[i].type) {
		case TYPE_KEY: {
			keysym = xkb_keysym_from_name(all_bindings[i].key_name,
			    XKB_KEYSYM_NO_FLAGS);
			if (keysym == XKB_KEY_NoSymbol) {
				log_msg("Unable to bind key: %s, no symbol",
					all_bindings[i].key_name);
				continue;
			}
			pressed.key = keysym;
			break;
		}
		case TYPE_MOUSE: {
			mouse = strtonum(all_bindings[i].key_name, 0,
					INT_MAX, NULL);
			log_msg("Mouse button '%d' found", mouse);
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
	print_key_bindings();
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
	xcb_keycode_t	*kc;
	u_int		 i;

	uint32_t values[] = {
		XCB_EVENT_MASK_EXPOSURE|XCB_EVENT_MASK_BUTTON_PRESS|
		XCB_EVENT_MASK_BUTTON_RELEASE|XCB_EVENT_MASK_POINTER_MOTION|
		XCB_EVENT_MASK_ENTER_WINDOW|XCB_EVENT_MASK_LEAVE_WINDOW|
                XCB_EVENT_MASK_KEY_PRESS|XCB_EVENT_MASK_KEY_RELEASE
	};
	xcb_change_window_attributes(dpy, win, XCB_CW_EVENT_MASK, values);

	xcb_ungrab_key(dpy, XCB_GRAB_ANY, win, XCB_MOD_MASK_ANY);

	TAILQ_FOREACH(kb, &global_bindings, entry) {
		switch (kb->type) {
		case TYPE_KEY: {
			kc = get_keycodes(kb->p.key);

			for (i = 0; kc[i] != XCB_NO_SYMBOL; i++) {
				log_msg("Grabbing key with keysym: '%d' 0x%x",
					kc[i], win);
				xcb_grab_key(dpy, 0, win, kb->modifier,
				    kc[i], XCB_GRAB_MODE_SYNC,
				    XCB_GRAB_MODE_ASYNC);
			}
			free(kc);
		}
			break;
		case TYPE_MOUSE:
			log_msg("Grabbing mouse button (win: 0x%x)...", win);
			xcb_grab_button(dpy, 0, win,
					XCB_EVENT_MASK_BUTTON_PRESS,
					XCB_GRAB_MODE_SYNC,
					XCB_GRAB_MODE_ASYNC,
					XCB_NONE,
					XCB_NONE,
					kb->p.button, kb->modifier);
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
	kb->modifier = modifiers;
	kb->p = p;
	kb->type = type;
	kb->cmd_list = cmds;

	if (TAILQ_EMPTY(&global_bindings))
		TAILQ_INSERT_HEAD(&global_bindings, kb, entry);
	else
		TAILQ_INSERT_TAIL(&global_bindings, kb, entry);
}
