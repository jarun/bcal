CC = gcc
CFLAGS = -Wall -Wextra -Werror -O3 -m64
LDLIBS = -lquadmath

PREFIX ?= /usr/local
BINDIR = $(DESTDIR)$(PREFIX)/bin
MANDIR = $(DESTDIR)$(PREFIX)/share/man/man1
DOCDIR = $(DESTDIR)$(PREFIX)/share/doc/bcal

all: bcal

bcal: bcal.c
	$(CC) $(CFLAGS) -o bcal bcal.c $(LDLIBS)
	strip bcal

.PHONY: clean
clean:
	-rm -f bcal

distclean: clean
	rm -f *~

install: bcal
	install -m755 -d $(BINDIR)
	install -m755 -d $(MANDIR)
	install -m755 -d $(DOCDIR)
	gzip -c bcal.1 > bcal.1.gz
	install -m755 bcal $(BINDIR)
	install -m644 bcal.1.gz $(MANDIR)
	install -m644 README.md $(DOCDIR)
	rm -f bcal.1.gz

uninstall:
	rm -f $(BINDIR)/bcal
	rm -f $(MANDIR)/bcal.1.gz
	rm -rf $(DOCDIR)
