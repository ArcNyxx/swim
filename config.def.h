/* swim - simple window manager
 * Copyright (C) 2022 ArcNyxx
 * see LICENCE file for licensing information */

#define GAPIH 12
#define GAPIV 12
#define GAPOH 8
#define GAPOV 8

const unsigned int borderw = 1;  /* border width of windows */
const unsigned int mfact   = 50; /* percentage factor of master area */
const unsigned int nmaster = 1;  /* number of clients in master area */
const unsigned int snap    = 32; /* mouse functions snap boundary */
const int showbar          = 1;  /* show status bar */
const int topbar           = 0;  /* status bar on top */
const int resizehints      = 0;  /* enable client resize hints */
const int lockfullscreen   = 1;  /* force focus on fullscreen window */

const char *fonts[]     = { "monospace:size=10" };
const char *tags[]      = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };
const char *colors[][3] = {
	[SchemeNorm] = { "#bbbbbb", "#222222", "#444444" },
	[SchemeSel]  = { "#eeeeee", "#005577", "#005577" }
};

#define MODKEY Mod4Mask

const Key keys[] = {
	{ MODKEY, XK_Return, spawn, { .v = (void *[]){ "st", NULL } } },
	{ MODKEY, XK_w,      spawn, { .v = (void *[]){ "chromium", NULL } } },
	{ MODKEY | ShiftMask, XK_q, killclient, { 0 } },

	{ MODKEY, XK_semicolon, zoom, { 0 } },
	{ MODKEY, XK_j, focusstack, { .n = 1 } },
	{ MODKEY, XK_k, focusstack, { .n = -1 } },
	{ MODKEY, XK_h, setmfact,   { .n = 5 } },
	{ MODKEY, Xk_l, setmfact,   { .n = -5 } },
	{ MODKEY, XK_i, incnmaster, { .n = 1 } },
	{ MODKEY, XK_o, incnmaster, { .n = -1 } },

	{ MODKEY, XK_comma,  focusmon, { .n = -1 } },
	{ MODKEY, XK_period, focusmon, { .n = +1 } },
	{ MODKEY | ShiftMask, XK_comma,  tagmon, { .n = -1 } },
	{ MODKEY | ShiftMask, XK_period, tagmon, { .n = +1 } },

	{ MODKEY, XK_b, togglebar, { 0 } },
	{ MODKEY, XK_g, togglegaps, { 0 } },
	{ MODKEY, XK_space, togglefloating, { 0 } },

	{ MODKEY, XK_1, view, { .n = 1 << 0 } },
	{ MODKEY | ControlMask, XK_1, toggleview, { .n = 1 << 0 } },
	{ MODKEY | ShiftMask, XK_1, tag, { .n = 1 << 0 } },
	{ MODKEY | ControlMask | ShiftMask, XK_1, toggletag, { .n = 1 << 0 } },

	{ MODKEY, XK_2, view, { .n = 1 << 1 } },
	{ MODKEY | ControlMask, XK_2, toggleview, { .n = 1 << 1 } },
	{ MODKEY | ShiftMask, XK_2, tag, { .n = 1 << 1 } },
	{ MODKEY | ControlMask | ShiftMask, XK_2, toggletag, { .n = 1 << 1 } },

	{ MODKEY, XK_3, view, { .n = 1 << 2 } },
	{ MODKEY | ControlMask, XK_3, toggleview, { .n = 1 << 2 } },
	{ MODKEY | ShiftMask, XK_3, tag, { .n = 1 << 2 } },
	{ MODKEY | ControlMask | ShiftMask, XK_3, toggletag, { .n = 1 << 2 } },

	{ MODKEY, XK_4, view, { .n = 1 << 3 } },
	{ MODKEY | ControlMask, XK_4, toggleview, { .n = 1 << 3 } },
	{ MODKEY | ShiftMask, XK_4, tag, { .n = 1 << 3 } },
	{ MODKEY | ControlMask | ShiftMask, XK_4, toggletag, { .n = 1 << 3 } },

	{ MODKEY, XK_5, view, { .n = 1 << 4 } },
	{ MODKEY | ControlMask, XK_5, toggleview, { .n = 1 << 4 } },
	{ MODKEY | ShiftMask, XK_5, tag, { .n = 1 << 4 } },
	{ MODKEY | ControlMask | ShiftMask, XK_5, toggletag, { .n = 1 << 4 } },

	{ MODKEY, XK_6, view, { .n = 1 << 5 } },
	{ MODKEY | ControlMask, XK_6, toggleview, { .n = 1 << 5 } },
	{ MODKEY | ShiftMask, XK_6, tag, { .n = 1 << 5 } },
	{ MODKEY | ControlMask | ShiftMask, XK_6, toggletag, { .n = 1 << 5 } },

	{ MODKEY, XK_7, view, { .n = 1 << 6 } },
	{ MODKEY | ControlMask, XK_7, toggleview, { .n = 1 << 6 } },
	{ MODKEY | ShiftMask, XK_7, tag, { .n = 1 << 6 } },
	{ MODKEY | ControlMask | ShiftMask, XK_7, toggletag, { .n = 1 << 6 } },

	{ MODKEY, XK_8, view, { .n = 1 << 7 } },
	{ MODKEY | ControlMask, XK_8, toggleview, { .n = 1 << 7 } },
	{ MODKEY | ShiftMask, XK_8, tag, { .n = 1 << 7 } },
	{ MODKEY | ControlMask | ShiftMask, XK_8, toggletag, { .n = 1 << 7 } },

	{ MODKEY, XK_9, view, { .n = 1 << 8 } },
	{ MODKEY | ControlMask, XK_9, toggleview, { .n = 1 << 8 } },
	{ MODKEY | ShiftMask, XK_9, tag, { .n = 1 << 8 } },
	{ MODKEY | ControlMask | ShiftMask, XK_9, toggletag, { .n = 1 << 8 } },

	{ MODKEY, XK_0, view, { .n = ~0 } },
	{ MODKEY | ShiftMask, XK_0, tag, { .n = ~0 } },

	{ MODKEY | ShiftMask, XK_e, quit, { 0 } }
};

const Button buttons[] = {
	{ ClkTagBar, 0,      Button1, view,       { 0 } },
	{ ClkTagBar, 0,      Button3, toggleview, { 0 } },
	{ ClkTagBar, MODKEY, Button1, tag,        { 0 } },
	{ ClkTagBar, MODKEY, Button3, toggletag,  { 0 } },
};

