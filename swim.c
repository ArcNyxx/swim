/* swim - simple window manager
 * Copyright (C) 2022 ArcNyxx
 * see LICENCE file for licensing information */

#include <X11/X.h>
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
#include <X11/Xft/Xft.h>
#include <X11/XKBlib.h>

#include "bar.h"
#include "config.h"
#include "drw.h"
#include "evt.h"
#include "grab.h"
#include "struct.h"
#include "func.h"
#include "util.h"
#include "xerr.h"

#define ATOM(name) XInternAtom(dpy, name, false)

static void sighandle(int null);

int sw, sh;
Atom wmatom[WMLast], netatom[NetLast];
Clr **scheme;
Display *dpy;
Drw *drw;
Monitor *mons, *selmon;
Window root;

static void
sighandle(int null)
{
	while (waitpid(-1, NULL, WNOHANG) > 0);
}

int
main(void)
{
	if ((dpy = XOpenDisplay(NULL)) == NULL)
		die("swim: unable to open display\n");
	chkwm(dpy);

	root = RootWindow(dpy, DefaultScreen(dpy));
	sw   = DisplayWidth(dpy, DefaultScreen(dpy));
	sh   = DisplayHeight(dpy, DefaultScreen(dpy));

	drw = drw_create(dpy, DefaultScreen(dpy), root, sw, sh);
	if (!drw_fontset_create(drw, font))
		die("swim: unable to create fonts\n");

	struct sigaction act = { .sa_handler = sighandle };
	sigemptyset(&act.sa_mask); sigaction(SIGCHLD, &act, NULL);

	updategeom();
	scheme = scalloc(LENGTH(colors), sizeof(Clr *));
	for (size_t i = 0; i < LENGTH(colors); i++)
		scheme[i] = drw_scm_create(drw, colors[i], 3);

	updatebars();
	drawbars();

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

	XSetWindowAttributes attrs = { .event_mask = ButtonPressMask |
			EnterWindowMask | LeaveWindowMask | PointerMotionMask |
			PropertyChangeMask | StructureNotifyMask |
			SubstructureNotifyMask | SubstructureRedirectMask,
			.cursor = XCreateFontCursor(dpy, XC_left_ptr) };
	XChangeWindowAttributes(dpy, root, CWEventMask | CWCursor, &attrs);
	XSelectInput(dpy, root, attrs.event_mask);
	grabkeys(dpy);
	focus(NULL);

	XSync(dpy, false);
	handle_events();
	XCloseDisplay(dpy);
}
