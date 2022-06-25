/* swim - simple window manager
 * Copyright (C) 2022 ArcNyxx
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

static long getstate(Window win);
static void sighandle(int null);

int sw, sh;
Atom wmatom[WMLast], netatom[NetLast];
Cursor cursor;
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
	sw = DisplayWidth(dpy, DefaultScreen(dpy));
	sh = DisplayHeight(dpy, DefaultScreen(dpy));

	drw = drw_create(dpy, DefaultScreen(dpy), root, sw, sh);
	if (!drw_fontset_create(drw, font))
		die("swim: unable to create fonts\n");

	struct sigaction act = { .sa_handler = sighandle };
	sigemptyset(&act.sa_mask); sigaction(SIGCHLD, &act, NULL);

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
	drawbars();

	Window wmcheckwin = XCreateSimpleWindow(dpy, root,
			0, 0, 1, 1, 0, 0, 0);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	char swim[] = "swim";
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMName], utf8string, 8,
		PropModeReplace, (unsigned char *)swim, 3);
	XChangeProperty(dpy, root, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	/* EWMH support per view */
	XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
		PropModeReplace, (unsigned char *) netatom, NetLast);
	XDeleteProperty(dpy, root, netatom[NetClientList]);

	/* select events */
	XSetWindowAttributes sattrs = { .cursor = cursor, .event_mask =
			ButtonPressMask | EnterWindowMask | LeaveWindowMask |
			PointerMotionMask | PropertyChangeMask |
			StructureNotifyMask | SubstructureNotifyMask |
			SubstructureRedirectMask };
	XChangeWindowAttributes(dpy, root, CWEventMask|CWCursor, &sattrs);
	XSelectInput(dpy, root, sattrs.event_mask);
	grabkeys(dpy);
	focus(NULL);

	XSync(dpy, false);
	handle_events();
	XCloseDisplay(dpy);
}
