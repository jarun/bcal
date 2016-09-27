CC = gcc
CFLAGS = -Wall -Wextra -Werror -O3
LDLIBS = -lquadmath

PREFIX ?= /usr/local
BINDIR = $(DESTDIR)$(PREFIX)/bin
MANDIR = $(DESTDIR)$(PREFIX)/share/man/man1
DOCDIR = $(DESTDIR)$(PREFIX)/share/doc/calb

all: calb

calb: calb.c
	$(CC) $(CFLAGS) -o calb calb.c $(LDLIBS)
	strip calb

.PHONY: clean
clean:
	-rm -f calb

distclean: clean
	rm -f *~

install: calb
	install -m755 -d $(BINDIR)
	install -m755 -d $(MANDIR)
	install -m755 -d $(DOCDIR)
	gzip -c calb.1 > calb.1.gz
	install -m755 calb $(BINDIR)
	install -m644 calb.1.gz $(MANDIR)
	install -m644 README.md $(DOCDIR)
	rm -f calb.1.gz

uninstall: calb
	rm -f $(BINDIR)/calb
	rm -f $(MANDIR)/calb.1.gz
	rm -rf $(DOCDIR)
