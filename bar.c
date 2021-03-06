/* swim - simple window manager
 * Copyright (C) 2022 ArcNyxx
 * see LICENCE file for licensing information */

#include "bar.h"
#include "config.h"
#include "drw.h"
#include "struct.h"
#include "util.h"

#define BOXS (PADW / 9)
#define BOXW (PADW / 6 + 2)

extern Monitor *sel, *mons;
extern Drw *drw;
extern Clr **scheme;

void
drawbar(const Monitor *mon)
{
	extern int exec;
	extern char stext[256], execa[256];

	if (!mon->showbar)
		return;

	int x, w, tw = 0;
	drw_setscheme(drw, scheme[ClrNorm]);
	tw = drw_fontset_getwidth(drw, stext);
	drw_text(drw, mon->ww - tw, 0, tw, PADH, 0, stext, 0);

	int occ = 0, urg = 0;
	for (Client *cli = mon->clients; cli != NULL; cli = cli->next)
		occ |= cli->tags, urg |= cli->isurgent ? cli->tags : 0;

	x = 0;
	for (int i = 0; i < LENGTH(tags); ++i) {
		w = drw_fontset_getwidth(drw, tags[i]) + PADW;
		drw_setscheme(drw, scheme[mon->tags & 1 << i ?
				ClrSel : ClrNorm]);
		drw_text(drw, x, 0, w, PADH, PADW / 2,
				tags[i], (urg & 1 << i) != 0);
		if ((occ & 1 << i) != 0)
			drw_rect(drw, x + BOXS, BOXS, BOXW, BOXW,
					mon == sel && sel->sel != NULL &&
					(sel->sel->tags & 1 << i) != 0,
					(urg & 1 << 1) != 0);
		x += w;
	}

	if ((w = mon->ww - tw - x) > PADW) {
		if (exec != -1) {
			drw_setscheme(drw, scheme[ClrSel]);
			drw_text(drw, x, 0, w, PADH, PADW / 2, execa, 0);
		} else if (mon->sel != NULL) {
			drw_setscheme(drw, scheme[mon == sel ?
					ClrSel : ClrNorm]);
			drw_text(drw, x, 0, w, PADH, PADW / 2,
					mon->sel->name, 0);
			if (mon->sel->isfloating)
				drw_rect(drw, x + BOXS, BOXS, BOXW, BOXW,
						mon->sel->isfixed, 0);
		} else {
			drw_setscheme(drw, scheme[ClrNorm]);
			drw_rect(drw, x, 0, w, PADH, 1, 1);
		}
	}
	drw_map(drw, mon->barwin, 0, 0, mon->ww, PADH);
}

void
drawbars(void)
{
	for (Monitor *mon = mons; mon != NULL; mon = mon->next)
		drawbar(mon);
}
