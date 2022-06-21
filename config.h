/* swim - simple window manager
 * Copyright (C) 2022 ArcNyxx
 * see LICENCE file for licensing information */

#ifndef CONFIG_H
#define CONFIG_H

#include "struct.h"

#define GAPIH 12
#define GAPIV 12
#define GAPOH 8
#define GAPOV 8

extern const unsigned int borderw;
extern const unsigned int mfact;
extern const unsigned int nmaster;
extern const unsigned int snap;
extern const int showbar;
extern const int topbar;
extern const int resizehints;

extern const char *fonts[1];
extern const char *tags[9];
extern const char *colors[2][3];

extern const Key keys[60];
extern const Button buttons[4];

#endif /* CONFIG_H */
