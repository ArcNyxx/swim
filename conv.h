/* swim - simple window manager
 * Copyright (C) 2022 ArcNyxx
 * see LICENCE file for licensing information */

#ifndef CONV_H
#define CONV_H

#include "struct.h"

Monitor *rectomon(int x, int y, int w, int h);
Monitor *dirtomon(int dir);
Client  *wintocli(Window win);
Monitor *wintomon(Window win);

#endif /* CONV_H */
