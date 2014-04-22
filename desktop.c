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

/* Routines for handling desktops */

#include <string.h>
#include "lswm.h"

struct desktop *
desktop_create(void)
{
	struct desktop	*d;

	d = xmalloc(sizeof *d);
	memset(d, 0, sizeof *d);

	TAILQ_INIT(&d->clients_q);
	TAILQ_INIT(&d->client_focus_q);

	d->name = NULL;

	return (d);
}

void
add_desktop_to_monitor(struct monitor *m, struct desktop *d)
{
	if (m == NULL) {
		log_fatal("Can't add desktop '%s' to nonexistent monitor",
			  d->name);
	}

	if (TAILQ_EMPTY(&m->desktops_q))
		TAILQ_INSERT_HEAD(&m->desktops_q, d, entry);
	else
		TAILQ_INSERT_TAIL(&m->desktops_q, d, entry);

	log_msg("Added desktop: '%s' to monitor: '%s'", d->name, m->name);
}

void
desktop_set_name(struct desktop *d, const char *name)
{
	if (name == NULL)
		log_fatal("Setting desktop name cannot be NULL");

	free(d->name);
	d->name = strdup(name);
}

void
desktop_setup(struct monitor *m, const char *name)
{
	struct desktop	*d;

	d = desktop_create();
	desktop_set_name(d, name);
	add_desktop_to_monitor(m, d);
}

struct desktop *
desktop_contains_client(struct client *c)
{
	struct monitor	*m;
	struct desktop	*d, *find_d = NULL;
	struct client	*c1;

	TAILQ_FOREACH(m, &monitor_q, entry) {
		TAILQ_FOREACH(d, &m->desktops_q, entry) {
			TAILQ_FOREACH(c1, &d->clients_q, entry) {
				if (c1 == c) {
					find_d = d;
					break;
				}
			}
		}
	}

	return (find_d);
}

inline int
desktop_count_all_desktops(void)
{
	struct monitor	*m;
	struct desktop	*d;
	int		 c;

	c = 0;
	TAILQ_FOREACH(m, &monitor_q, entry) {
		TAILQ_FOREACH(d, &m->desktops_q, entry) {
			c++;
		}
	}

	return (c);
}
