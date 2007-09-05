# jdwm - Julien's dynamic window manager
# © 2007 Julien Danjou
# © 2006-2007 Anselm R. Garbe, Sander van Dijk

include config.mk

SRC = client.c draw.c event.c layout.c jdwm.c tag.c util.c config.c
OBJ = ${SRC:.c=.o} ${LAYOUTS:.c=.o}

all: options jdwm

options:
	@echo jdwm build options:
	@echo "LAYOUTS  = ${LAYOUTS}"
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	@echo -e \\t\(CC\) $<
	@${CC} -c ${CFLAGS} $< -o $@

${OBJ}: jdwm.h config.mk

jdwm: ${OBJ}
	@echo -e \\t\(CC\) ${OBJ} -o $@
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f jdwm ${OBJ} jdwm-${VERSION}.tar.gz
	@rm -rf doc

dist: clean
	@echo creating dist tarball
	@mkdir -p jdwm-${VERSION}
	@cp -R LICENSE Makefile README config.*.h config.mk \
	    jdwm.1 jdwm.h grid.h tile.h mem.h ${SRC} ${LAYOUTS} jdwm-${VERSION}
	@tar -cf jdwm-${VERSION}.tar jdwm-${VERSION}
	@gzip jdwm-${VERSION}.tar
	@rm -rf jdwm-${VERSION}

install: all
	@echo installing executable file to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f jdwm ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/jdwm
	@echo installing manual page to ${DESTDIR}${MANPREFIX}/man1
	@mkdir -p ${DESTDIR}${MANPREFIX}/man1
	@sed "s/VERSION/${VERSION}/g" < jdwm.1 > ${DESTDIR}${MANPREFIX}/man1/jdwm.1
	@chmod 644 ${DESTDIR}${MANPREFIX}/man1/jdwm.1

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/jdwm
	@echo removing manual page from ${DESTDIR}${MANPREFIX}/man1
	@rm -f ${DESTDIR}${MANPREFIX}/man1/jdwm.1

doc:
	@echo generating documentation
	@doxygen jdwm.doxygen

.PHONY: all options clean dist install uninstall doc
