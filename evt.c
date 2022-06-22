/* swim - simple window manager
 * Copyright (C) 2021-2022 ArcNyxx
 * see LICENCE file for licensing information */

#include <stdbool.h>
#include <stdio.h> //TODO

#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <xkbcommon/xkbcommon.h>

#include "act.h"
#include "config.h"
#include "conv.h"
#include "drw.h"
#include "evt.h"
#include "grab.h"
#include "struct.h"
#include "util.h"

static void buttonpress      (XEvent *evt);
static void clientmessage    (XEvent *evt);
static void configurenotify  (XEvent *evt);
static void configurerequest (XEvent *evt);
static void destroynotify    (XEvent *evt);
static void enternotify      (XEvent *evt);
static void expose           (XEvent *evt);
static void focusin          (XEvent *evt);
static void keypress         (XEvent *evt);
static void mappingnotify    (XEvent *evt);
static void maprequest       (XEvent *evt);
static void motionnotify     (XEvent *evt);
static void propertynotify   (XEvent *evt);
static void unmapnotify      (XEvent *evt);

void unfocus(Client *c, int setfocus);
void focus(Client *c);
void restack(Monitor *m);
void setfullscreen(Client *c, int fullscreen);
void seturgent(Client *c, int urg);
int updategeom(void);
void arrange(Monitor *m);
void updatebars(void);
void resizeclient(Client *c, int x, int y, int w, int h);
void configure(Client *c);
void unmanage(Client *c, int destroyed);
void setfocus(Client *c);
void drawbar(Monitor *m);
void manage(Window w, XWindowAttributes *wa);
int gettextprop(Window w, Atom atom, char *text, unsigned int size);
void drawbars(void);
void setclientstate(Client *c, long state);
void updatesizehints(Client *c);
void updatetitle(Client *c);
void updatewindowtype(Client *c);
void updatewmhints(Client *c);

int exec = -1;
char stext[256], execa[256] = { 0 };
bool running = true;

extern Window root;
extern Display *dpy;
extern Monitor *selmon, *mons;
extern Drw *drw;
extern Atom wmatom[WMLast], netatom[NetLast];
extern int sw, sh;	   /* X display screen geometry width, height */

static void
buttonpress(XEvent *evt)
{
fprintf(stderr, "buttonpress\n");
	Monitor *mon;
	Client *client;
	Arg arg = { .n = 0 };
	int click = ClkRootWin;
	XButtonPressedEvent *bpe = &evt->xbutton;

	if ((mon = wintomon(bpe->window)) != NULL && mon != selmon) {
		unfocus(selmon->sel, 0);
		selmon = mon;
		focus(NULL);
	}

	if (bpe->window == selmon->barwin) {
		int i = 0, x = 0;
		do
			x += TEXTW(drw, tags[i]) + drw->fonts->h;
		while (bpe->x >= x && ++i < LENGTH(tags));
		if (i < LENGTH(tags))
			click = ClkTagBar, arg.n = 1 << i;
		else if (bpe->x > selmon->ww - TEXTW(drw, stext) + drw->fonts->h)
			click = ClkStatusText;
		else
			click = ClkWinTitle;
	} else if ((client = wintoclient(bpe->window)) != NULL) {
		focus(client);
		restack(selmon);
		XAllowEvents(dpy, ReplayPointer, CurrentTime);
		click = ClkClientWin;
	}

	for (int i = 0; i < LENGTH(buttons); ++i)
		if (CLEAN(buttons[i].mask) == CLEAN(bpe->state) &&
				buttons[i].button == bpe->button &&
				buttons[i].click == click &&
				buttons[i].func != NULL) {
			if (click == ClkTagBar && buttons[i].arg.n == 0)
				buttons[i].func(arg);
			else
				buttons[i].func(buttons[i].arg);
		}
}

static void
clientmessage(XEvent *evt)
{
fprintf(stderr, "clientmessage\n");
	Client *client;
	XClientMessageEvent *cme = &evt->xclient;
	if ((client = wintoclient(cme->window)) == NULL)
		return;
	if (cme->message_type == netatom[NetWMState]) {
		if (cme->data.l[1] == netatom[NetWMFullscreen] ||
				cme->data.l[2] == netatom[NetWMFullscreen])
			setfullscreen(client, (!client->isfullscreen &&
					cme->data.l[0] == 2) ||
					cme->data.l[0] == 1);
	} else if (cme->message_type == netatom[NetActiveWindow]) {
		if (client != selmon->sel && !client->isurgent)
			seturgent(client, 1);
	}
}

static void
configurenotify(XEvent *evt)
{
fprintf(stderr, "configurenotify\n");
	XConfigureEvent *cfe = &evt->xconfigure;
	if (cfe->window != root)
		return;
	int dirty = (sw != cfe->width || sh != cfe->height);
	sw = cfe->width, sh = cfe->height;

	if (updategeom() || dirty) {
		drw_resize(drw, sw, drw->fonts->h + 2);
		updatebars();
		for (Monitor *mon = mons; mon != NULL; mon = mon->next) {
			for (Client *client = mon->clients; client != NULL;
					client = client->next)
				if (client->isfullscreen)
					resizeclient(client, mon->mx, mon->my,
							mon->mw, mon->mh);
			XMoveResizeWindow(dpy, mon->barwin, mon->wx, mon->by,
					mon->ww, drw->fonts->h + 2);
		}
		focus(NULL);
		arrange(NULL);
	}
}

static void
configurerequest(XEvent *evt)
{
fprintf(stderr, "configurerequest\n");
	Client *c;
	XConfigureRequestEvent *cre = &evt->xconfigurerequest;
	if ((c = wintoclient(cre->window)) != NULL) {
		if (c->isfloating) {
			if (cre->value_mask & CWX)
				c->oldx = c->x, c->x = c->mon->mx + cre->x;
			if (cre->value_mask & CWY)
				c->oldy = c->y, c->y = c->mon->my + cre->y;
			if (cre->value_mask & CWWidth)
				c->oldw = c->w, c->w = cre->width;
			if (cre->value_mask & CWHeight)
				c->oldh = c->h, c->h = cre->height;

			if ((c->x + c->w) > c->mon->mx + c->mon->mw)
				c->x = c->mon->mx + (c->mon->mw / 2 -
						WIDTH(c) / 2); /* centre x */
			if ((c->y + c->h) > c->mon->my + c->mon->mh)
				c->y = c->mon->my + (c->mon->mh / 2 -
						WIDTH(c) / 2); /* centre y */
			if ((cre->value_mask & (CWWidth | CWHeight)) == 0 &&
					cre->value_mask & (CWX | CWY))
				configure(c);
			if (VISIBLE(c))
				XMoveResizeWindow(dpy, c->win,
						c->x, c->y, c->w, c->h);
		} else if ((cre->value_mask & CWBorderWidth) == 0) {
			configure(c);
		}
	} else {
		XWindowChanges wc = {
			.x = cre->x, .y = cre->y, .height = cre->height,
			.sibling = cre->above, .stack_mode = cre->detail,
			.border_width = cre->border_width, .width = cre->width
		};
		XConfigureWindow(dpy, cre->window, cre->value_mask, &wc);
	}
	XSync(dpy, false);
}

static void
destroynotify(XEvent *evt)
{
fprintf(stderr, "destroynotify\n");
	Client *client;
	if ((client = wintoclient(evt->xdestroywindow.window)) != NULL)
		unmanage(client, 1);
}

static void
enternotify(XEvent *evt)
{
fprintf(stderr, "enternotify\n");
	XCrossingEvent *cre = &evt->xcrossing;
	if ((cre->mode != NotifyNormal || cre->detail == NotifyInferior) &&
			cre->window != root)
		return;

	Client *client = wintoclient(cre->window);
	Monitor *mon = client != NULL ? client->mon : wintomon(cre->window);
	if (mon != selmon) {
		unfocus(selmon->sel, 1);
		selmon = mon;
	} else if (client != NULL && client != selmon->sel) {
		focus(client);
	}
}

static void
expose(XEvent *evt)
{
fprintf(stderr, "expose\n");
	Monitor *mon;
	if (evt->xexpose.count == 0 &&
			(mon = wintomon(evt->xexpose.window)) != NULL)
		drawbar(mon);
}

static void
focusin(XEvent *evt)
{
fprintf(stderr, "focusin\n");
	if (selmon->sel != NULL && evt->xfocus.window != selmon->sel->win)
		setfocus(selmon->sel); /* handles broken clients */
}

static void
keypress(XEvent *evt)
{
fprintf(stderr, "keypress\n");
	bool hotkey = false;
	XKeyEvent *kpe = &evt->xkey;
	KeySym keysym = XkbKeycodeToKeysym(dpy, kpe->keycode, 0, 0);
	for (int i = 0; i < LENGTH(keys); ++i)
		if (keysym == keys[i].keysym && keys[i].func != NULL &&
				CLEAN(keys[i].mod) == CLEAN(kpe->state)) {
			keys[i].func(keys[i].arg);
			hotkey = true;
		}
	if (hotkey || exec == -1)
		return;

	/* presetve uppercase keysym information */
	keysym = XkbKeycodeToKeysym(dpy, kpe->keycode,
			0, (kpe->state & ShiftMask) != 0);
	switch (keysym) {
	case XK_Return:
		spawn((Arg){ .v = (void *[]){ execa, NULL } });
	case XK_Escape: /* FALLTHROUGH */
		exec = -1, execa[0] = '\0';
		grabkeys(dpy);
		break;
	case XK_BackSpace:
		while (exec > 0 && (execa[--exec] & 0xC0) == 0x80);
		execa[exec] = '\0';
		break;
	default:
		exec += xkb_keysym_to_utf8(keysym, &execa[exec],
				sizeof(execa) - exec) - 1;
		break;
	}
	drawbar(selmon);
}

static void
mappingnotify(XEvent *evt)
{
fprintf(stderr, "mappingnotify\n");
	XRefreshKeyboardMapping(&evt->xmapping);
	if (evt->xmapping.request == MappingKeyboard)
		grabkeys(dpy);
}

static void
maprequest(XEvent *evt)
{
fprintf(stderr, "maprequest\n");
	static XWindowAttributes wa;
	if (XGetWindowAttributes(dpy, evt->xmaprequest.window, &wa) &&
			!wa.override_redirect &&
			wintoclient(evt->xmaprequest.window) == NULL)
		manage(evt->xmaprequest.window, &wa);
}

static void
motionnotify(XEvent *evt)
{
fprintf(stderr, "motionnotify\n");
	Monitor *newmon;
	static Monitor *mon = NULL;

	if (evt->xmotion.window != root)
		return;
	if ((newmon = recttomon(evt->xmotion.x_root, evt->xmotion.y_root,
				1, 1)) != mon && mon != NULL) {
		unfocus(selmon->sel, 1);
		selmon = newmon;
		focus(NULL);
	}
	mon = newmon;
}

static void
propertynotify(XEvent *evt)
{
fprintf(stderr, "propertynotify\n");
	Client *c;
	XPropertyEvent *pre = &evt->xproperty;
	if (pre->window == root && pre->atom == XA_WM_NAME) {
		gettextprop(root, XA_WM_NAME, stext, sizeof(stext));
		drawbar(selmon);
	} else if (pre->state != PropertyDelete &&
			(c = wintoclient(pre->window)) != NULL) {
		Window tr;
		if (pre->atom == XA_WM_TRANSIENT_FOR && !c->isfloating &&
				XGetTransientForHint(dpy, c->win, &tr) &&
				(c->isfloating = wintoclient(tr) != NULL)) {
			arrange(c->mon);
		} else if (pre->atom == XA_WM_NORMAL_HINTS) {
			updatesizehints(c);
		} else if (pre->atom == XA_WM_HINTS) {
			updatewmhints(c);
			drawbars();
		}

		if (pre->atom == XA_WM_NAME ||
				pre->atom == netatom[NetWMName]) {
			updatetitle(c);
			if (c == c->mon->sel)
				drawbar(c->mon);
		}
		if (pre->atom == netatom[NetWMWindowType])
			updatewindowtype(c);
	}
}

static void
unmapnotify(XEvent *evt)
{
fprintf(stderr, "unmapnotify\n");
	Client *client;
	if ((client = wintoclient(evt->xunmap.window)) != NULL) {
		if (evt->xunmap.window)
			setclientstate(client, WithdrawnState);
		else
			unmanage(client, 0);
	}
}

void
handle_events(Display *dpy)
{
	typedef void (*Event)(XEvent *);
	static const Event events[LASTEvent] = {
		[ButtonPress]      = buttonpress,
		[ClientMessage]    = clientmessage,
		[ConfigureRequest] = configurerequest,
		[ConfigureNotify]  = configurenotify,
		[DestroyNotify]    = destroynotify,
		[EnterNotify]      = enternotify,
		[Expose]           = expose,
		[FocusIn]          = focusin,
		[KeyPress]         = keypress,
		[MappingNotify]    = mappingnotify,
		[MapRequest]       = maprequest,
		[MotionNotify]     = motionnotify,
		[PropertyNotify]   = propertynotify,
		[UnmapNotify]      = unmapnotify
	};

	XEvent evt;
	while (running && XNextEvent(dpy, &evt) == 0)
		if (events[evt.type] != NULL)
			events[evt.type](&evt);
}
