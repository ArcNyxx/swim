/* swim - simple window manager
 * Copyright (C) 2022 ArcNyxx
 * see LICENCE file for licensing information */

#ifndef TILE_H
#define TILE_H

#include "struct.h"

Client *nexttiled(Client *cli);
void tile(Monitor *mon);

#endif /* TILE_H */
