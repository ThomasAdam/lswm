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

#include <X11/Xlib.h>
#include <X11/keysymdef.h>
#include "lswm.h"

void
setup_key_bindings(void)
{
	u_int		 i;
	xcb_keysym_t	 keysym;
	const struct keys {
		const char	*modifier_string;
		const char	*key_name;
		const char	*command_string;
	} all_keys[] = {
		{ "CM",	"a", "move" },
		{ "4S", "q", "move" },
	};

	for (i = 0; i < nitems(all_keys); i++) {
		/* Rather than force the user to enter in XK_$X directly, accept
		 * the single string notations, and use those to lookup proper
		 * keysyms directly.
		 *
		 * Note that xcb-util doesn't seem to have the equivalent of
		 * XStringToKeysym() so we've got to default to a mixture of
		 * X11/Xlib and XCB.  APIs!  Yaaaay!
		 */

		keysym = XStringToKeysym(all_keys[i].key_name);
	}
}
