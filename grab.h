/* swim - simple window manager
 * Copyright (C) 2021-2022 ArcNyxx
 * see LICENCE file for licensing information */

#ifndef GRAB_H
#define GRAB_H

#include <X11/Xlib.h>

#include "struct.h"

void grabbuttons(Display *dpy, Client *client, int focused);
void grabkeys(Display *dpy);

#endif /* GRAB_H */