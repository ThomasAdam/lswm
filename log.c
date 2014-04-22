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

/* Routines for logging to a file to provide informative feedback for
 * debugging purposes, etc.
 */

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "lswm.h"

static FILE	*l_file;
static void	 write_variadic(const char *, FILE *, va_list);

static void
write_variadic(const char *fmt, FILE *f, va_list vl)
{
	char	*msg;

	if (f == NULL)
		return;

	if (asprintf(&msg, "%s\n", fmt) == -1)
		exit(1);
	if (vfprintf(f, msg, vl) == -1)
		exit(1);

	free(msg);
}

void
log_file(void)
{
	char *log_name;

	/* XXX: One day support different log-level severities? */
	if (log_level == 0)
		return;

	xasprintf(&log_name, "lswm-debug%s", ".log");
	//xasprintf(&log_name, "lswm-debug-%ld.log", (long)getpid());

	/* Open the log file. */
	if ((l_file = fopen(log_name, "w")) == NULL) {
		fprintf(stderr, "Couldn't open logfile\n");
		return;
	}

	setlinebuf(l_file);
}

void
log_close(void)
{
	if (l_file != NULL) {
		fflush(l_file);
		fclose(l_file);
	}
}

void
log_msg(const char *fmt, ...)
{
	va_list	 vl;

	va_start(vl, fmt);
	write_variadic(fmt, l_file, vl);
	fflush(l_file);
	va_end(vl);
}

void
log_fatal(const char *fmt, ...)
{
	char	*msg;
	va_list	 vl;

	va_start(vl, fmt);

	if (asprintf(&msg, "fatal: %s", fmt) == -1)
		exit(1);

	write_variadic(msg, l_file, vl);
	write_variadic(msg, stderr, vl);
	va_end(vl);

	free(msg);
	log_close();

	exit(1);
}
