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

/* Routines for generic command parsing. */

#include <string.h>
#include "lswm.h"

struct cmd	 cmd_rename;

struct cmd	*cmd_table[] = {
	&cmd_rename,
	NULL
};

struct cmd *
find_cmd(const char *cmd_name)
{
	struct cmd	**cmd_ent;
	struct cmd	 *cmd_p;

	cmd_ent = NULL;

	for (cmd_ent = cmd_table; *cmd_ent != NULL; cmd_ent++) {
		if (strcmp((*cmd_ent)->name, cmd_name) == 0) {
			cmd_p = *cmd_ent;
			break;
		}
	}

	return (cmd_p);
}
