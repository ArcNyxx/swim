/* swim - simple window manager
 * Copyright (C) 2022 ArcNyxx
 * see LICENCE file for licensing information */

#ifndef FUNC_H
#define FUNC_H

#include <stdbool.h>

#include <X11/Xlib.h>

#include "struct.h"

void configure(Client *c);
void detach(Client *c);
void detachstack(Client *c);
void focus(Client *c);
void resizeclient(Client *c, int x, int y, int w, int h);
void restack(Monitor *m);
int sendevent(Client *c, Atom proto);
void setfocus(Client *c);
void setfullscreen(Client *c, int fullscreen);
void seturgent(Client *c, int urg);
void unfocus(Client *c, int setfocus);
void updatebarpos(Monitor *m);
bool updategeom(void);
void updatesizehints(Client *c);

#endif /* FUNC_H */
