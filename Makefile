CC = gcc
CFLAGS = -Wall -Wextra -Werror -O3
LDLIBS = -lm

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
	install -d ${DESTDIR}/usr/bin/
	install -g staff -o root ${<} ${DESTDIR}/usr/bin/

uninstall: calb
	rm -f ${DESTDIR}/usr/bin/calb
