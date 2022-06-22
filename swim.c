/* swim - simple window manager
 * Copyright (C) 2021-2022 ArcNyxx
 * see LICENCE file for licensing information */

#include <signal.h>
#include <sys/wait.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif /* XINERAMA */
#include <X11/Xft/Xft.h>
#include <X11/XKBlib.h>
#include <xkbcommon/xkbcommon.h>

#include "act.h"
#include "config.h"
#include "conv.h"
#include "drw.h"
#include "grab.h"
#include "struct.h"
#include "util.h"
#include "xerr.h"

int applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact);
void arrange(Monitor *m);
void buttonpress(XEvent *e);
void cleanupmon(Monitor *mon);
void clientmessage(XEvent *e);
void configure(Client *c);
void configurenotify(XEvent *e);
void configurerequest(XEvent *e);
Monitor *createmon(void);
void destroynotify(XEvent *e);
void detach(Client *c);
void detachstack(Client *c);
void drawbar(Monitor *m);
void drawbars(void);
void enternotify(XEvent *e);
void expose(XEvent *e);
void focus(Client *c);
void focusin(XEvent *e);
Atom getatomprop(Client *c, Atom prop);
long getstate(Window w);
int gettextprop(Window w, Atom atom, char *text, unsigned int size);
void keypress(XEvent *e);
void manage(Window w, XWindowAttributes *wa);
void mappingnotify(XEvent *e);
void maprequest(XEvent *e);
void motionnotify(XEvent *e);
Client *nexttiled(Client *c);
void pop(Client *);
void propertynotify(XEvent *e);
void resize(Client *c, int x, int y, int w, int h, int interact);
void resizeclient(Client *c, int x, int y, int w, int h);
void restack(Monitor *m);
int sendevent(Client *c, Atom proto);
void sendmon(Client *c, Monitor *m);
void setclientstate(Client *c, long state);
void setfocus(Client *c);
void setfullscreen(Client *c, int fullscreen);
void seturgent(Client *c, int urg);
void showhide(Client *c);
void sigchld(int unused);
void tile(Monitor *);
void unfocus(Client *c, int setfocus);
void unmanage(Client *c, int destroyed);
void unmapnotify(XEvent *e);
void updatebarpos(Monitor *m);
void updatebars(void);
void updateclientlist(void);
int updategeom(void);
void updatenumlockmask(void);
void updatesizehints(Client *c);
void updatetitle(Client *c);
void updatewindowtype(Client *c);
void updatewmhints(Client *c);

int exec = -1;
char stext[256], execa[256] = { 0 };

int screen;
int sw, sh;	   /* X display screen geometry width, height */
int gap = 1;	  /* enables gaps, used by togglegaps */
Atom wmatom[WMLast], netatom[NetLast];
int running = 1;
Cursor cursor;
Clr **scheme;
Display *dpy;
Drw *drw;
Monitor *mons, *selmon;
Window root;

int
applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact)
{
	int baseismin;
	Monitor *m = c->mon;

	/* set minimum possible */
	*w = MAX(1, *w);
	*h = MAX(1, *h);
	if (interact) {
		if (*x > sw)
			*x = sw - WIDTH(c);
		if (*y > sh)
			*y = sh - HEIGHT(c);
		if (*x + *w + 2*borderw < 0)
			*x = 0;
		if (*y + *h + 2*borderw < 0)
			*y = 0;
	} else {
		if (*x >= m->wx + m->ww)
			*x = m->wx + m->ww - WIDTH(c);
		if (*y >= m->wy + m->wh)
			*y = m->wy + m->wh - HEIGHT(c);
		if (*x + *w + 2*borderw <= m->wx)
			*x = m->wx;
		if (*y + *h + 2*borderw <= m->wy)
			*y = m->wy;
	}
	if (*h < (drw->fonts->h + 2))
		*h = (drw->fonts->h + 2);
	if (*w < (drw->fonts->h + 2))
		*w = (drw->fonts->h + 2);
	if (resizehints || c->isfloating) {
		/* see last two sentences in ICCCM 4.1.2.3 */
		baseismin = c->basew == c->minw && c->baseh == c->minh;
		if (!baseismin) { /* temporarily remove base dimensions */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for aspect limits */
		if (c->mina > 0 && c->maxa > 0) {
			if (c->maxa < (float)*w / *h)
				*w = *h * c->maxa + 0.5;
			else if (c->mina < (float)*h / *w)
				*h = *w * c->mina + 0.5;
		}
		if (baseismin) { /* increment calculation requires this */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for increment value */
		if (c->incw)
			*w -= *w % c->incw;
		if (c->inch)
			*h -= *h % c->inch;
		/* restore base dimensions */
		*w = MAX(*w + c->basew, c->minw);
		*h = MAX(*h + c->baseh, c->minh);
		if (c->maxw)
			*w = MIN(*w, c->maxw);
		if (c->maxh)
			*h = MIN(*h, c->maxh);
	}
	return *x != c->x || *y != c->y || *w != c->w || *h != c->h;
}

void
arrange(Monitor *m)
{
	if (m)
		showhide(m->stack);
	else for (m = mons; m; m = m->next)
		showhide(m->stack);
	if (m) {
		tile(m);
		restack(m);
	} else for (m = mons; m; m = m->next)
		tile(m);
}

void
buttonpress(XEvent *e)
{
	unsigned int i, x, click;
	Arg arg = {0};
	Client *c;
	Monitor *m;
	XButtonPressedEvent *ev = &e->xbutton;

	click = ClkRootWin;
	/* focus monitor if necessary */
	if ((m = wintomon(ev->window)) && m != selmon) {
		unfocus(selmon->sel, 1);
		selmon = m;
		focus(NULL);
	}
	if (ev->window == selmon->barwin) {
		i = x = 0;
		do
			x += drw_fontset_getwidth(drw, tags[i]) + (drw->fonts->h);
		while (ev->x >= x && ++i < LENGTH(tags));
		if (i < LENGTH(tags)) {
			click = ClkTagBar;
			arg.n = 1 << i;
		} else if (ev->x > selmon->ww - (int)drw_fontset_getwidth(drw, stext) + (drw->fonts->h))
			click = ClkStatusText;
		else
			click = ClkWinTitle;
	} else if ((c = wintoclient(ev->window))) {
		focus(c);
		restack(selmon);
		XAllowEvents(dpy, ReplayPointer, CurrentTime);
		click = ClkClientWin;
	}
	for (i = 0; i < LENGTH(buttons); i++)
		if (click == buttons[i].click && buttons[i].func && buttons[i].button == ev->button
		&& CLEAN(buttons[i].mask) == CLEAN(ev->state))
			buttons[i].func(click == ClkTagBar && buttons[i].arg.n == 0 ? arg : buttons[i].arg);
}

void
cleanupmon(Monitor *mon)
{
	Monitor *m;

	if (mon == mons)
		mons = mons->next;
	else {
		for (m = mons; m && m->next != mon; m = m->next);
		m->next = mon->next;
	}
	XUnmapWindow(dpy, mon->barwin);
	XDestroyWindow(dpy, mon->barwin);
	free(mon);
}

void
clientmessage(XEvent *e)
{
	XClientMessageEvent *cme = &e->xclient;
	Client *c = wintoclient(cme->window);

	if (!c)
		return;
	if (cme->message_type == netatom[NetWMState]) {
		if (cme->data.l[1] == netatom[NetWMFullscreen]
		|| cme->data.l[2] == netatom[NetWMFullscreen])
			setfullscreen(c, (cme->data.l[0] == 1 /* _NET_WM_STATE_ADD    */
				|| (cme->data.l[0] == 2 /* _NET_WM_STATE_TOGGLE */ && !c->isfullscreen)));
	} else if (cme->message_type == netatom[NetActiveWindow]) {
		if (c != selmon->sel && !c->isurgent)
			seturgent(c, 1);
	}
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
configurenotify(XEvent *e)
{
	Monitor *m;
	Client *c;
	XConfigureEvent *ev = &e->xconfigure;
	int dirty;

	/* TODO: updategeom handling sucks, needs to be simplified */
	if (ev->window == root) {
		dirty = (sw != ev->width || sh != ev->height);
		sw = ev->width;
		sh = ev->height;
		if (updategeom() || dirty) {
			drw_resize(drw, sw, (drw->fonts->h + 2));
			updatebars();
			for (m = mons; m; m = m->next) {
				for (c = m->clients; c; c = c->next)
					if (c->isfullscreen)
						resizeclient(c, m->mx, m->my, m->mw, m->mh);
				XMoveResizeWindow(dpy, m->barwin, m->wx, m->by, m->ww, (drw->fonts->h + 2));
			}
			focus(NULL);
			arrange(NULL);
		}
	}
}

void
configurerequest(XEvent *e)
{
	Client *c;
	Monitor *m;
	XConfigureRequestEvent *ev = &e->xconfigurerequest;
	XWindowChanges wc;

	if ((c = wintoclient(ev->window))) {
		if (c->isfloating) {
			m = c->mon;
			if (ev->value_mask & CWX) {
				c->oldx = c->x;
				c->x = m->mx + ev->x;
			}
			if (ev->value_mask & CWY) {
				c->oldy = c->y;
				c->y = m->my + ev->y;
			}
			if (ev->value_mask & CWWidth) {
				c->oldw = c->w;
				c->w = ev->width;
			}
			if (ev->value_mask & CWHeight) {
				c->oldh = c->h;
				c->h = ev->height;
			}
			if ((c->x + c->w) > m->mx + m->mw && c->isfloating)
				c->x = m->mx + (m->mw / 2 - WIDTH(c) / 2); /* center in x direction */
			if ((c->y + c->h) > m->my + m->mh && c->isfloating)
				c->y = m->my + (m->mh / 2 - HEIGHT(c) / 2); /* center in y direction */
			if ((ev->value_mask & (CWX|CWY)) && !(ev->value_mask & (CWWidth|CWHeight)))
				configure(c);
			if (VISIBLE(c))
				XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
		} else if (!(ev->value_mask & CWBorderWidth))
			configure(c);
	} else {
		wc.x = ev->x;
		wc.y = ev->y;
		wc.width = ev->width;
		wc.height = ev->height;
		wc.border_width = ev->border_width;
		wc.sibling = ev->above;
		wc.stack_mode = ev->detail;
		XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
	}
	XSync(dpy, false);
}

Monitor *
createmon(void)
{
	Monitor *m;

	m = scalloc(1, sizeof(Monitor));
	m->tags = 1;
	m->mfact = mfact;
	m->nmaster = nmaster;
	m->showbar = showbar;
	return m;
}

void
destroynotify(XEvent *e)
{
	Client *c;
	XDestroyWindowEvent *ev = &e->xdestroywindow;

	if ((c = wintoclient(ev->window)))
		unmanage(c, 1);
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
drawbar(Monitor *m)
{
	int x, w, tw = 0;
	int boxs = drw->fonts->h / 9;
	int boxw = drw->fonts->h / 6 + 2;
	unsigned int i, occ = 0, urg = 0;
	Client *c;

	if (!m->showbar)
		return;

	/* draw status first so it can be overdrawn by tags later */
	if (m == selmon) { /* status is only drawn on selected monitor */
		drw_setscheme(drw, scheme[ClrNorm]);
		tw = drw_fontset_getwidth(drw, stext) + 2; /* 2px right padding */
		drw_text(drw, m->ww - tw, 0, tw, (drw->fonts->h + 2), 0, stext, 0);
	}

	for (c = m->clients; c; c = c->next) {
		occ |= c->tags;
		if (c->isurgent)
			urg |= c->tags;
	}
	x = 0;
	for (i = 0; i < LENGTH(tags); i++) {
		w = drw_fontset_getwidth(drw, tags[i]) + (drw->fonts->h);
		drw_setscheme(drw, scheme[m->tags & 1 << i ? ClrSel : ClrNorm]);
		drw_text(drw, x, 0, w, (drw->fonts->h + 2), (drw->fonts->h) / 2, tags[i], urg & 1 << i);
		if (occ & 1 << i)
			drw_rect(drw, x + boxs, boxs, boxw, boxw,
				m == selmon && selmon->sel && selmon->sel->tags & 1 << i,
				urg & 1 << i);
		x += w;
	}

	if ((w = m->ww - tw - x) > (drw->fonts->h + 2)) {
		if (exec != -1) {
			drw_setscheme(drw, scheme[m == selmon ? ClrSel : ClrNorm]);
			drw_text(drw, x, 0, w, (drw->fonts->h + 2), (drw->fonts->h) / 2, execa, 0);
		} else if (m->sel) {
			drw_setscheme(drw, scheme[m == selmon ? ClrSel : ClrNorm]);
			if (exec == -1)
				drw_text(drw, x, 0, w, (drw->fonts->h + 2), (drw->fonts->h) / 2, m->sel->name, 0);
			if (m->sel->isfloating)
				drw_rect(drw, x + boxs, boxs, boxw, boxw, m->sel->isfixed, 0);
		} else {
			drw_setscheme(drw, scheme[ClrNorm]);
			drw_rect(drw, x, 0, w, (drw->fonts->h + 2), 1, 1);
		}
	}
	drw_map(drw, m->barwin, 0, 0, m->ww, (drw->fonts->h + 2));
}

void
drawbars(void)
{
	Monitor *m;

	for (m = mons; m; m = m->next)
		drawbar(m);
}

void
enternotify(XEvent *e)
{
	Client *c;
	Monitor *m;
	XCrossingEvent *ev = &e->xcrossing;

	if ((ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root)
		return;
	c = wintoclient(ev->window);
	m = c ? c->mon : wintomon(ev->window);
	if (m != selmon) {
		unfocus(selmon->sel, 1);
		selmon = m;
	} else if (!c || c == selmon->sel)
		return;
	focus(c);
}

void
expose(XEvent *e)
{
	Monitor *m;
	XExposeEvent *ev = &e->xexpose;

	if (ev->count == 0 && (m = wintomon(ev->window)))
		drawbar(m);
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

/* there are some broken focus acquiring clients needing extra handling */
void
focusin(XEvent *e)
{
	XFocusChangeEvent *ev = &e->xfocus;

	if (selmon->sel && ev->window != selmon->sel->win)
		setfocus(selmon->sel);
}

Atom
getatomprop(Client *c, Atom prop)
{
	int di;
	unsigned long dl;
	unsigned char *p = NULL;
	Atom da, atom = None;

	if (XGetWindowProperty(dpy, c->win, prop, 0L, sizeof atom, false, XA_ATOM,
		&da, &di, &dl, &dl, &p) == Success && p) {
		atom = *(Atom *)p;
		XFree(p);
	}
	return atom;
}

long
getstate(Window w)
{
	int format;
	long result = -1;
	unsigned char *p = NULL;
	unsigned long n, extra;
	Atom real;

	if (XGetWindowProperty(dpy, w, wmatom[WMState], 0L, 2L, false, wmatom[WMState],
		&real, &format, &n, &extra, (unsigned char **)&p) != Success)
		return -1;
	if (n != 0)
		result = *p;
	XFree(p);
	return result;
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
keypress(XEvent *evt)
{
	char hotkey = 0;
	KeySym keysym = XkbKeycodeToKeysym(dpy,
			(KeyCode)evt->xkey.keycode, 0, 0);
	for (size_t i = 0; i < LENGTH(keys); ++i)
		if (keysym == keys[i].keysym && keys[i].func != NULL &&
				CLEAN(keys[i].mod) == CLEAN(evt->xkey.state)) {
			keys[i].func(keys[i].arg);
			hotkey = 1;
		}

	if (hotkey || exec == (size_t)-1)
		return;

	/* preserve uppercase keysym information */
	keysym = XkbKeycodeToKeysym(dpy, evt->xkey.keycode, 0,
			(evt->xkey.state & ShiftMask) != 0);
	switch (keysym) {
	case XK_Return:
		char *ptrs[8] = { 0 }; ptrs[0] = execa;
		for (size_t i = 0, j = 1; i < exec; ++i)
			if (execa[i] == ' ') {
				execa[i] = '\0';
				ptrs[j++] = &execa[i + 1];
			}
		Arg arg = { .v = ptrs };
		spawn(arg);
	case XK_Escape: /* FALLTHROUGH */
		exec = -1, execa[0] = '\0';
		grabkeys(dpy);
		break;
	case XK_BackSpace:
		size_t i = 0;
		while (exec - i > 0 && (execa[exec - ++i] & 0xc0) ==0x80);
		execa[(exec -= i)] = '\0';
		break;
	default:
		exec += xkb_keysym_to_utf8(keysym, &execa[exec],
				sizeof(execa) - exec) - 1;
	}
	drawbar(selmon);
}

void
manage(Window w, XWindowAttributes *wa)
{
	Client *c, *t = NULL;
	Window trans = None;
	XWindowChanges wc;

	c = scalloc(1, sizeof(Client));
	c->win = w;
	/* geometry */
	c->x = c->oldx = wa->x;
	c->y = c->oldy = wa->y;
	c->w = c->oldw = wa->width;
	c->h = c->oldh = wa->height;

	updatetitle(c);
	if (XGetTransientForHint(dpy, w, &trans) && (t = wintoclient(trans))) {
		c->mon = t->mon;
		c->tags = t->tags;
	} else {
		c->mon = selmon;
		c->tags = c->mon->tags;
	}

	if (c->x + WIDTH(c) > c->mon->mx + c->mon->mw)
		c->x = c->mon->mx + c->mon->mw - WIDTH(c);
	if (c->y + HEIGHT(c) > c->mon->my + c->mon->mh)
		c->y = c->mon->my + c->mon->mh - HEIGHT(c);
	c->x = MAX(c->x, c->mon->mx);
	/* only fix client y-offset, if the client center might cover the bar */
	c->y = MAX(c->y, ((c->mon->by == c->mon->my) && (c->x + (c->w / 2) >= c->mon->wx)
		&& (c->x + (c->w / 2) < c->mon->wx + c->mon->ww)) ? (drw->fonts->h + 2) : c->mon->my);

	wc.border_width = borderw;
	XConfigureWindow(dpy, w, CWBorderWidth, &wc);
	XSetWindowBorder(dpy, w, scheme[ClrNorm][ColBorder].pixel);
	configure(c); /* propagates border_width, if size doesn't change */
	updatewindowtype(c);
	updatesizehints(c);
	updatewmhints(c);
	XSelectInput(dpy, w, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
	grabbuttons(dpy, c, 0);
	if (!c->isfloating)
		c->isfloating = c->oldstate = trans != None || c->isfixed;
	if (c->isfloating)
		XRaiseWindow(dpy, c->win);
	c->next = c->mon->clients; c->mon->clients = c;
	c->snext = c->mon->stack; c->mon->stack = c;
	XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32, PropModeAppend,
		(unsigned char *) &(c->win), 1);
	XMoveResizeWindow(dpy, c->win, c->x + 2 * sw, c->y, c->w, c->h); /* some windows require this */
	setclientstate(c, NormalState);
	if (c->mon == selmon)
		unfocus(selmon->sel, 0);
	c->mon->sel = c;
	arrange(c->mon);
	XMapWindow(dpy, c->win);
	focus(NULL);
}

void
mappingnotify(XEvent *e)
{
	XMappingEvent *ev = &e->xmapping;

	XRefreshKeyboardMapping(ev);
	if (ev->request == MappingKeyboard)
		grabkeys(dpy);
}

void
maprequest(XEvent *e)
{
	static XWindowAttributes wa;
	XMapRequestEvent *ev = &e->xmaprequest;

	if (!XGetWindowAttributes(dpy, ev->window, &wa))
		return;
	if (wa.override_redirect)
		return;
	if (!wintoclient(ev->window))
		manage(ev->window, &wa);
}

void
motionnotify(XEvent *e)
{
	static Monitor *mon = NULL;
	Monitor *m;
	XMotionEvent *ev = &e->xmotion;

	if (ev->window != root)
		return;
	if ((m = recttomon(ev->x_root, ev->y_root, 1, 1)) != mon && mon) {
		unfocus(selmon->sel, 1);
		selmon = m;
		focus(NULL);
	}
	mon = m;
}

Client *
nexttiled(Client *c)
{
	for (; c && (c->isfloating || !VISIBLE(c)); c = c->next);
	return c;
}

void
pop(Client *c)
{
	detach(c);
	c->next = c->mon->clients; c->mon->clients = c;
	focus(c);
	arrange(c->mon);
}

void
propertynotify(XEvent *e)
{
	Client *c;
	Window trans;
	XPropertyEvent *ev = &e->xproperty;

	if ((ev->window == root) && (ev->atom == XA_WM_NAME)) {
		if (!gettextprop(root, XA_WM_NAME, stext, sizeof(stext)))
			strcpy(stext, "dwm");
		drawbar(selmon);
	} else if (ev->state != PropertyDelete && (c = wintoclient(ev->window))) {
		switch(ev->atom) {
		default: break;
		case XA_WM_TRANSIENT_FOR:
			if (!c->isfloating && (XGetTransientForHint(dpy, c->win, &trans)) &&
				(c->isfloating = (wintoclient(trans)) != NULL))
				arrange(c->mon);
			break;
		case XA_WM_NORMAL_HINTS:
			updatesizehints(c);
			break;
		case XA_WM_HINTS:
			updatewmhints(c);
			drawbars();
			break;
		}
		if (ev->atom == XA_WM_NAME || ev->atom == netatom[NetWMName]) {
			updatetitle(c);
			if (c == c->mon->sel)
				drawbar(c->mon);
		}
		if (ev->atom == netatom[NetWMWindowType])
			updatewindowtype(c);
	}
}

void
resize(Client *c, int x, int y, int w, int h, int interact)
{
	if (applysizehints(c, &x, &y, &w, &h, interact))
		resizeclient(c, x, y, w, h);
}

void
resizeclient(Client *c, int x, int y, int w, int h)
{
	XWindowChanges wc;

	c->oldx = c->x; c->x = wc.x = x;
	c->oldy = c->y; c->y = wc.y = y;
	c->oldw = c->w; c->w = wc.width = w;
	c->oldh = c->h; c->h = wc.height = h;
	wc.border_width = c->isfullscreen ? 0 : borderw;
	XConfigureWindow(dpy, c->win, CWX|CWY|CWWidth|CWHeight|CWBorderWidth, &wc);
	configure(c);
	XSync(dpy, false);
}

void
restack(Monitor *m)
{
	Client *c;
	XEvent ev;
	XWindowChanges wc;

	drawbar(m);
	if (!m->sel)
		return;
	if (m->sel->isfloating)
		XRaiseWindow(dpy, m->sel->win);
	wc.stack_mode = Below;
	wc.sibling = m->barwin;
	for (c = m->stack; c; c = c->snext)
		if (!c->isfloating && VISIBLE(c)) {
			XConfigureWindow(dpy, c->win, CWSibling|CWStackMode, &wc);
			wc.sibling = c->win;
		}
	XSync(dpy, false);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}

void
sendmon(Client *c, Monitor *m)
{
	if (c->mon == m)
		return;
	unfocus(c, 1);
	detach(c);
	detachstack(c);
	c->mon = m;
	c->tags = m->tags; /* assign tags of target monitor */
	c->next = c->mon->clients; c->mon->clients = c;
	c->snext = c->mon->stack; c->mon->stack = c;
	focus(NULL);
	arrange(NULL);
}

void
setclientstate(Client *c, long state)
{
	long data[] = { state, None };

	XChangeProperty(dpy, c->win, wmatom[WMState], wmatom[WMState], 32,
		PropModeReplace, (unsigned char *)data, 2);
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
			(unsigned char *) &(c->win), 1);
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
		arrange(c->mon);
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
showhide(Client *c)
{
	if (!c)
		return;
	if (VISIBLE(c)) {
		/* show clients top down */
		XMoveWindow(dpy, c->win, c->x, c->y);
		if (c->isfloating && !c->isfullscreen)
			resize(c, c->x, c->y, c->w, c->h, 0);
		showhide(c->snext);
	} else {
		/* hide clients bottom up */
		showhide(c->snext);
		XMoveWindow(dpy, c->win, WIDTH(c) * -2, c->y);
	}
}

void
sigchld(int unused)
{
	if (signal(SIGCHLD, sigchld) == SIG_ERR)
		die("can't install SIGCHLD handler:");
	while (0 < waitpid(-1, NULL, WNOHANG));
}

void
tile(Monitor *mon)
{
	unsigned int totcli; /* total number of clients */
	Client *client = nexttiled(mon->clients);
	for (totcli = 0; client != NULL; ++totcli)
		client = nexttiled(client->next);
	client = nexttiled(mon->clients);

	unsigned int width; /* width of master/fullwidth windows */
	if (totcli > mon->nmaster) /* master and stacking windows separate */
		width = (mon->ww - 2*GAPOH*gap - GAPIH*gap) * mon->mfact / 100;
	else /* master takes up entire monitor */
		width = mon->ww - 2*GAPOH*gap;

	/* master/fullwidth windows handler */
	unsigned int clinum = 0, totgap = 0; /* client number and spacing */
	for (; clinum < mon->nmaster && client != NULL; ++clinum,
			client = nexttiled(client->next)) {
		unsigned int numdown = MIN(totcli, mon->nmaster) - clinum;
		unsigned int height = (mon->wh - totgap - 2*GAPOV*gap -
				GAPIV*gap * (numdown - 1)) / numdown;
		resize(client, mon->wx + GAPOH*gap, mon->wy + totgap +
				GAPOV*gap, width - 2*borderw,
				height - 2*borderw, 0);

		if (totgap + HEIGHT(client) + GAPIH*gap < mon->wh)
			totgap += HEIGHT(client) + GAPIH*gap;
	}
	totgap = 0; /* reset for next handler to use */

	/* stacking windows handler */
	for (; client != NULL; ++clinum, client = nexttiled(client->next)) {
		unsigned int numdown = totcli - clinum;
		unsigned int height = (mon->wh - totgap - 2*GAPOV*gap -
				GAPIV*gap * (numdown - 1)) / numdown;
		resize(client, mon->wx + GAPOH*gap + width + GAPIH*gap,
				mon->wy + totgap + GAPOH*gap,
				mon->ww - width - 2*borderw -
				2*GAPOV*gap - GAPIV*gap,
				height - 2*borderw, 0);

		if (totgap + HEIGHT(client) + GAPIH*gap < mon->wh)
			totgap += HEIGHT(client) + GAPIH*gap;
	}
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
unmanage(Client *c, int destroyed)
{
	Monitor *m = c->mon;
	XWindowChanges wc;

	detach(c);
	detachstack(c);
	if (!destroyed) {
		wc.border_width = borderw;
		XGrabServer(dpy); /* avoid race conditions */
		XSetErrorHandler(xetemp);
		XConfigureWindow(dpy, c->win, CWBorderWidth, &wc); /* restore border */
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		setclientstate(c, WithdrawnState);
		XSync(dpy, false);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}
	free(c);
	focus(NULL);
	updateclientlist();
	arrange(m);
}

void
unmapnotify(XEvent *e)
{
	Client *c;
	XUnmapEvent *ev = &e->xunmap;

	if ((c = wintoclient(ev->window))) {
		if (ev->send_event)
			setclientstate(c, WithdrawnState);
		else
			unmanage(c, 0);
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
	XClassHint ch = {"dwm", "dwm"};
	for (m = mons; m; m = m->next) {
		if (m->barwin)
			continue;
		m->barwin = XCreateWindow(dpy, root, m->wx, m->by, m->ww, (drw->fonts->h + 2), 0, DefaultDepth(dpy, screen),
				CopyFromParent, DefaultVisual(dpy, screen),
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
		m->wh -= (drw->fonts->h + 2);
		if (topbar)
			m->by = m->wy, m->wy += (drw->fonts->h + 2);
		else
			m->by = m->wy + m->wh;
	} else
		m->by = -(drw->fonts->h + 2);
}

void
updateclientlist()
{
	Client *c;
	Monitor *m;

	XDeleteProperty(dpy, root, netatom[NetClientList]);
	for (m = mons; m; m = m->next)
		for (c = m->clients; c; c = c->next)
			XChangeProperty(dpy, root, netatom[NetClientList],
				XA_WINDOW, 32, PropModeAppend,
				(unsigned char *) &(c->win), 1);
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
		if (n <= nn) { /* new monitors available */
			for (i = 0; i < (nn - n); i++) {
				for (m = mons; m && m->next; m = m->next);
				if (m)
					m->next = createmon();
				else
					mons = createmon();
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
		} else { /* less monitors available nn < n */
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
				cleanupmon(m);
			}
		}
		free(unique);
	} else
#endif /* XINERAMA */
	{ /* default monitor setup */
		if (!mons)
			mons = createmon();
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
}

void
updatetitle(Client *c)
{
	if (!gettextprop(c->win, netatom[NetWMName], c->name, sizeof c->name))
		gettextprop(c->win, XA_WM_NAME, c->name, sizeof c->name);
}

void
updatewindowtype(Client *c)
{
	Atom state = getatomprop(c, netatom[NetWMState]);
	Atom wtype = getatomprop(c, netatom[NetWMWindowType]);

	if (state == netatom[NetWMFullscreen])
		setfullscreen(c, 1);
	if (wtype == netatom[NetWMWindowTypeDialog])
		c->isfloating = 1;
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
		if (wmh->flags & InputHint)
			c->neverfocus = !wmh->input;
		else
			c->neverfocus = 0;
		XFree(wmh);
	}
}

int
main(int argc, char **argv)
{
	if ((dpy = XOpenDisplay(NULL)) == NULL)
		die("swim: unable to open display\n");
	chkwm(dpy);

	sigchld(0); /* reap zombies */

	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	sw = DisplayWidth(dpy, screen);
	sh = DisplayHeight(dpy, screen);

	drw = drw_create(dpy, screen, root, sw, sh);
	if (!drw_fontset_create(drw, fonts, LENGTH(fonts)))
		die("swim: unable to create fonts\n");

	updategeom();

	Atom utf8string = XInternAtom(dpy, "UTF8_STRING", false);
	wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", false);
	wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", false);
	wmatom[WMState] = XInternAtom(dpy, "WM_STATE", false);
	wmatom[WMTakeFocus] = XInternAtom(dpy, "WM_TAKE_FOCUS", false);
	netatom[NetActiveWindow] = XInternAtom(dpy,
			"_NET_ACTIVE_WINDOW", false);
	netatom[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", false);
	netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", false);
	netatom[NetWMState] = XInternAtom(dpy, "_NET_WM_STATE", false);
	netatom[NetWMCheck] = XInternAtom(dpy,
			"_NET_SUPPORTING_WM_CHECK", false);
	netatom[NetWMFullscreen] = XInternAtom(dpy,
			"_NET_WM_STATE_FULLSCREEN", false);
	netatom[NetWMWindowType] = XInternAtom(dpy,
			"_NET_WM_WINDOW_TYPE", false);
	netatom[NetWMWindowTypeDialog] = XInternAtom(dpy,
			"_NET_WM_WINDOW_TYPE_DIALOG", false);
	netatom[NetClientList] = XInternAtom(dpy, "_NET_CLIENT_LIST", false);

	cursor = XCreateFontCursor(dpy, XC_left_ptr);

	scheme = scalloc(LENGTH(colors), sizeof(Clr *));
	for (size_t i = 0; i < LENGTH(colors); i++)
		scheme[i] = drw_scm_create(drw, colors[i], 3);

	updatebars();
	if (!gettextprop(root, XA_WM_NAME, stext, sizeof(stext)))
		strcpy(stext, "dwm");
	drawbars();

	Window wmcheckwin = XCreateSimpleWindow(dpy, root,
			0, 0, 1, 1, 0, 0, 0);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMName], utf8string, 8,
		PropModeReplace, (unsigned char *) "dwm", 3);
	XChangeProperty(dpy, root, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	/* EWMH support per view */
	XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
		PropModeReplace, (unsigned char *) netatom, NetLast);
	XDeleteProperty(dpy, root, netatom[NetClientList]);
	
	/* select events */
	XSetWindowAttributes sattrs = {
		.cursor = cursor,
		.event_mask = ButtonPressMask | EnterWindowMask |
				LeaveWindowMask | PointerMotionMask |
				PropertyChangeMask | StructureNotifyMask |
				SubstructureNotifyMask |
				SubstructureRedirectMask
	};
	XChangeWindowAttributes(dpy, root, CWEventMask|CWCursor, &sattrs);
	XSelectInput(dpy, root, sattrs.event_mask);
	grabkeys(dpy);
	focus(NULL);

	XWindowAttributes attrs;
	Window win1, win2, *wins = NULL;
	uint num;

	if (XQueryTree(dpy, root, &win1, &win2, &wins, &num) == 0)
		goto scanps;
	for (uint i = 0; i < num; ++i) {
		if (XGetWindowAttributes(dpy, wins[i], &attrs) == 0 ||
				attrs.override_redirect ||
				XGetTransientForHint(dpy, wins[i], &win1))
			continue;
		if (attrs.map_state == IsViewable ||
				getstate(wins[i]) == IconicState)
			manage(wins[i], &attrs);
	}
	for (uint i = 0; i < num; ++i) { /* handle transients */
		if (XGetWindowAttributes(dpy, wins[i], &attrs) == 0)
			continue;
		if (XGetTransientForHint(dpy, wins[i], &win1) &&
				(attrs.map_state == IsViewable ||
				getstate(wins[i]) == IconicState))
			manage(wins[i], &attrs);
	}
	if (wins)
		XFree(wins);

scanps:
	XSync(dpy, false);

	XEvent evt;
	void (*handler[LASTEvent])(XEvent *) = {
		[ButtonPress] = buttonpress,
		[ClientMessage] = clientmessage,
		[ConfigureRequest] = configurerequest,
		[ConfigureNotify] = configurenotify,
		[DestroyNotify] = destroynotify,
		[EnterNotify] = enternotify,
		[Expose] = expose,
		[FocusIn] = focusin,
		[KeyPress] = keypress,
		[MappingNotify] = mappingnotify,
		[MapRequest] = maprequest,
		[MotionNotify] = motionnotify,
		[PropertyNotify] = propertynotify,
		[UnmapNotify] = unmapnotify
	};
	while (running && XNextEvent(dpy, &evt) == 0)
		if (handler[evt.type])
			handler[evt.type](&evt);

	Arg a = { .n = ~0};
	Monitor *m;
	size_t iter;

	view(a);
	for (m = mons; m; m = m->next)
		while (m->stack)
			unmanage(m->stack, 0);
	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	while (mons)
		cleanupmon(mons);
	XFreeCursor(dpy, cursor);
	for (iter = 0; iter < LENGTH(colors); iter++)
		free(scheme[iter]);
	XDestroyWindow(dpy, wmcheckwin);
	drw_free(drw);
	XSync(dpy, false);
	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
	XDeleteProperty(dpy, root, netatom[NetActiveWindow]);

	XCloseDisplay(dpy);
}
