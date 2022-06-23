/* swim - simple window manager
 * Copyright (C) 2022 ArcNyxx
 * see LICENCE file for licensing information */

#ifndef XERR_H
#define XERR_H

#include <X11/Xlib.h>

int xetemp(Display *dpy, XErrorEvent *evt);
int xerror(Display *dpy, XErrorEvent *evt);
void chkwm(Display *dpy);

#endif /* XERR_H */
