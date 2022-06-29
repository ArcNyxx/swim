/* swim - simple window manager
 * Copyright (C) 2022 ArcNyxx
 * see LICENCE file for licensing information */

#include <stdbool.h>
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
extern Monitor *sel, *mons;

void
focusmon(const Arg arg)
{
	Monitor *mon;
	if (mons->next != NULL && (mon = dirtomon(arg.n)) != sel) {
		unfocus(sel->sel, 0);
		sel = mon;
		focus(NULL);
	}
}

void
focusstack(const Arg arg)
{
	if (sel->sel == NULL || sel->sel->isfullscreen)
		return;

	Client *cli, *next;
	if (arg.n > 0) {
		for (cli = sel->sel->next; cli != NULL &&
				!VISIBLE(cli); cli = cli->next);
		if (cli == NULL) {
			for (cli = sel->clients; cli != NULL &&
					!VISIBLE(cli); cli = cli->next);
		}
	} else {
		cli = NULL, next = sel->clients;
		for (; next != sel->sel; next = next->next)
			if (VISIBLE(next))
				cli = next;
		if (cli == NULL)
			for (; next != NULL; next = next->next)
				if (VISIBLE(next))
					cli = next;
	}
	if (cli != NULL) {
		focus(cli);
		restack(sel);
	}
}

void
incnmaster(const Arg arg)
{
	sel->nmaster = MAX(sel->nmaster + arg.n, 1);
	tile(sel);
}

void
killclient(const Arg arg)
{
	extern Atom wmatom[WMLast];
	if (sel->sel != NULL && !sendevent(sel->sel, wmatom[WMDelete])) {
		XGrabServer(dpy);
		XKillClient(dpy, sel->sel->win);
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
	int newmfact = arg.n > 100 ? arg.n - 100 : sel->mfact + arg.n;
	if (newmfact >= 5 && newmfact <= 95) {
		sel->mfact = newmfact;
		tile(sel);
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
	if (sel->sel != NULL && (arg.n & TAG) != 0) {
		sel->sel->tags = arg.n & TAG;
		focus(NULL);
		tile(sel);
	}
}

void
tagmon(const Arg arg)
{
	Monitor *mon;
	if (sel->sel != NULL && mons->next != NULL &&
			(mon = dirtomon(arg.n)) != NULL) {
		Client *cli = sel->sel;
		unfocus(cli, 1);
		detach(cli);
		detachstack(cli);
		cli->mon = mon, cli->tags = mon->tags;
		cli->next = cli->mon->clients, cli->mon->clients = cli;
		cli->snext = cli->mon->stack, cli->mon->stack = cli;
		focus(NULL);
		tile(NULL);
	}
}

void
togglebar(const Arg arg)
{
	sel->showbar = !sel->showbar;
	updatebarpos(sel);
	XMoveResizeWindow(dpy, sel->barwin, sel->wx, sel->by, sel->ww, PADH);
	tile(sel);
}

void
togglegaps(const Arg arg)
{
	extern bool gap;
	gap = !gap;
	tile(sel);
}

void
toggletag(const Arg arg)
{
	unsigned int newtags;
	if (sel->sel != NULL &&
			(newtags = sel->sel->tags ^ (arg.n & TAG)) != 0) {
		sel->sel->tags = newtags;
		focus(NULL);
		tile(sel);
	}
}

void
toggleview(const Arg arg)
{
	unsigned int newtags;
	if ((newtags = sel->tags ^ (arg.n & TAG)) != 0) {
		sel->tags = newtags;
		focus(NULL);
		tile(sel);
	}
}

void
view(const Arg arg)
{
	if ((arg.n & TAG) != 0) {
		sel->tags = arg.n & TAG;
		focus(NULL);
		tile(sel);
	}
}

void
zoom(const Arg arg)
{
	Client *cli = sel->sel;
	if (cli != NULL && cli->isfloating)
		return;
	if (cli != nexttiled(sel->clients) || (cli != NULL &&
			(cli = nexttiled(cli->next)) != NULL)) {
		detach(cli);
		cli->next = cli->mon->clients, cli->mon->clients = cli;
		focus(cli);
		tile(cli->mon);
	}
}
