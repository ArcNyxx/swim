/* swim - simple window manager
 * Copyright (C) 2022 ArcNyxx
 * see LICENCE file for licensing information */

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>

#include "bar.h"
#include "config.h"
#include "drw.h"
#include "evt.h"
#include "grab.h"
#include "struct.h"
#include "func.h"
#include "util.h"

#define ATOM(name) XInternAtom(dpy, name, false)

static void sighandle(int null);
static int xchkwm(Display *dpy, XErrorEvent *evt);
static int xerror(Display *dpy, XErrorEvent *evt);

static int (*xeorig)(Display *, XErrorEvent *);

int sw, sh;
Atom wmatom[WMLast], netatom[NetLast];
Clr **scheme;
Display *dpy;
Drw *drw;
Monitor *mons, *sel;
Window root;

static void
sighandle(int null)
{
	while (waitpid(-1, NULL, WNOHANG) > 0);
}

static int
xchkwm(Display *null, XErrorEvent *evt)
{
	die("swim: another window manager already running\n");
	return 1;
}

static int
xerror(Display *null, XErrorEvent *evt)
{
	switch (evt->error_code) {
	case BadMatch:
		if (evt->request_code != X_ConfigureWindow &&
				evt->request_code != X_SetInputFocus)
			break;
		return 0;
	case BadDrawable:
		if (evt->request_code != X_CopyArea &&
				evt->request_code != X_PolyFillRectangle &&
				evt->request_code != X_PolySegment &&
				evt->request_code != X_PolyText8)
			break;
		return 0;
	case BadAccess:
		if (evt->request_code != X_GrabButton &&
				evt->request_code != X_GrabKey)
			break;
		return 0;
	case BadWindow:   /* unable to check destroyed window access, */
		return 0; /* ignored especially on unmapnotify */
	default:
		break;
	}
	fprintf(stderr, "swim: fatal error: request (%d), error (%d)\n",
			evt->request_code, evt->error_code);
	return xeorig(null, evt); /* may exit */
}

int
main(void)
{
	if ((dpy = XOpenDisplay(NULL)) == NULL)
		die("swim: unable to open display\n");

	root = RootWindow(dpy, DefaultScreen(dpy));
	sw   = DisplayWidth(dpy, DefaultScreen(dpy));
	sh   = DisplayHeight(dpy, DefaultScreen(dpy));

	drw = drw_create(dpy, DefaultScreen(dpy), root, sw, sh);
	if (!drw_fontset_create(drw, font))
		die("swim: unable to create fonts\n");

	struct sigaction act = { .sa_handler = sighandle };
	sigemptyset(&act.sa_mask); sigaction(SIGCHLD, &act, NULL);

	scheme = scalloc(LENGTH(colors), sizeof(Clr *));
	for (size_t i = 0; i < LENGTH(colors); i++)
		scheme[i] = drw_scm_create(drw, colors[i], 3);

	mons = scalloc(1, sizeof(Monitor));
	mons->tags = 1, mons->mfact = mfact, mons->nmaster = nmaster,
			mons->showbar = showbar;
	updategeom(); drawbars();

	wmatom [WMProtocols]           = ATOM("WM_PROTOCOLS");
	wmatom [WMDelete]              = ATOM("WM_DELETE_WINDOW");
	wmatom [WMState]               = ATOM("WM_STATE");
	wmatom [WMTakeFocus]           = ATOM("WM_TAKE_FOCUS");
	netatom[NetActiveWindow]       = ATOM("_NET_ACTIVE_WINDOW");
	netatom[NetSupported]          = ATOM("_NET_SUPPORTED");
	netatom[NetWMName]             = ATOM("_NET_WM_NAME");
	netatom[NetWMState]            = ATOM("_NET_WM_STATE");
	netatom[NetWMCheck]            = ATOM("_NET_SUPPORTING_WM_CHECK");
	netatom[NetWMFullscreen]       = ATOM("_NET_WM_STATE_FULLSCREEN");
	netatom[NetWMWindowType]       = ATOM("_NET_WM_WINDOW_TYPE");
	netatom[NetWMWindowTypeDialog] = ATOM("_NET_WM_WINDOW_TYPE_DIALOG");
	netatom[NetClientList]         = ATOM("_NET_CLIENT_LIST");

	Window checkwin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
	XChangeProperty(dpy, checkwin, netatom[NetWMCheck], XA_WINDOW, 32,
			PropModeReplace, (unsigned char *)&checkwin, 1);
	XChangeProperty(dpy, root, netatom[NetWMCheck], XA_WINDOW, 32,
			PropModeReplace, (unsigned char *)&checkwin, 1);
	XChangeProperty(dpy, checkwin, netatom[NetWMName], ATOM("UTF8_STRING"),
			8, PropModeReplace, (const unsigned char *)"swim", 3);
	XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
			PropModeReplace, (unsigned char *)netatom, NetLast);
	XDeleteProperty(dpy, root, netatom[NetClientList]);

	/* fail if multiple wms due to substructureredirectmask selection */
	XSetWindowAttributes attrs = { .event_mask = ButtonPressMask |
			EnterWindowMask | LeaveWindowMask | PointerMotionMask |
			PropertyChangeMask | StructureNotifyMask |
			SubstructureNotifyMask | SubstructureRedirectMask,
			.cursor = XCreateFontCursor(dpy, XC_left_ptr) };
	XChangeWindowAttributes(dpy, root, CWEventMask | CWCursor, &attrs);
	xeorig = XSetErrorHandler(xchkwm);
	XSelectInput(dpy, root, attrs.event_mask);
	XSync(dpy, false);
	XSetErrorHandler(xerror);
	XSync(dpy, false);
	grabkeys(dpy);

	focus(NULL);
	handle_events();
	XCloseDisplay(dpy);
}
