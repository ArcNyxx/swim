/* swim - simple window manager
 * Copyright (C) 2022 ArcNyxx
 * see LICENCE file for licensing information */

#ifndef CONV_H
#define CONV_H

#include "struct.h"

static Monitor *recttomon(int x, int y, int w, int h);
static Monitor *dirtomon(int dir);
static Client *wintoclient(Window win);
static Monitor *wintomon(Window win);

#endif /* CONV_H */
