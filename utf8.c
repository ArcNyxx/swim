/* swim - simple window manager
 * Copyright (C) 2022 ArcNyxx
 * see LICENCE file for licensing information */

#include <stddef.h>

#include "utf8.h"
#include "util.h"

#define UTF_SIZE 4
#define UTF_INVALID 0xFFFD

static char utf8decodebyte(const char ch, int *ext);

static const char utfbyte[UTF_SIZE + 1] = { 0x80, 0, 0xC0, 0xE0, 0xF0 };
static const char utfmask[UTF_SIZE + 1] = { 0xC0, 0x80, 0xE0, 0xF0, 0xF8 };

static const int utfmin[UTF_SIZE + 1] = { 0, 0, 0x80, 0x800, 0x10000 };
static const int utfmax[UTF_SIZE + 1] =
		{ 0x10FFFF, 0x7F, 0x7FF, 0xFFFF, 0x10FFFF };

static char
utf8decodebyte(const char ch, int *ext)
{
	for (*ext = 0; *ext < (UTF_SIZE + 1); ++(*ext))
		if ((ch & utfmask[*ext]) == utfbyte[*ext])
			return ch & ~utfmask[*ext];
	*ext = -1;
	return 0;
}

int
utf8decode(const char *restrict str, wchar_t *restrict val)
{
	int len;
	*val = UTF_INVALID;
	wchar_t decoded = utf8decodebyte(str[0], &len);
	for (int i = 1, type; i < len; ++i) {
		decoded = (decoded << 6) + utf8decodebyte(str[i], &type);
		if (type != 0)
			return i;
	}
	if (len != -1 && decoded <= utfmax[len] && decoded >= utfmin[len] &&
			(decoded < 0xD800 || decoded > 0xDFFF))
		*val = decoded;
	return len;
}
