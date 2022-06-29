/* swim - simple window manager
 * Copyright (C) 2022 ArcNyxx
 * see LICENCE file for licensing information */

#include <X11/X.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif /* XINERAMA */

#include "bar.h"
#include "config.h"
#include "conv.h"
#include "drw.h"
#include "func.h"
#include "grab.h"
#include "tile.h"
#include "struct.h"
#include "util.h"

extern int sw, sh;
extern Atom wmatom[WMLast], netatom[NetLast];
extern Clr **scheme;
extern Display *dpy;
extern Drw *drw;
extern Monitor *mons, *sel;
extern Window root;

void
updatesizehints(Client *c)
{
	long msize;
	XSizeHints size;

	if (!XGetWMNormalHints(dpy, c->win, &size, &msize))
		/* size is uninitialized, ensure that size.flags aren't used */
		size.flags = PSize;
	if (size.flags & PBaseSize) {
		c->basew = size.base_width;
		c->baseh = size.base_height;
	} else if (size.flags & PMinSize) {
		c->basew = size.min_width;
		c->baseh = size.min_height;
	} else
		c->basew = c->baseh = 0;
	if (size.flags & PResizeInc) {
		c->incw = size.width_inc;
		c->inch = size.height_inc;
	} else
		c->incw = c->inch = 0;
	if (size.flags & PMaxSize) {
		c->maxw = size.max_width;
		c->maxh = size.max_height;
	} else
		c->maxw = c->maxh = 0;
	if (size.flags & PMinSize) {
		c->minw = size.min_width;
		c->minh = size.min_height;
	} else if (size.flags & PBaseSize) {
		c->minw = size.base_width;
		c->minh = size.base_height;
	} else
		c->minw = c->minh = 0;
	if (size.flags & PAspect) {
		c->mina = (float)size.min_aspect.y / size.min_aspect.x;
		c->maxa = (float)size.max_aspect.x / size.max_aspect.y;
	} else
		c->maxa = c->mina = 0.0;
	c->isfixed = (c->maxw && c->maxh && c->maxw == c->minw && c->maxh == c->minh);
	c->hintsvalid = 1;
}

void
configure(Client *c)
{
	XConfigureEvent evt = { .type = ConfigureNotify, .display = dpy,
			.event = c->win, .window = c->win, .x = c->x,
			.y = c->y, .width = c->w, .height = c->h, .above = 0,
			.border_width = borderw, .override_redirect = false };
	XSendEvent(dpy, c->win, false, StructureNotifyMask, (XEvent *)&evt);
}

void
detach(Client *c)
{
	Client **tc;

	for (tc = &c->mon->clients; *tc && *tc != c; tc = &(*tc)->next);
	*tc = c->next;
}

void
detachstack(Client *c)
{
	Client **tc, *t;

	for (tc = &c->mon->stack; *tc && *tc != c; tc = &(*tc)->snext);
	*tc = c->snext;

	if (c == c->mon->sel) {
		for (t = c->mon->stack; t && !VISIBLE(t); t = t->snext);
		c->mon->sel = t;
	}
}

void
focus(Client *c)
{
	if (!c || !VISIBLE(c))
		for (c = sel->stack; c && !VISIBLE(c); c = c->snext);
	if (sel->sel && sel->sel != c)
		unfocus(sel->sel, 0);
	if (c) {
		if (c->mon != sel)
			sel = c->mon;
		if (c->isurgent)
			seturgent(c, 0);
		detachstack(c);
	c->snext = c->mon->stack; c->mon->stack = c;
		grabbuttons(dpy, c, 1);
		XSetWindowBorder(dpy, c->win, scheme[ClrSel][ColBorder].pixel);
		setfocus(c);
	} else {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
	sel->sel = c;
	drawbars();
}

void
resizeclient(Client *cli, int x, int y, int w, int h)
{
	XWindowChanges wc = { .border_width = !cli->isfullscreen * borderw };
	cli->oldx = cli->x, cli->x = wc.x = x;
	cli->oldy = cli->y, cli->y = wc.y = y;
	cli->oldw = cli->w, cli->w = wc.width = w;
	cli->oldh = cli->h, cli->h = wc.height = h;
	XConfigureWindow(dpy, cli->win,
			CWX | CWY | CWWidth | CWHeight | CWBorderWidth, &wc);
	configure(cli);
	XSync(dpy, false);
}

void
restack(Monitor *mon)
{
	drawbar(mon);
	if (mon->sel == NULL)
		return;
	if (mon->sel->isfloating)
		XRaiseWindow(dpy, mon->sel->win);

	XWindowChanges wc = { .stack_mode = Below, .sibling = mon->barwin };
	for (Client *cli = mon->stack; cli != NULL; cli = cli->snext)
		if (!cli->isfloating && VISIBLE(cli)) {
			XConfigureWindow(dpy, cli->win,
					CWSibling | CWStackMode, &wc);
			wc.sibling = cli->win;
		}
	XSync(dpy, false);

	XEvent evt;
	while (XCheckMaskEvent(dpy, EnterWindowMask, &evt));
}

int
sendevent(Client *c, Atom proto)
{
	int n;
	Atom *protocols;
	int exists = 0;
	XEvent ev;

	if (XGetWMProtocols(dpy, c->win, &protocols, &n)) {
		while (!exists && n--)
			exists = protocols[n] == proto;
		XFree(protocols);
	}
	if (exists) {
		ev.type = ClientMessage;
		ev.xclient.window = c->win;
		ev.xclient.message_type = wmatom[WMProtocols];
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = proto;
		ev.xclient.data.l[1] = CurrentTime;
		XSendEvent(dpy, c->win, false, NoEventMask, &ev);
	}
	return exists;
}

void
setfocus(Client *c)
{
	if (!c->neverfocus) {
		XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
		XChangeProperty(dpy, root, netatom[NetActiveWindow],
				XA_WINDOW, 32, PropModeReplace,
				(unsigned char *)&(c->win), 1);
	}
	sendevent(c, wmatom[WMTakeFocus]);
}

void
setfullscreen(Client *c, int fullscreen)
{
	if (fullscreen && !c->isfullscreen) {
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
			PropModeReplace, (unsigned char*)&netatom[NetWMFullscreen], 1);
		c->isfullscreen = 1;
		c->oldstate = c->isfloating;
		c->isfloating = 1;
		resizeclient(c, c->mon->mx, c->mon->my, c->mon->mw, c->mon->mh);
		XRaiseWindow(dpy, c->win);
	} else if (!fullscreen && c->isfullscreen){
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
			PropModeReplace, (unsigned char*)0, 0);
		c->isfullscreen = 0;
		c->isfloating = c->oldstate;
		c->x = c->oldx;
		c->y = c->oldy;
		c->w = c->oldw;
		c->h = c->oldh;
		resizeclient(c, c->x, c->y, c->w, c->h);
		tile(c->mon);
	}
}

void
seturgent(Client *c, int urg)
{
	XWMHints *wmh;

	c->isurgent = urg;
	if (!(wmh = XGetWMHints(dpy, c->win)))
		return;
	wmh->flags = urg ? (wmh->flags | XUrgencyHint) : (wmh->flags & ~XUrgencyHint);
	XSetWMHints(dpy, c->win, wmh);
	XFree(wmh);
}

void
unfocus(Client *c, int setfocus)
{
	if (!c)
		return;
	grabbuttons(dpy, c, 0);
	XSetWindowBorder(dpy, c->win, scheme[ClrNorm][ColBorder].pixel);
	if (setfocus) {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
}

void
updatebars(void)
{
	Monitor *m;
	XSetWindowAttributes wa = {
		.override_redirect = True,
		.background_pixmap = ParentRelative,
		.event_mask = ButtonPressMask|ExposureMask
	};

	char swim[] = "swim";
	XClassHint ch = { swim, swim };

	for (m = mons; m; m = m->next) {
		if (m->barwin)
			continue;
		m->barwin = XCreateWindow(dpy, root, m->wx, m->by, m->ww, PADH, 0, DefaultDepth(dpy, DefaultScreen(dpy)),
				CopyFromParent, DefaultVisual(dpy, DefaultScreen(dpy)),
				CWOverrideRedirect|CWBackPixmap|CWEventMask, &wa);
		XMapRaised(dpy, m->barwin);
		XSetClassHint(dpy, m->barwin, &ch);
	}
}

void
updatebarpos(Monitor *m)
{
	m->wy = m->my;
	m->wh = m->mh;
	if (m->showbar) {
		m->wh -= PADH;
		if (topbar)
			m->by = m->wy, m->wy += PADH;
		else
			m->by = m->wy + m->wh;
	} else
		m->by = -PADH;
}


bool
updategeom(void)
{
	bool dirty = false;

#ifdef XINERAMA
	if (!XineramaIsActive(dpy)) {
#endif /* XINERAMA */
	if (mons == NULL) {
		mons = scalloc(1, sizeof(Monitor));
		mons->tags = 1, mons->mfact = mfact, mons->nmaster = nmaster,
				mons->showbar = showbar;
	}
	if (mons->mw != sw || mons->mh != sh) {
		dirty = true;
		mons->mw = mons->ww = sw, mons->mh = mons->wh = sh;
		updatebarpos(mons);
	}
	goto end;
#ifdef XINERAMA
	}

	size_t num = 0, new;
	for (Monitor *mon = mons; mon != NULL; mon = mon->next, ++num);
	XineramaScreenInfo *inf = XineramaQueryScreens(dpy, &new);

	for (size_t i = 0; i < new; ++i)
		for (size_t j = 0; j < i; ++j)
			if (
				inf[i].x_org  == inf[j].x_org &&
				inf[i].y_org  == inf[j].y_org &&
				inf[i].width  == inf[j].width &&
				inf[i].height == inf[j].height
			) {
				memmove(&inf[i], &inf[i + 1], (new - i) *
						sizeof(XineramaScreenInfo));
				--new;
			}

	for (size_t i = num; i < new; ++i) {
		Monitor *mon, *tmp = scalloc(1, sizeof(Monitor));
		for (mon = mons; mon != NULL && mon->next != NULL;
				mon = mon->next);
		tmp->tags = 1, tmp->mfact = mfact, tmp->nmaster = nmaster,
				tmp->showbar = showbar;
		if (mon != NULL)
			mon->next = tmp;
		else
			mons = tmp;
	}

	size_t i;
	Monitor *mon;
	for (mon = mons, i = 0; mon != NULL && i < new; mon = mon->next, ++i) {
		if (i >= num ||
			inf[i].x_org != mon->mx || inf[i].y_org  != mon->my ||
			inf[i].width != mon->mw || inf[i].height != mon->mh
		) {
			dirty = true;
			mon->mx = mon->wx = inf[i].x_org;
			mon->my = mon->wy = inf[i].y_org;
			mon->mw = mon->ww = inf[i].width;
			mon->mh = mon->wh = inf[i].height;
			updatebarpos(mon);
		}
	}

	for (size_t i = 0; i < num; ++i) {
		for (Monitor *mon = mons; mon != NULL && mon->next != NULL;
				mon = mon->next);
		for (Client *cli = mon->clients; cli != NULL;
				cli = mon->clients) {
			dirty = true;
			mon->clients = cli->next;
			detachstack(cli);
			cli->mon = mons;
			cli->next = cli->mon->clients; cli->mon->clients = cli;
			cli->snext = cli->mon->stack; cli->mon->stack = cli;
		}
		if (mon == sel)
			sel = mons;

		if (mon == mons) {
			mons = mons->next;
		} else {
			Monitor *iter = mons;
			for (; iter != NULL && iter->next != mon; iter = iter->next);
			iter->next = mon->next;
		}
		XUnmapWindow(dpy, mon->barwin);
		XDestroyWindow(dpy, mon->barwin);
		free(mon);
	}

	XFree(inf);
#endif /* XINERAMA */
end:
	if (dirty)
		sel = mons, sel = wintomon(root);
	return dirty;
}

void
updatewindowtype(Client *cli)
{
	unsigned long null;
	unsigned char *ptr;
	if (XGetWindowProperty(dpy, cli->win, netatom[NetWMState], 0,
			sizeof(Atom), false, XA_ATOM, &null, (int *)&null,
			&null, &null, &ptr) && ptr != NULL) {
		if (*(Atom *)ptr == netatom[NetWMFullscreen])
			setfullscreen(cli, true);
		XFree(ptr);
	}
	if (XGetWindowProperty(dpy, cli->win, netatom[NetWMWindowType], 0,
			sizeof(Atom), false, XA_ATOM, &null, (int *)&null,
			&null, &null, &ptr) && ptr != NULL) {
		if (*(Atom *)ptr == netatom[NetWMWindowTypeDialog])
			cli->isfloating = true;
		XFree(ptr);
	}
}
