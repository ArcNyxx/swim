# swim - simple window manager
# Copyright (C) 2022 ArcNyxx
# see LICENCE file for licensing information

.POSIX:

include config.mk

SRC = drw.c swim.c util.c xerr.c
HEAD = drw.h struct.h util.h xerr.h
OBJ = $(SRC:.c=.o)

all: swim

options:
	@echo swim build options:
	@echo "COMPILER = $(CC)"
	@echo "CFLAGS   = $(CFLAGS)"
	@echo "LDFLAGS  = $(LDFLAGS)"

$(OBJ): $(HEAD) config.h config.mk

config.h:
	cp config.def.h $@

.c.o:
	$(CC) -c $(CFLAGS) $<

swim: $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

clean:
	rm -f swim $(OBJ) swim-*.tar.gz

devclean: clean
	rm -f config.h

devswim: devclean install

dist: clean
	mkdir -p swim-$(VERSION)
	cp -R LICENSE README Makefile config.mk config.def.h \
		$(SRC) $(HEAD) swim.1 swim-$(VERSION)
	tar -cf swim-$(VERSION).tar swim-$(VERSION)
	gzip swim-$(VERSION).tar
	rm -rf swim-$(VERSION)

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f swim $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/swim
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	sed "s/VERSION/$(VERSION)/g" < swim.1 > \
		$(DESTDIR)$(MANPREFIX)/man1/swim.1
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/swim.1

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/swim \
		$(DESTDIR)$(MANPREFIX)/man1/swim.1

.PHONY: all options clean devclean devswim dist install uninstall
