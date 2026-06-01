# whk - wayland hotkey daemon
include config.mk

.POSIX:
.SUFFIXES:

# TODO: clean this up, how do i get suffix rules to work with paths
.SUFFIXES: .xml .h .c .o

all: whd

.xml.h:
	wayland-scanner client-header < $< > $@
.xml.c:
	wayland-scanner private-code < $< > $@

.c.o:
	${CC} -c ${CFLAGS} $<

ext-action-binder-v1.h: ${PROTOPREFIX}/wayland-protocols/staging/ext-action-binder/ext-action-binder-v1.xml
	wayland-scanner client-header < $< > $@
ext-action-binder-v1.c: ${PROTOPREFIX}/wayland-protocols/staging/ext-action-binder/ext-action-binder-v1.xml
	wayland-scanner private-code < $< > $@

config.h: config.def.h
	cp --backup $< $@

wayland.o: wayland.c wayland.h ext-action-binder-v1.h

whd.o: whd.c config.h

whd: whd.o wayland.o ext-action-binder-v1.o
	${CC} -o $@ $^ ${LDFLAGS}

clean:
	rm -f whd whd.o ext-action-binder-v1.* whd-${VERSION}.tar.gz

dist: clean
	mkdir -p whd-${VERSION}
	cp LICENSE Makefile README config.def.h config.mk\
		whd.1 ${SRC} whd-${VERSION}
	tar -cf whd-${VERSION}.tar whd-${VERSION}
	gzip whd-${VERSION}.tar
	rm -rf whd-${VERSION}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f whd ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/whd
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	sed "s/VERSION/${VERSION}/g" < whd.1 > ${DESTDIR}${MANPREFIX}/man1/whd.1
	chmod 644 ${DESTDIR}${MANPREFIX}/man1/whd.1

uninstall:
	rm -rf ${DESTDIR}${PREFIX}/bin/whd\
		${DESTDIR}${MANPREFIX}/man1/whd.1

.PHONY: all clean dist install uninstall
