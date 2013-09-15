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

/* Provides some wrappers around various syscalls to handle things like
 * memory/error checking, etc.
 */

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include "lswm.h"

/* Wrapper around asprintf() to handle error code returns. */
int
xasprintf(char **out, const char *fmt, ...)
{
	va_list	 ap;
	int	 i;

	va_start(ap, fmt);
	i = vasprintf(out, fmt, ap);

	if (i == -1) {
		/* Then there were problems allocating memory. */
		log_fatal("Couldn't allocate memory for asprintf()");
	}

	va_end(ap);

	return (i);
}

/* Wrapper for malloc() to handle memory */
void *
xmalloc(size_t s)
{
	void	*mem;

	if (s == 0)
		log_fatal("Cannot pass zero size to malloc()");

	if ((mem = malloc(s)) == NULL)
		log_fatal("malloc() returned NULL");

	return (mem);
}

/* Wrapper for s[n]printf */
int
xsprintf(char *out, const char *fmt, ...)
{
	va_list	 ap;
	int	 i;

	va_start(ap, fmt);
	i = sprintf(out, fmt, ap);

	if (i == -1)
		log_fatal("sprintf failed.");

	va_end(ap);

	return (i);
}
