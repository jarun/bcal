PREFIX ?= /usr/local
BINDIR = $(DESTDIR)$(PREFIX)/bin
MANDIR = $(DESTDIR)$(PREFIX)/share/man/man1
DOCDIR = $(DESTDIR)$(PREFIX)/share/doc/bcal
STRIP ?= strip

CFLAGS += -Wall -Wextra -Wno-unused-parameter -Werror
LDLIBS += -lreadline

SRC = $(wildcard src/*.c)
INCLUDE = -Iinc

bcal: $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDE) -o bcal $(SRC) $(LDLIBS)

all: bcal

x86: $(SRC)
	$(CC) -m64 $(CFLAGS) $(LDFLAGS) $(INCLUDE) -o bcal $(SRC) $(LDLIBS)
	strip bcal

distclean: clean
	rm -f *~

install: bcal
	install -m 0755 -d $(BINDIR)
	install -m 0755 -d $(MANDIR)
	install -m 0755 -d $(DOCDIR)
	install -m 0755 bcal $(BINDIR)
	gzip -c bcal.1 > bcal.1.gz
	install -m 0644 bcal.1.gz $(MANDIR)
	install -m 0644 README.md $(DOCDIR)
	rm -f bcal.1.gz

uninstall:
	rm -f $(BINDIR)/bcal
	rm -f $(MANDIR)/bcal.1.gz
	rm -rf $(DOCDIR)

strip: bcal
	$(STRIP) $^

clean:
	-rm -f bcal

skip: ;

.PHONY: bcal all x86 distclean install uninstall strip clean
