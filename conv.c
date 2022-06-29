/* swim - simple window manager
 * Copyright (C) 2022 ArcNyxx
 * see LICENCE file for licensing information */

#include <X11/Xlib.h>

#include "conv.h"
#include "struct.h"
#include "util.h"

extern Display *dpy;
extern Monitor *sel, *mons;

Monitor *
rectomon(int x, int y, int w, int h)
{
	int area, lastarea = 0;
	Monitor *mon, *save = sel;
	for (mon = mons; mon != NULL; mon = mon->next)
		if ((area = MAX(0, MIN(x + w, mon->wx + mon->ww) -
				MAX(x, mon->wx)) *
				MAX(0, MIN(y + h, mon->wy + mon->wh) -
				MAX(y, mon->wy))) > lastarea)
			lastarea = area, save = mon;
	return save;
}

Monitor *
dirtomon(int dir)
{
	Monitor *mon = NULL;
	if (dir > 0) {
		if ((mon = sel->next) == NULL)
			mon = mons;
	} else if (sel == mons) {
		for (mon = mons; mon->next != NULL; mon = mon->next);
	} else {
		for (mon = mons; mon->next != sel; mon = mon->next);
	}
	return mon;
}

Client *
wintocli(Window win)
{
	for (Monitor *mon = mons; mon != NULL; mon = mon->next)
		for (Client *client = mon->clients;
				client != NULL; client = client->next)
			if (client->win == win)
				return client;
	return NULL;
}

Monitor *
wintomon(Window win)
{
	int x, y, null;
	Client *client;
	Window dummy, root = RootWindow(dpy, DefaultScreen(dpy));

	if (win == root && XQueryPointer(dpy, root, &dummy, &dummy, &x, &y,
			&null, &null, (unsigned int *)&null))
		return rectomon(x, y, 1, 1);
	for (Monitor *mon = mons; mon != NULL; mon = mon->next)
		if (win == mon->barwin)
			return mon;
	if ((client = wintocli(win)))
		return client->mon;
	return sel;
}
