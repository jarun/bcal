CC = gcc
CFLAGS = -Wall -Wextra -Werror
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

# just for checkinstall
install: lsstack
	install -d ${DESTDIR}/usr/bin/
	install -g staff -o root ${<} ${DESTDIR}/usr/bin/
