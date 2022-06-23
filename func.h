/* swim - simple window manager
 * Copyright (C) 2022 ArcNyxx
 * see LICENCE file for licensing information */

#ifndef FUNC_H
#define FUNC_H

#include <X11/Xlib.h>

#include "struct.h"

int applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact);
void cleanupmon(Monitor *mon);
void configure(Client *c);
Monitor *createmon(void);
void detach(Client *c);
void detachstack(Client *c);
void focus(Client *c);
Atom getatomprop(Client *c, Atom prop);
long getstate(Window w);
int gettextprop(Window w, Atom atom, char *text, unsigned int size);
void manage(Window w, XWindowAttributes *wa);
Client *nexttiled(Client *c);
void pop(Client *);
void resize(Client *c, int x, int y, int w, int h, int interact);
void resizeclient(Client *c, int x, int y, int w, int h);
void restack(Monitor *m);
int sendevent(Client *c, Atom proto);
void sendmon(Client *c, Monitor *m);
void setfocus(Client *c);
void setfullscreen(Client *c, int fullscreen);
void seturgent(Client *c, int urg);
void unfocus(Client *c, int setfocus);
void unmanage(Client *c, int destroyed);
void updatebarpos(Monitor *m);
void updatebars(void);
int updategeom(void);
void updatesizehints(Client *c);
void updatewindowtype(Client *c);
void updatewmhints(Client *c);

#endif /* FUNC_H */
