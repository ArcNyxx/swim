/* swim - simple window manager
 * Copyright (C) 2022 ArcNyxx
 * see LICENCE file for licensing information */

#ifndef ACT_H
#define ACT_H

#include "struct.h"

void focusmon    (const Arg arg);
void focusstack  (const Arg arg);
void incnmaster  (const Arg arg);
void killclient  (const Arg arg);
void movemouse   (const Arg arg);
void quit        (const Arg arg);
void resizemouse (const Arg arg);
void togglegaps  (const Arg arg);
void setmfact    (const Arg arg);
void spawn       (const Arg arg);
void startexec   (const Arg arg);
void tag         (const Arg arg);
void tagmon      (const Arg arg);
void togglebar   (const Arg arg);
void toggletag   (const Arg arg);
void toggleview  (const Arg arg);
void view        (const Arg arg);
void zoom        (const Arg arg);

#endif /* ACT_H */
