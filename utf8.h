/* swim - simple window manager
 * Copyright (C) 2022 ArcNyxx
 * see LICENCE file for licensing information */

#ifndef UTF8_H
#define UTF8_H

#include <stddef.h>

int utf8decode(const char *restrict str, wchar_t *restrict val);

#endif /* UTF8_H */
