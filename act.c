/* swim - simple window manager
 * Copyright (C) 2022 ArcNyxx
 * see LICENCE file for licensing information */

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include <X11/Xlib.h>

#include "act.h"
#include "conv.h"
#include "config.h"
#include "func.h"
#include "struct.h"
#include "tile.h"
#include "util.h"

extern Display *dpy;
extern Monitor *selmon, *mons;

void
focusmon(const Arg arg)
{
	Monitor *mon;
	if (mons->next != NULL && (mon = dirtomon(arg.n)) != selmon) {
		unfocus(selmon->sel, 0);
		selmon = mon;
		focus(NULL);
	}
}

void
focusstack(const Arg arg)
{
	if (selmon->sel == NULL || selmon->sel->isfullscreen)
		return;

	Client *cli, *next;
	if (arg.n > 0) {
		for (cli = selmon->sel->next; cli != NULL &&
				!VISIBLE(cli); cli = cli->next);
		if (cli == NULL) {
			for (cli = selmon->clients; cli != NULL &&
					!VISIBLE(cli); cli = cli->next);
		}
	} else {
		cli = NULL, next = selmon->clients;
		for (; next != selmon->sel; next = next->next)
			if (VISIBLE(next))
				cli = next;
		if (cli == NULL)
			for (; next != NULL; next = next->next)
				if (VISIBLE(next))
					cli = next;
	}
	if (cli != NULL) {
		focus(cli);
		restack(selmon);
	}
}

void
incnmaster(const Arg arg)
{
	selmon->nmaster = MAX(selmon->nmaster + arg.n, 1);
	tile(selmon);
}

void
killclient(const Arg arg)
{
	extern Atom wmatom[WMLast];
	if (selmon->sel != NULL && !sendevent(selmon->sel, wmatom[WMDelete])) {
		XGrabServer(dpy);
		XKillClient(dpy, selmon->sel->win);
		XSync(dpy, false);
		XUngrabServer(dpy);
	}
}

void
quit(const Arg arg)
{
	extern bool running;
	running = false;
}

void
setmfact(const Arg arg)
{
	/* arg > 100 will set mfact absolutely */
	int newmfact = arg.n > 100 ? arg.n - 100 : selmon->mfact + arg.n;
	if (newmfact >= 5 && newmfact <= 95) {
		selmon->mfact = newmfact;
		tile(selmon);
	}
}

void
spawn(const Arg arg)
{
	if (fork() == 0) {
		if (dpy != NULL)
			close(ConnectionNumber(dpy));
		setsid();
		execvp(((char *const *)arg.v)[0], (char *const *)arg.v);
		die("swim: unable to run %s: ", ((char *const *)arg.v)[0]);
	}
}

void
startexec(const Arg arg)
{
	extern int exec;
	exec = 0;
	XGrabKey(dpy, AnyKey, AnyModifier, RootWindow(dpy, DefaultScreen(dpy)),
			true, GrabModeAsync, GrabModeAsync);
}

void
tag(const Arg arg)
{
	if (selmon->sel != NULL && (arg.n & TAG) != 0) {
		selmon->sel->tags = arg.n & TAG;
		focus(NULL);
		tile(selmon);
	}
}

void
tagmon(const Arg arg)
{
	if (selmon->sel != NULL && mons->next != NULL)
		sendmon(selmon->sel, dirtomon(arg.n));
}

void
togglebar(const Arg arg)
{
	selmon->showbar = !selmon->showbar;
	updatebarpos(selmon);
	XMoveResizeWindow(dpy, selmon->barwin, selmon->wx,
			selmon->by, selmon->ww, PADDING);
	tile(selmon);
}

void
togglegaps(const Arg arg)
{
	extern bool gap;
	gap = !gap;
	tile(selmon);
}

void
toggletag(const Arg arg)
{
	unsigned int newtags;
	if (selmon->sel != NULL &&
			(newtags = selmon->sel->tags ^ (arg.n & TAG)) != 0) {
		selmon->sel->tags = newtags;
		focus(NULL);
		tile(selmon);
	}
}

void
toggleview(const Arg arg)
{
	unsigned int newtags;
	if ((newtags = selmon->tags ^ (arg.n & TAG)) != 0) {
		selmon->tags = newtags;
		focus(NULL);
		tile(selmon);
	}
}

void
view(const Arg arg)
{
	if ((arg.n & TAG) != 0) {
		selmon->tags = arg.n & TAG;
		focus(NULL);
		tile(selmon);
	}
}

void
zoom(const Arg arg)
{
	Client *cli = selmon->sel;
	if (cli != NULL && cli->isfloating)
		return;
	if (cli != nexttiled(selmon->clients) || (cli != NULL &&
			(cli = nexttiled(cli->next)) != NULL))
		pop(cli);
}
