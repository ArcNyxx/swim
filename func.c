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
#include "xerr.h"

extern int sw, sh;
extern Atom wmatom[WMLast], netatom[NetLast];
extern Cursor cursor;
extern Clr **scheme;
extern Display *dpy;
extern Drw *drw;
extern Monitor *mons, *selmon;
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
		for (c = selmon->stack; c && !VISIBLE(c); c = c->snext);
	if (selmon->sel && selmon->sel != c)
		unfocus(selmon->sel, 0);
	if (c) {
		if (c->mon != selmon)
			selmon = c->mon;
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
	selmon->sel = c;
	drawbars();
}

int
gettextprop(Window w, Atom atom, char *text, unsigned int size)
{
	char **list = NULL;
	int n;
	XTextProperty name;

	if (!text || size == 0)
		return 0;
	text[0] = '\0';
	if (!XGetTextProperty(dpy, w, &name, atom) || !name.nitems)
		return 0;
	if (name.encoding == XA_STRING)
		strncpy(text, (char *)name.value, size - 1);
	else {
		if (XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success && n > 0 && *list) {
			strncpy(text, *list, size - 1);
			XFreeStringList(list);
		}
	}
	text[size - 1] = '\0';
	XFree(name.value);
	return 1;
}

#ifdef XINERAMA
static int
isuniquegeom(XineramaScreenInfo *unique, size_t n, XineramaScreenInfo *info)
{
	while (n--)
		if (unique[n].x_org == info->x_org && unique[n].y_org == info->y_org
		&& unique[n].width == info->width && unique[n].height == info->height)
			return 0;
	return 1;
}
#endif /* XINERAMA */

void
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
		XDefineCursor(dpy, m->barwin, cursor);
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

int
updategeom(void)
{
	int dirty = 0;

#ifdef XINERAMA
	if (XineramaIsActive(dpy)) {
		int i, j, n, nn;
		Client *c;
		Monitor *m;
		XineramaScreenInfo *info = XineramaQueryScreens(dpy, &nn);
		XineramaScreenInfo *unique = NULL;

		for (n = 0, m = mons; m; m = m->next, n++);
		/* only consider unique geometries as separate screens */
		unique = scalloc(nn, sizeof(XineramaScreenInfo));
		for (i = 0, j = 0; i < nn; i++)
			if (isuniquegeom(unique, j, &info[i]))
				memcpy(&unique[j++], &info[i], sizeof(XineramaScreenInfo));
		XFree(info);
		nn = j;

		/* new monitors if nn > n */
		for (i = n; i < nn; i++) {
			for (m = mons; m && m->next; m = m->next);
			if (m) {
				Monitor *mon = scalloc(1, sizeof(Monitor));
				mon->tags = 1, mon->mfact = mfact, mon->nmaster = nmaster,
						mon->showbar = showbar;
				m->next = mon;
			} else {
				Monitor *mon = scalloc(1, sizeof(Monitor));
				mon->tags = 1, mon->mfact = mfact, mon->nmaster = nmaster,
						mon->showbar = showbar;
				mons = mon;
			}
		}
		for (i = 0, m = mons; i < nn && m; m = m->next, i++)
			if (i >= n
			|| unique[i].x_org != m->mx || unique[i].y_org != m->my
			|| unique[i].width != m->mw || unique[i].height != m->mh)
			{
				dirty = 1;
				m->mx = m->wx = unique[i].x_org;
				m->my = m->wy = unique[i].y_org;
				m->mw = m->ww = unique[i].width;
				m->mh = m->wh = unique[i].height;
				updatebarpos(m);
			}
		/* removed monitors if n > nn */
		for (i = nn; i < n; i++) {
			for (m = mons; m && m->next; m = m->next);
			while ((c = m->clients)) {
				dirty = 1;
				m->clients = c->next;
				detachstack(c);
				c->mon = mons;
				c->next = c->mon->clients; c->mon->clients = c;
				c->snext = c->mon->stack; c->mon->stack = c;
			}
			if (m == selmon)
				selmon = mons;


			if (m == mons) {
				mons = mons->next;
			} else {
				Monitor *iter = mons;
				for (; iter != NULL && iter->next != m; iter = iter->next);
				iter->next = m->next;
			}
			XUnmapWindow(dpy, m->barwin);
			XDestroyWindow(dpy, m->barwin);
			free(m);
		}
		free(unique);
	} else
#endif /* XINERAMA */
	{ /* default monitor setup */
		if (!mons) {
			Monitor *mon = scalloc(1, sizeof(Monitor));
			mon->tags = 1, mon->mfact = mfact, mon->nmaster = nmaster,
					mon->showbar = showbar;
			mons = mon;
		}
		if (mons->mw != sw || mons->mh != sh) {
			dirty = 1;
			mons->mw = mons->ww = sw;
			mons->mh = mons->wh = sh;
			updatebarpos(mons);
		}
	}
	if (dirty) {
		selmon = mons;
		selmon = wintomon(root);
	}
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

void
updatewmhints(Client *c)
{
	XWMHints *wmh;

	if ((wmh = XGetWMHints(dpy, c->win))) {
		if (c == selmon->sel && wmh->flags & XUrgencyHint) {
			wmh->flags &= ~XUrgencyHint;
			XSetWMHints(dpy, c->win, wmh);
		} else
			c->isurgent = (wmh->flags & XUrgencyHint) ? 1 : 0;
		c->neverfocus = wmh->flags & InputHint ? !wmh->input : 0;
		XFree(wmh);
	}
}
