/* swim - simple window manager
 * Copyright (C) 2022 ArcNyxx
 * see LICENCE file for licensing information */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "utf8.h"
#include "util.h"

static char utf8decodebyte(const char ch, size_t *ext);

#define UTF_SIZE 4
#define UTF_INVALID 0xFFFD

/* ext, single, double ext, triple ext, quad ext */
static const char utfbyte[UTF_SIZE + 1] = 
		{ 0x80, 0, 0xc0, 0xe0, 0xf0 };
static const char utfmask[UTF_SIZE + 1] =
		{ 0xc0, 0x80, 0xe0, 0xf0, 0xf8 };

/* min and max sizes for each type of ext */
static const size_t utfmin[UTF_SIZE + 1] =
		{ 0, 0, 0x80, 0x800, 0x10000 };
static const size_t utfmax[UTF_SIZE + 1] =
		{ 0x10ffff, 0x7f, 0x7ff, 0xffff, 0x10ffff };

/* get ext type of the given byte, return the remaining value */
static char
utf8decodebyte(const char ch, size_t *ext)
{
	for (*ext = 0; *ext < (UTF_SIZE + 1); ++(*ext))
		if ((ch & utfmask[*ext]) == utfbyte[*ext])
			return ch & ~utfmask[*ext];
	*ext = -1; /* indicate error to caller */
	return 0;
}

/* returns length of char in bytes, zero if invalid */
size_t
utf8decode(const char *restrict str, wchar_t *restrict val)
{
	size_t i, len, type;
	uint32_t decoded;
	*val = UTF_INVALID;

	/*
	 * because the length is set to negative one on invalid ext
	 * the following loop will go until the next ext is reached
	 * essentially skipping a char
	 */
	decoded = utf8decodebyte(str[0], &len);
	for (i = 1; i < len; ++i) {
		/* make room for next ext */
		decoded = (decoded << 6) | utf8decodebyte(str[i], &type);
		if (type)
			return i; /* if next char too early, discard current and move on */
	}

	/* ensures char is within valid utf values */
	if (!(decoded > utfmax[len] || decoded < utfmin[len] ||
			(decoded >= 0xD800 && decoded <= 0xDFFF)))
		*val = decoded;
	return len;
}

wchar_t *
utf8convert(const char *str, size_t len)
{
	size_t str_i, wstr_i;
	wchar_t *wstr = scalloc(len, sizeof(wchar_t));

	for (str_i = wstr_i = 0; str_i < len; ) {
		wchar_t val;
		str_i += utf8decode(str, &val);
		if (val != UTF_INVALID)
			wstr[wstr_i++] = val; 
	}

	if (str_i != wstr_i)
		return srealloc(wstr, wstr_i * sizeof(wchar_t));
	return wstr;
}
