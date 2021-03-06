/* swim - simple window manager
 * Copyright (C) 2022 ArcNyxx
 * see LICENCE file for licensing information */

#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

#include "grab.h"

#define MAX(num1, num2) ((num1) > (num2) ? (num1) : (num2))
#define MIN(num1, num2) ((num1) < (num2) ? (num1) : (num2))
#define LENGTH(array) (int)(sizeof(array) / sizeof(array[0]))

#define VISIBLE( client) (client->tags & client->mon->tags)
#define WIDTH(   client) (client->w + 2 * borderw)
#define HEIGHT(  client) (client->h + 2 * borderw)
#define TAG              ((1 << LENGTH(tags)) - 1)
#define CLEAN(   mask)   (mask & ~(numlock | LockMask) & (ShiftMask | \
	ControlMask | Mod1Mask | Mod2Mask | Mod3Mask | Mod4Mask | Mod5Mask))

void die(const char *fmt, ...);
void *scalloc(size_t nmemb, size_t size);
void *srealloc(void *ptr, size_t size);

#endif /* UTIL_H */
