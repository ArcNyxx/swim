/* swim - simple window manager
 * Copyright (C) 2022 ArcNyxx
 * see LICENCE file for licensing information */

#include <stdbool.h>

#include <X11/Xlib.h>

#include "config.h"
#include "func.h"
#include "struct.h"
#include "tile.h"
#include "util.h"

static void showhide(Client *cli);
static void arrange(Monitor *mon);

bool gap = true;

extern Display *dpy;
extern Monitor *mons;

static void
showhide(Client *cli)
{
	if (cli == NULL)
		return;
	if (VISIBLE(cli)) {
		XMoveWindow(dpy, cli->win, cli->x, cli->y);
		if (cli->isfloating && !cli->isfullscreen)
			resize(cli, cli->x, cli->y, cli->w, cli->h);
		showhide(cli->snext); /* show clients top down */
	} else {
		showhide(cli->snext); /* hide clients bottom up */
		XMoveWindow(dpy, cli->win, WIDTH(cli) * -2, cli->y);
	}
}

static void
arrange(Monitor *mon)
{
	int totcli; /* total number of clients */
	Client *client = nexttiled(mon->clients);
	for (totcli = 0; client != NULL; ++totcli)
		client = nexttiled(client->next);
	client = nexttiled(mon->clients);

	int width; /* width of master/fullwidth windows */
	if (totcli > mon->nmaster) /* master and stacking windows separate */
		width = (mon->ww - 2*GAPOH*gap - GAPIH*gap) * mon->mfact / 100;
	else /* master takes up entire monitor */
		width = mon->ww - 2*GAPOH*gap;

	/* master/fullwidth windows handler */
	int clinum = 0, totgap = 0; /* client number and spacing */
	for (; clinum < mon->nmaster && client != NULL; ++clinum,
			client = nexttiled(client->next)) {
		int numdown = MIN(totcli, mon->nmaster) - clinum;
		int height = (mon->wh - totgap - 2*GAPOV*gap -
				GAPIV*gap * (numdown - 1)) / numdown;
		resize(client, mon->wx + GAPOH*gap, mon->wy + totgap +
				GAPOV*gap, width - 2*borderw,
				height - 2*borderw);

		if (totgap + HEIGHT(client) + GAPIH*gap < mon->wh)
			totgap += HEIGHT(client) + GAPIH*gap;
	}
	totgap = 0; /* reset for next handler to use */

	/* stacking windows handler */
	for (; client != NULL; ++clinum, client = nexttiled(client->next)) {
		int numdown = totcli - clinum;
		int height = (mon->wh - totgap - 2*GAPOV*gap -
				GAPIV*gap * (numdown - 1)) / numdown;
		resize(client, mon->wx + GAPOH*gap + width + GAPIH*gap,
				mon->wy + totgap + GAPOH*gap,
				mon->ww - width - 2*borderw -
				2*GAPOV*gap - GAPIV*gap,
				height - 2*borderw);

		if (totgap + HEIGHT(client) + GAPIH*gap < mon->wh)
			totgap += HEIGHT(client) + GAPIH*gap;
	}
}

Client *
nexttiled(Client *cli)
{
	for (; cli && (cli->isfloating || !VISIBLE(cli)); cli = cli->next);
	return cli;
}

void
tile(Monitor *mon)
{
	if (mon != NULL) {
		showhide(mon->stack);
		arrange(mon);
		restack(mon);
	} else {
		for (mon = mons; mon != NULL; mon = mon->next) {
			showhide(mon->stack);
			arrange(mon);
		}
	}
}
