/* swim - simple window manager
 * Copyright (C) 2022 ArcNyxx
 * see LICENCE file for licensing information */

#ifndef GRAB_H
#define GRAB_H

#include <X11/Xlib.h>

#include "struct.h"

extern unsigned int numlock;

void grabbuttons(Display *dpy, Client *client, int focused);
void grabkeys(Display *dpy);

#endif /* GRAB_H */
