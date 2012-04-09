BIN = $(DESTDIR)/usr/bin
ICONS = $(DESTDIR)/usr/share/puckman/images
ICON = $(DESTDIR)/usr/share/pixmaps
SYMICON = $(DESTDIR)/usr/share/icons/hicolor/48x48/apps
SHELL = /bin/sh
CC = gcc
prefix = /usr
includedir = $(prefix)/include
pacdir = ~/.puckman
puckman: puckman.c
	$(CC) -Wall -I$(includedir)/SDL $< -o $@ -lSDL -lSDL_image -lSDL_gfx -lm
	if test -d $(pacdir); then echo "$(pacdir) already exists, skipping."; else mkdir $(pacdir); fi

install: puckman
	install -d $(BIN) $(ICONS) $(ICON) $(SYMICON)
	install ./puckman $(BIN)
	install -m644 images/*.{gif,png} $(ICONS)
	install -m644 images/puckman.png $(ICON)
	install -m644 images/puckman.png $(SYMICON)

clean:
	rm puckman

uninstall:
	rm -vr $(ICONS) $(BIN)/puckman $(ICON)/puckman.png $(SYMICON)/puckman.png
	if test -e $(DESTDIR)/usr/share/applications/puckman.desktop; then rm -v $(DESTDIR)/usr/share/applications/puckman.desktop; fi
