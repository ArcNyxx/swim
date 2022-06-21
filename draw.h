/* swim - simple window manager
 * Copyright (C) 2022 ArcNyxx
 * see LICENCE file for licensing information */

#include <stdint.h>

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include FT_ADVANCES_H
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>

#define SAVE_CHAR 128
#define SAVE_ALLOC 8

enum cursor { CurNormal, CurResize, CurMove, CurLast };
enum colour { ClrNormal, ClrSelect, ClrText, ClrBorder, ClrLast };

typedef struct text_ctx {
	FT_Library lib;
	XRenderPictFormat *fmt_win, *fmt_rgba;
	Picture pic, fill;
} TextCtx;

typedef struct text_char {
	wchar_t ch;
	uint16_t adv, height;
	int16_t asc;
} TextChar;

typedef struct text_row {
	TextChar *data;
	size_t len, alloc;
} TextRow;

typedef struct text_font {
	FT_Face *faces;
	size_t faces_num;

	GlyphSet gs;
	uint16_t height, ascent;

	TextRow table[SAVE_CHAR];
} TextFont;

typedef struct text_bbox {
	int32_t x, y;
	uint32_t w, h;
} TextBBox;

typedef struct text_colour {
	uint8_t r, g, b, a;
} TextColour;

typedef struct draw {
	Display *dpy;
	Window win;
	GC gc;

	Drawable draw;
	int screen;

	unsigned int w, h;

	TextCtx ctx;
	TextFont fnt;

	TextColour clr[ClrLast];
	Cursor cur[CurLast];
} Draw;

void text_box(Draw *restrict draw, TextBBox *restrict bbox,
		const wchar_t *restrict str, size_t len);

void draw_init(Draw *restrict draw, char *restrict fnt,
		const TextColour clr[ClrLast], const unsigned int cur[CurLast]);
void draw_free(Draw *draw);

void draw_text_bottom(Draw *restrict draw, int clr_fg, int clr_bg,
		TextBBox *bbox, uint32_t x, uint32_t y,
		const wchar_t *restrict str, size_t len);
