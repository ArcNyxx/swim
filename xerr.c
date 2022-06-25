/* swim - simple window manager
 * Copyright (C) 2022 ArcNyxx
 * see LICENCE file for licensing information */

#include <stdbool.h>
#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xproto.h>

#include "util.h"
#include "xerr.h"

static int (*xeorig)(Display *, XErrorEvent *);
static int xerror(Display *dpy, XErrorEvent *evt);

static int
xchkwm(Display *dpy, XErrorEvent *evt)
{
	die("swim: another window manager already running\n");
	return 1;
}

static int
xerror(Display *dpy, XErrorEvent *evt)
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
	return xeorig(dpy, evt); /* may exit */
}

void
chkwm(Display *dpy)
{
	xeorig = XSetErrorHandler(xchkwm);
	XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask);
	XSync(dpy, false);
	XSetErrorHandler(xerror);
	XSync(dpy, false);
}
