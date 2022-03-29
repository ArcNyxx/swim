# swim - simple window manager
# Copyright (C) 2022 ArcNyxx
# see LICENCE file for licensing information

VERSION = 6.3

PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

WPROFILE = -Wall -Wextra -Wstrict-prototypes -Wmissing-declarations \
-Wswitch-default -Wunreachable-code -Wcast-align -Wpointer-arith \
-Wbad-function-cast -Winline -Wundef -Wnested-externs -Wcast-qual -Wshadow \
-Wwrite-strings -Wno-unused-parameter -Wfloat-equal -Wpedantic
INC = -I/usr/X11R6/include -I/usr/include/freetype2
STD = -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -DXINERAMA
LIB = -L/usr/X11R6/lib -lX11 -lXinerama -lfontconfig -lXft -lxkbcommon

CFLAGS = $(INC) $(STD) -Os
LDFLAGS = $(LIB)
