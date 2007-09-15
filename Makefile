# awesome
# © 2007 Julien Danjou

include config.mk

SRC = client.c draw.c event.c layout.c awesome.c tag.c util.c config.c screen.c statusbar.c
OBJ = ${SRC:.c=.o} ${LAYOUTS:.c=.o}

all: options awesome

options:
	@echo awesome build options:
	@echo "LAYOUTS  = ${LAYOUTS}"
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	@echo -e \\t\(CC\) $<
	@${CC} -c ${CFLAGS} $< -o $@

${OBJ}: awesome.h config.mk

awesome: ${OBJ}
	@echo -e \\t\(CC\) ${OBJ} -o $@
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f awesome ${OBJ} awesome-${VERSION}.tar.gz
	@rm -rf doc

dist: clean
	@echo creating dist tarball
	@mkdir -p awesome-${VERSION}
	@cp -R LICENSE Makefile README config.*.h config.mk \
	    awesome.1 awesome.h grid.h tile.h mem.h ${SRC} ${LAYOUTS} awesome-${VERSION}
	@tar -cf awesome-${VERSION}.tar awesome-${VERSION}
	@gzip awesome-${VERSION}.tar
	@rm -rf awesome-${VERSION}

install: all
	@echo installing executable file to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f awesome ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/awesome
	@echo installing manual page to ${DESTDIR}${MANPREFIX}/man1
	@mkdir -p ${DESTDIR}${MANPREFIX}/man1
	@sed "s/VERSION/${VERSION}/g" < awesome.1 > ${DESTDIR}${MANPREFIX}/man1/awesome.1
	@chmod 644 ${DESTDIR}${MANPREFIX}/man1/awesome.1

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/awesome
	@echo removing manual page from ${DESTDIR}${MANPREFIX}/man1
	@rm -f ${DESTDIR}${MANPREFIX}/man1/awesome.1

doc:
	@echo generating documentation
	@doxygen awesome.doxygen

.PHONY: all options clean dist install uninstall doc
