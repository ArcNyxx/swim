/* swim - simple window manager
 * Copyright (C) 2022 ArcNyxx
 * see LICENCE file for licensing information */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

#include "struct.h"

#define GAPIH 12
#define GAPIV 12
#define GAPOH 8
#define GAPOV 8

#define PADW 16
#define PADH 20

extern const int  borderw;
extern const int  mfact;
extern const int  nmaster;
extern const int  snap;
extern const bool rhints;
extern const bool showbar;
extern const bool topbar;

extern const char *font;
extern const char *tags[9];
extern const char *colors[2][3];

extern const Key keys[59];
extern const Button buttons[4];

#endif /* CONFIG_H */
