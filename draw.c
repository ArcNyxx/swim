/* swim - simple window manager
 * Copyright (C) 2022 ArcNyxx
 * see LICENCE file for licensing information */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include FT_ADVANCES_H
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>

#include "draw.h"
#include "utf8.h"
#include "util.h"

static XRenderColor draw_clr_to_xrclr(const TextColour *clr);
static unsigned long draw_clr_to_xclr(Draw *draw, const TextColour *clr);

static void text_init(Draw *draw);
static void text_free(Draw *draw);

static void text_font_init(Draw *restrict draw, char *restrict name);
static void text_font_free(Draw *draw);

static TextChar *text_load_char(Draw *draw, wchar_t ch);

static void text_draw(Draw *restrict draw, int clr, uint32_t x, uint32_t y,
		const wchar_t *restrict str, size_t len);

static Colormap clrmap;

static XRenderColor
draw_clr_to_xrclr(const TextColour *clr)
{
	return (XRenderColor){
		.red = (clr->r << 8) + clr->r,
		.green = (clr->g << 8) + clr->g,
		.blue = (clr->b << 8) + clr->b,
		.alpha = (clr->a << 8) + clr->a
	};
}

/* does not allow alpha values */
static unsigned long
draw_clr_to_xclr(Draw *draw, const TextColour *clr)
{
	XColor xclr = {
		.red = (clr->r << 8) + clr->r,
		.green = (clr->g << 8) + clr->g,
		.blue = (clr->b << 8) + clr->b,
		.flags = DoRed | DoGreen | DoBlue
	};
	XAllocColor(draw->dpy, clrmap, &xclr);
	return xclr.pixel;
}

static TextChar *
text_load_char(Draw *draw, wchar_t ch)
{
	/* check if glyph already made */
	TextRow row = draw->fnt.table[ch % SAVE_CHAR];
	if (row.data != NULL)
		for (size_t i = 0; i < row.len; ++i)
			if (row.data[i].ch == ch)
				return &row.data[i];

	/* loads the glyph image, returns if match not found in charmap */
	FT_GlyphSlot slot = 0;
	for (size_t i = 0; i < draw->fnt.faces_num; ++i) {
		if (FT_Load_Char(draw->fnt.faces[i], ch, FT_LOAD_RENDER) == 0) {
			slot = draw->fnt.faces[i]->glyph;
			break;
		}
	}
	if (slot == 0)
		return NULL; /* handled by caller, dont want to die on font fail */

	XGlyphInfo xglyph = {
		.width = slot->bitmap.width,
		.height = slot->bitmap.rows,
		.x = slot->bitmap_left,
		.y = slot->bitmap_top,
		.xOff = slot->advance.x >> 6,
		.yOff = slot->advance.y >>6
	};

	/* maps then renders the glyph */
	char *img = scalloc(1, 4 * xglyph.width * xglyph.height);
	for (size_t y = 0; y < xglyph.height; ++y)
		for (size_t x = 0; x < xglyph.width; ++x)
			for (size_t i = 0; i < 4; ++i)
				img[4 * (y * xglyph.width + x) + i] =
						slot->bitmap.buffer[y * xglyph.width + x];

	XRenderAddGlyphs(draw->dpy, draw->fnt.gs, (size_t *)&ch, &xglyph, 1,
			img, 4 * xglyph.width * xglyph.height);
	free(img); /* glyph saved into set by x */

	/* saves the glyph into the table */
	TextChar data = {
		.ch = ch,
		.adv = slot->advance.x >> 6,
		.height = slot->metrics.height >> 6,
		.asc = slot->metrics.horiBearingY >> 6
	};

	/*
	 * allocates if data is null 
	 * reallocates a longer row if not enough space
	 */
	if (row.data == NULL)
		row.data = scalloc(row.alloc = SAVE_ALLOC, sizeof(TextChar));
	else if (row.alloc == row.len)
		row.data = srealloc(row.data, (row.alloc *= 2) * sizeof(TextChar));
	row.data[row.len++] = data;

	return &row.data[row.len];
}

static void
text_init(Draw *draw)
{
	if (FT_Init_FreeType(&draw->ctx.lib) != 0)
		die("dwm: unable to init freetype\n");

	Visual *visual = XDefaultVisual(draw->dpy, XDefaultScreen(draw->dpy));
	draw->ctx.fmt_win = XRenderFindVisualFormat(draw->dpy, visual);
	draw->ctx.fmt_rgba = XRenderFindStandardFormat(draw->dpy,
			PictStandardARGB32);
	XFree(visual);

	XRenderPictureAttributes attrs;
	draw->ctx.pic = XRenderCreatePicture(draw->dpy,
			draw->draw, draw->ctx.fmt_win, 0, &attrs);

	Pixmap pix = XCreatePixmap(draw->dpy,
			DefaultRootWindow(draw->dpy), 1, 1, 32);
	attrs.repeat = 1;
	draw->ctx.fill = XRenderCreatePicture(draw->dpy, pix, draw->ctx.fmt_rgba,
			CPRepeat, &attrs);
	XFreePixmap(draw->dpy, pix);
}

static void
text_free(Draw *draw)
{
	XRenderFreePicture(draw->dpy, draw->ctx.pic);
	XRenderFreePicture(draw->dpy, draw->ctx.fill);
	XFree(draw->ctx.fmt_win);
	XFree(draw->ctx.fmt_rgba);

	FT_Done_FreeType(draw->ctx.lib);
}

static void
text_font_init(Draw *restrict draw, char *restrict name)
{
	/* gets the number of faces by checking number of semicolons */
	size_t num = 0;
	for (size_t i = 0; name[i]; ++i)
		num += (name[i] == ';');
	draw->fnt.faces = scalloc(num, sizeof(FT_Face));

	for (draw->fnt.faces_num = 0; num < draw->fnt.faces_num; ) {
		size_t len = strchr(name, ';') - name, colon = strchr(name, ':') - name;
		name[len] = name[colon] = '\0';

		if (FT_New_Face(draw->ctx.lib, name, 0,
				&draw->fnt.faces[draw->fnt.faces_num]) != 0)
			continue; /* continue if unable to make the face */
		if (FT_Set_Char_Size(draw->fnt.faces[draw->fnt.faces_num],
				strtoul(&name[colon] + 1, NULL, 10) << 6, 0, 0, 0) != 0) {
			FT_Done_Face(draw->fnt.faces[draw->fnt.faces_num]);
			continue;
		}

		name += len + 1;
		++draw->fnt.faces_num;
	}
	if (draw->fnt.faces_num == 0)
		die("dwm: unable to create font faces\n");
	if (draw->fnt.faces_num != num) /* reallocate if unequal prealloc */
		draw->fnt.faces = srealloc(draw->fnt.faces,
				draw->fnt.faces_num * sizeof(FT_Face));

	/* register the glyph set with x */
	draw->fnt.gs = XRenderCreateGlyphSet(draw->dpy, draw->ctx.fmt_rgba);
	memset(draw->fnt.table, '\0', SAVE_CHAR * sizeof(TextRow));

	/* get the height of the largest glyph */
	draw->fnt.ascent = 0;
	int16_t descent = 0;
	for (size_t i = 0; i < draw->fnt.faces_num; ++i) {
		draw->fnt.ascent = MAX(draw->fnt.ascent,
				draw->fnt.faces[i]->ascender >> 6);
		descent = MIN(descent, draw->fnt.faces[i]->descender >> 6);
	}
	draw->fnt.height = draw->fnt.height - descent;
}

static void
text_font_free(Draw *draw)
{
	XRenderFreeGlyphSet(draw->dpy, draw->fnt.gs);
	for (size_t i = 0; i < draw->fnt.faces_num; ++i)
		FT_Done_Face(draw->fnt.faces[i]);
	free(draw->fnt.faces);
}

void
text_box(Draw *restrict draw, TextBBox *restrict bbox,
		const wchar_t *restrict str, size_t len)
{
	if (bbox == NULL)
		return;
	memset(bbox, '\0', sizeof(TextBBox));
	for (size_t i = 0; i < len; ++i) {
		TextChar *data;
		if ((data = text_load_char(draw, str[i])) == NULL)
			continue; /* continue if unable to render a char */
		bbox->w += data->adv;
		bbox->h = MAX(data->height, bbox->h + MAX(0, bbox->y - data->asc));
		bbox->y = MIN(data->asc, bbox->y);
	}
}

static void
text_draw(Draw *restrict draw, int clr, uint32_t x, uint32_t y,
		const wchar_t *restrict str, size_t len)
{
	XRenderColor xrclr = draw_clr_to_xrclr(&draw->clr[clr]);
	XRenderFillRectangle(draw->dpy, PictOpSrc,
			draw->ctx.fill, &xrclr, 0, 0, 1, 1);

	wchar_t *sstr = scalloc(len, sizeof(wchar_t));
	for (size_t i = 0, j = 0; i < len; ++i)
		if (text_load_char(draw, str[i]) != NULL)
			sstr[j++] = str[i]; /* copy only renderable chars */
	XRenderCompositeString32(draw->dpy, PictOpOver, draw->ctx.fill,
			draw->ctx.pic, draw->ctx.fmt_rgba, draw->fnt.gs, 0, 0, x, y,
			(const uint32_t *)sstr, len);
}

void
draw_init(Draw *restrict draw, char *restrict fnt,
		const TextColour clr[ClrLast], const unsigned int cur[CurLast])
{
	draw->screen = DefaultScreen(draw->dpy);
	draw->win = RootWindow(draw->dpy, draw->screen);
	draw->w = DisplayWidth(draw->dpy, draw->screen);
	draw->h = DisplayHeight(draw->dpy, draw->screen);

	/* creates static global colourmap for xlib colour translation */
	clrmap = DefaultColormap(draw->dpy, draw->screen);

	draw->draw = XCreatePixmap(draw->dpy, draw->win, draw->w, draw->h,
			DefaultDepth(draw->dpy, draw->screen));
	if ((draw->gc = XCreateGC(draw->dpy, draw->win, 0, NULL)) != 0)
		die("dwm: unable to create graphics context\n");
	if (XSetLineAttributes(draw->dpy, draw->gc, 1, 
			LineSolid, CapButt, JoinMiter) != 0)
		die("dwm: unable to set line attributes\n");

	text_init(draw);
	text_font_init(draw, fnt);

	draw->clr = clr;
	for (int i = 0; i < CurLast; ++i)
		draw->cur[i] = XCreateFontCursor(draw->dpy, cur[i]);
}

void
draw_free(Draw *draw)
{
	for (int i = 0; i < CurLast; ++i)
		XFreeCursor(draw->dpy, draw->cur[i]);

	text_font_free(draw);
	text_free(draw);

	XFreeGC(draw->dpy, draw->gc);
	XFreePixmap(draw->dpy, draw->draw);
}

void
draw_text_bottom(Draw *restrict draw, int clr_fg, int clr_bg, TextBBox *bbox,
		uint32_t x, uint32_t y, const wchar_t *restrict str, size_t len)
{
	if (bbox == NULL)
		text_box(draw, bbox, str, len);

	XSetForeground(draw->dpy, draw->gc,
			draw_clr_to_xclr(draw, &draw->clr[clr_bg]));
	XFillRectangle(draw->dpy, draw->draw, draw->gc, bbox->x, bbox->y,
			bbox->w + 2 * bbox->h, draw->fnt.height - bbox->h);
	text_draw(draw, clr_fg, bbox->x + bbox->h,
			bbox->y + draw->fnt.ascent - bbox->h, str, len);
}
