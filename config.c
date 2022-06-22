/* swim - simple window manager
 * Copyright (C) 2022 ArcNyxx
 * see LICENCE file for licensing information */

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include "struct.h"
#include "act.h"

#define MODKEY Mod4Mask

const unsigned int borderw = 1;  /* border width of windows */
const unsigned int mfact   = 50; /* percentage factor of master area */
const unsigned int nmaster = 1;  /* number of clients in master area */
const unsigned int snap    = 32; /* mouse functions snap boundary */
const int showbar          = 1;  /* show status bar */
const int topbar           = 0;  /* status bar on top */
const int resizehints      = 0;  /* enable client resize hints */

const char *fonts[1]     = { "monospace:size=10" };
const char *tags[9]      = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };
const char *colors[2][3] = {
	[ClrNorm] = { "#bbbbbb", "#222222", "#444444" },
	[ClrSel]  = { "#eeeeee", "#005577", "#005577" }
};

const Key keys[60] = {
	{ MODKEY, XK_equal, spawn, { .v = (void *[])
			{ "amixer", "set", "Master", "5%+", NULL } } },
	{ MODKEY, XK_minus, spawn, { .v = (void *[])
			{ "amixer", "set", "Master", "5%-", NULL } } },
	{ MODKEY, XK_m,     spawn, { .v = (void *[])
			{ "amixer", "set", "Master", "toggle", NULL } } },

	{ MODKEY, XK_Return, spawn, { .v = (void *[]){ "st", NULL } } },
	{ MODKEY, XK_w,      spawn, { .v = (void *[]){ "chromium", NULL } } },
	{ MODKEY, XK_e,      startexec, { 0 } },
	{ MODKEY | ShiftMask, XK_q, killclient, { 0 } },

	{ MODKEY, XK_semicolon, zoom, { 0 } },
	{ MODKEY, XK_j, focusstack, { .n = 1 } },
	{ MODKEY, XK_k, focusstack, { .n = -1 } },
	{ MODKEY, XK_l, setmfact,   { .n = 5 } },
	{ MODKEY, XK_h, setmfact,   { .n = -5 } },
	{ MODKEY, XK_i, incnmaster, { .n = 1 } },
	{ MODKEY, XK_o, incnmaster, { .n = -1 } },

	{ MODKEY, XK_comma,  focusmon, { .n = -1 } },
	{ MODKEY, XK_period, focusmon, { .n = +1 } },
	{ MODKEY | ShiftMask, XK_comma,  tagmon, { .n = -1 } },
	{ MODKEY | ShiftMask, XK_period, tagmon, { .n = +1 } },

	{ MODKEY, XK_b, togglebar, { 0 } },
	{ MODKEY, XK_g, togglegaps, { 0 } },

#define TAGKEY(keysym, shift) \
	{ MODKEY, keysym, view, { .n = 1 << shift } }, \
	{ MODKEY | ControlMask, keysym, toggleview, { .n = 1 << shift } }, \
	{ MODKEY | ShiftMask, keysym, tag, { .n = 1 << shift } }, \
	{ MODKEY | ControlMask | ShiftMask, keysym, toggletag, \
		{ .n = 1 << shift } }
	TAGKEY(XK_1, 0),
	TAGKEY(XK_2, 1),
	TAGKEY(XK_3, 2),
	TAGKEY(XK_4, 3),
	TAGKEY(XK_5, 4),
	TAGKEY(XK_6, 5),
	TAGKEY(XK_7, 6),
	TAGKEY(XK_8, 7),
	TAGKEY(XK_9, 8),
#undef TAGKEY

	{ MODKEY, XK_0, view, { .n = ~0 } },
	{ MODKEY | ShiftMask, XK_0, tag, { .n = ~0 } },

	{ MODKEY | ShiftMask, XK_e, quit, { 0 } }
};

const Button buttons[4] = {
	{ ClkTagBar, 0,      Button1, view,       { 0 } },
	{ ClkTagBar, 0,      Button3, toggleview, { 0 } },
	{ ClkTagBar, MODKEY, Button1, tag,        { 0 } },
	{ ClkTagBar, MODKEY, Button3, toggletag,  { 0 } },
};
