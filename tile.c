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

static void arrange(Monitor *mon);
static void resize(Client *cli, int x, int y, int w, int h);
static void showhide(Client *cli);

bool gap = true;

extern Display *dpy;
extern Monitor *mons;

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
				mon->ww - width - 2*borderw - 2*GAPOV*gap -
				GAPIV*gap, height - 2*borderw);

		if (totgap + HEIGHT(client) + GAPIH*gap < mon->wh)
			totgap += HEIGHT(client) + GAPIH*gap;
	}
}

static void
resize(Client *cli, int x, int y, int w, int h)
{
	w = MAX(1, w), h = MAX(1, h); /* set minimum */

	Monitor *mon = cli->mon;
	if (x >= mon->wx + mon->ww)
		x = mon->wx + mon->ww - WIDTH(cli);
	if (y >= mon->wy + mon->wh)
		y = mon->wy + mon->wh - HEIGHT(cli);
	if (x + w + 2*borderw <= mon->wx)
		x = mon->wx;
	if (y + h + 2*borderw <= mon->wy)
		y = mon->wy;

	if (w < PADH)
		w = PADH;
	if (h < PADH)
		h = PADH;

	if (!rhints && !cli->isfloating)
		goto skip_hints;

	if (!cli->hintsvalid)
		updatesizehints(cli);

	bool baseismin = cli->basew == cli->minw && cli->baseh == cli->minh;
	if (baseismin)
		w -= cli->basew, h -= cli->baseh;
	if (cli->mina > 0 && cli->maxa > 0) {
		if (cli->maxa < (float)w / h)
			w = h * cli->maxa + 0.5;
		else if (cli->mina < (float)h / w)
			h = w * cli->mina + 0.5;
	}
	if (!baseismin)
		w -= cli->basew, h -= cli->baseh;
	if (cli->incw != 0)
		w -= w % cli->incw;
	if (cli->inch != 0)
		h -= h % cli->inch;

	w = MAX(w + cli->basew, cli->minw), h = MAX(h + cli->baseh, cli->minh);
	if (cli->maxw != 0)
		w = MIN(w, cli->maxw);
	if (cli->maxh != 0)
		h = MIN(h, cli->maxh);

skip_hints:
	if (x != cli->x || y != cli->y || w != cli->w || h != cli->h)
		resizeclient(cli, x, y, w, h);
}

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
