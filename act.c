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
#include "drw.h"
#include "struct.h"
#include "util.h"

void unfocus(Client *c, int setfocus);
void focus(Client *c);
void restack(Monitor *m);
int sendevent(Client *c, Atom proto);
void updatebarpos(Monitor *m);
void pop(Client *);
void arrange(Monitor *m);
void sendmon(Client *c, Monitor *m);
Client *nexttiled(Client *c);

extern int gap, exec;
extern bool running;
extern Display *dpy;
extern Monitor *selmon, *mons;
extern Atom wmatom[WMLast];
extern Window root;
extern Drw *drw;

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

	Client *client, *next;
	if (arg.n > 0) {
		client = selmon->sel->next;
		while (client != NULL && !VISIBLE(client))
			client = client->next;
		if (client == NULL) {
			client = selmon->clients;
			while (client != NULL && !VISIBLE(client))
				client = client->next;
		}
	} else {
		client = NULL, next = selmon->clients;
		for (; next != selmon->sel; next = next->next)
			if (VISIBLE(next))
				client = next;
		if (client == NULL)
			for (; next != NULL; next = next->next)
				if (VISIBLE(next))
					client = next;
	}

	if (client != NULL) {
		focus(client);
		restack(selmon);
	}
}

void
incnmaster(const Arg arg)
{
	selmon->nmaster = MAX(selmon->nmaster + arg.n, 1);
	arrange(selmon);
}

void
killclient(const Arg arg)
{
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
	running = false;
}

void
togglegaps(const Arg arg)
{
	gap = !gap;
	arrange(selmon);
}

void
setmfact(const Arg arg)
{
	/* arg > 100 will set mfact absolutely */
	int newmfact = arg.n > 100 ? arg.n - 100 : selmon->mfact + arg.n;

	if (newmfact >= 5 && newmfact <= 95) {
		selmon->mfact = newmfact;
		arrange(selmon);
	}
}

void
spawn(const Arg arg)
{
	if (fork() == 0) {
		if (dpy)
			close(ConnectionNumber(dpy));
		setsid();
		execvp(((char **)arg.v)[0], (char **)arg.v);
		die("swim: unable to run %s: ", ((char **)arg.v)[0]);
	}
}

void
startexec(const Arg arg)
{
	exec = 0;
	XGrabKey(dpy, AnyKey, AnyModifier, root, true,
			GrabModeAsync, GrabModeAsync);
}

void
tag(const Arg arg)
{
	if (selmon->sel != NULL && (arg.n & TAG) != 0) {
		selmon->sel->tags = arg.n & TAG;
		focus(NULL);
		arrange(selmon);
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
			selmon->by, selmon->ww, (drw->fonts->h + 2));
	arrange(selmon);
}

void
toggletag(const Arg arg)
{
	unsigned int newtags;
	if (selmon->sel != NULL &&
			(newtags = selmon->sel->tags ^ (arg.n & TAG)) != 0) {
		selmon->sel->tags = newtags;
		focus(NULL);
		arrange(selmon);
	}
}

void
toggleview(const Arg arg)
{
	unsigned int newtags;
	if ((newtags = selmon->tags ^ (arg.n & TAG)) != 0) {
		selmon->tags = newtags;
		focus(NULL);
		arrange(selmon);
	}
}

void
view(const Arg arg)
{
	if ((arg.n & TAG) != 0) {
		selmon->tags = arg.n & TAG;
		focus(NULL);
		arrange(selmon);
	}
}

void
zoom(const Arg arg)
{
	Client *client = selmon->sel;
	if (client != NULL && client->isfloating)
		return;
	if (client != nexttiled(selmon->clients) || (client != NULL &&
			(client = nexttiled(client->next)) != NULL))
		pop(client);
}
