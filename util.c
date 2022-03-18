/* swim - simple window manager
 * Copyright (C) 2022 ArcNyxx
 * see LICENCE file for licensing information */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

void
die(const char *fmt, ...)
{
	if (fmt[strlen(fmt) - 1] == '\n') {
		va_list list;
		va_start(list, fmt);
		vfprintf(stderr, fmt, list);
		va_end(list);
	} else {
		perror(fmt);
	}
	exit(1);
}

void *
scalloc(size_t nmemb, size_t size)
{
	void *ptr;
	if ((ptr = calloc(nmemb, size)) == NULL)
		die("swim: unable to allocate memory");
	return ptr;
}

void
*srealloc(void *ptr, size_t size)
{
	if ((ptr = realloc(ptr, size)) == NULL)
		die("swim: unable to allocate memory");
	return ptr;
}
