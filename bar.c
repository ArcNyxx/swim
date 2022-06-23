/* swim - simple window manager
 * Copyright (C) 2022 ArcNyxx
 * see LICENCE file for licensing information */

#include "bar.h"
#include "config.h"
#include "drw.h"
#include "struct.h"
#include "util.h"

extern Monitor *selmon, *mons;
extern Drw *drw;
extern Clr **scheme;

void
drawbar(Monitor *mon)
{
	extern int exec;
	extern char stext[256], execa[256];

	int x, w, tw = 0;
	int boxs = PADDING / 9, boxw = PADDING / 6 + 2;
	int occ = 0, urg = 0;

	if (!mon->showbar)
		return;

	if (mon == selmon) {
		drw_setscheme(drw, scheme[ClrNorm]);
		tw = drw_fontset_getwidth(drw, stext);
		drw_text(drw, mon->ww - tw, 0, tw, PADDING + 4, 0, stext, 0);
	}

	for (Client *cli = mon->clients; cli != NULL; cli = cli->next)
		occ |= cli->tags, urg |= cli->isurgent ? cli->tags : 0;
	x = 0;
	for (int i = 0; i < LENGTH(tags); ++i) {
		w = drw_fontset_getwidth(drw, tags[i]) + PADDING;
		drw_setscheme(drw, scheme[mon->tags & 1 << i ? ClrSel : ClrNorm]);
		drw_text(drw, x, 0, w, PADDING + 4, PADDING / 2, tags[i], (urg & 1 << i) != 0);
		if ((occ & 1 << i) != 0)
			drw_rect(drw, x + boxs, boxs, boxw, boxw,
					mon == selmon && selmon->sel != NULL &&
					(selmon->sel->tags & 1 << i) != 0,
					(urg & 1 << 1) != 0);
		x += w;
	}
	if ((w = mon->ww - tw - x) > PADDING) {
		if (exec != -1) {
			drw_setscheme(drw, scheme[ClrSel]);
			drw_text(drw, x, 0, w, PADDING + 4, PADDING / 2, execa, 0);
		} else if (mon->sel != NULL) {
			drw_setscheme(drw, scheme[mon == selmon ? ClrSel : ClrNorm]);
			drw_text(drw, x, 0, w, PADDING + 4, PADDING / 2, mon->sel->name, 0);
			if (mon->sel->isfloating)
				drw_rect(drw, x + boxs, boxs, boxw, boxw, mon->sel->isfixed, 0);
		} else {
			drw_setscheme(drw, scheme[ClrNorm]);
			drw_rect(drw, x, 0, w, PADDING , 1, 1);
		}
	}
	drw_map(drw, mon->barwin, 0, 0, mon->ww, PADDING);
}

void
drawbars(void)
{
	for (Monitor *mon = mons; mon != NULL; mon = mon->next)
		drawbar(mon);
}
