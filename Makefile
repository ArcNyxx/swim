# swim - simple window manager
# Copyright (C) 2022 ArcNyxx
# see LICENCE file for licensing information

.POSIX:

include config.mk

SRC  = act.c bar.c config.c conv.c drw.c evt.c func.c \
	grab.c tile.c utf8.c util.c xerr.c swim.c
HEAD = act.h bar.h config.h conv.h drw.h evt.h func.h \
	grab.h tile.h utf8.h util.h xerr.h struct.h
OBJ  = $(SRC:.c=.o)

all: swim

$(OBJ): $(HEAD) config.mk

.c.o:
	$(CC) -c $(CFLAGS) $<

swim: $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

clean:
	rm -f swim $(OBJ) swim-*.tar.gz

dist: clean
	mkdir -p swim-$(VERSION)
	cp -R LICENSE README Makefile config.mk $(SRC) $(HEAD) swim.1 \
		swim-$(VERSION)
	tar -cf - swim-$(VERSION) | gzip -c > swim-$(VERSION).tar.gz
	rm -rf swim-$(VERSION)

install: all
	mkdir -p $(PREFIX)/bin $(MANPREFIX)/man1
	cp -f swim $(PREFIX)/bin
	chmod 755 $(PREFIX)/bin/swim
	sed 's/VERSION/$(VERSION)/g' < swim.1 > $(MANPREFIX)/man1/swim.1
	chmod 644 $(MANPREFIX)/man1/swim.1

uninstall:
	rm -f $(PREFIX)/bin/swim $(MANPREFIX)/man1/swim.1

.PHONY: all clean dist install uninstall
