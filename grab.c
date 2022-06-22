#include <stdbool.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include "struct.h"
#include "util.h"
#include "config.h"

static void getmask(Display *dpy);

unsigned int numlock;

static void
getmask(Display *dpy)
{
	numlock = 0;
	XModifierKeymap *modmap = XGetModifierMapping(dpy);

	for (int i = 0; i < 8; ++i)
		for (int j = 0; j < modmap->max_keypermod; ++j)
			if (modmap->modifiermap[i * modmap->max_keypermod + j]
					== XKeysymToKeycode(dpy, XK_Num_Lock))
				numlock = (1 << i);
	XFreeModifiermap(modmap);
}

void
grabbuttons(Display *dpy, Client *client, int focused)
{
	getmask(dpy);
	XUngrabButton(dpy, AnyButton, AnyModifier, client->win);
	unsigned int mods[] = { 0, LockMask, numlock, LockMask | numlock };

	if (!focused)
		XGrabButton(dpy, AnyButton, AnyModifier, client->win, 0,
				BUTTON, GrabModeSync, GrabModeSync, 0, 0);
	for (int i = 0; i < LENGTH(buttons); ++i) {
		if (buttons[i].click != ClkClientWin)
			continue;
		for (int j = 0; j < LENGTH(mods); ++j)
			XGrabButton(dpy, buttons[i].button, buttons[i].mask |
					mods[j], client->win, false, BUTTON,
					GrabModeAsync, GrabModeSync, 0, 0);
	}
}

void
grabkeys(Display *dpy)
{
	Window root = RootWindow(dpy, DefaultScreen(dpy));

	getmask(dpy);
	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	unsigned int mods[] = { 0, LockMask, numlock, LockMask | numlock };

	KeyCode code;
	for (int i = 0; i < LENGTH(keys); ++i) {
		if ((code = XKeysymToKeycode(dpy, keys[i].keysym)) == 0)
			continue;
		for (int j = 0; j < LENGTH(mods); ++j)
			XGrabKey(dpy, code, keys[i].mod | mods[j], root,
					true, GrabModeAsync, GrabModeAsync);
	}
}
