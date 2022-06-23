/* swim - simple window manager
 * Copyright (C) 2022 ArcNyxx
 * see LICENCE file for licensing information */

#ifndef STRUCT_H
#define STRUCT_H

#include <X11/Xlib.h>

enum { ClrNorm, ClrSel };
enum { ClkTagBar, ClkStatusText, ClkWinTitle, ClkClientWin, ClkRootWin };
enum { NetSupported, NetWMName, NetWMState, NetWMCheck, NetWMFullscreen,
		NetActiveWindow, NetWMWindowType, NetWMWindowTypeDialog,
		NetClientList, NetLast };
enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMLast };

typedef union  arg     Arg;
typedef struct button  Button;
typedef struct key     Key;
typedef struct client  Client;
typedef struct monitor Monitor;

union arg {
	signed long n;
	const void *v;
};

struct button {
	unsigned int click, mask, button;
	void (*func)(const Arg);
	const Arg arg;
};

struct key {
	unsigned int mod;
	KeySym keysym;
	void (*func)(const Arg);
	const Arg arg;
};

struct client {
	char name[256];
	unsigned int tags;

	Client *next, *snext;
	Monitor *mon;

	float mina, maxa;
	int x, y, w, h;
	int oldx, oldy, oldw, oldh;
	int basew, baseh, incw, inch, maxw, maxh, minw, minh, hintsvalid;
	int isfixed, isfloating, isurgent, neverfocus, oldstate, isfullscreen;
	Window win;
};

struct monitor {
	unsigned int mfact, nmaster, tags, showbar;

	Client *clients, *stack, *sel;
	Monitor *next;

	int by;               /* bar geometry */
	int mx, my, mw, mh;   /* screen size */
	int wx, wy, ww, wh;   /* window area  */
	Window barwin;
};

#endif /* STRUCT_H */
