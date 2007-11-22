# awesome
# © 2007 Julien Danjou <julien@danjou.info>

include config.mk

SRC = client.c draw.c event.c layout.c awesome.c tag.c util.c xutil.c config.c screen.c statusbar.c uicb.c window.c rules.c mouse.c awesome-client-common.c
OBJ = ${SRC:.c=.o} ${LAYOUTS:.c=.o}
DOCS = awesome.1.txt

SRCCLIENT = awesome-client.c awesome-client-common.c util.c
OBJCLIENT = ${SRCCLIENT:.c=.o}

all: options awesome.1 awesome awesome-client

options:
	@echo awesome build options:
	@echo "LAYOUTS  = ${LAYOUTS}"
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	@echo -e "\t(CC) $<"
	@${CC} -c ${CFLAGS} $< -o $@

${OBJ}: awesome.h config.mk

${OBJCLIENT}: config.mk

awesome.1.xml: $(DOCS)
	asciidoc -d manpage -b docbook $<

awesome.1: ${DOCS:.txt=.xml}
	xmlto man $<

awesome-client: ${OBJCLIENT}
	@echo -e "\t(CC) ${OBJCLIENT} -o $@"
	@${CC} -o $@ ${OBJCLIENT} ${CLIENTLDFLAGS}

awesome: ${OBJ}
	@echo -e "\t(CC) ${OBJ} -o $@"
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f awesome awesome-client awesome.1 ${DOCS:.txt=.xml} ${OBJCLIENT} ${OBJ} awesome-${VERSION}.tar.gz
	@rm -rf doc

dist: clean
	@echo creating dist tarball
	@mkdir awesome-${VERSION}
	@mkdir awesome-${VERSION}/layouts
	@cp -fR STYLE LICENSE AUTHORS Makefile README awesomerc config.mk \
	    awesome.1 ${SRCCLIENT} ${SRCCLIENT:.c=.h} ${SRC} ${SRC:.c=.h} \
	    common.h awesome-${VERSION} || true
	@cp -R ${LAYOUTS} ${LAYOUTS:.c=.h} awesome-${VERSION}/layouts
	@tar -cf awesome-${VERSION}.tar awesome-${VERSION}
	@gzip -9 awesome-${VERSION}.tar
	@rm -rf awesome-${VERSION}

strip: all
	strip awesome
	strip awesome-client

install: strip install-unstrip

install-unstrip:
	@echo installing executable file to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f awesome awesome-client ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/awesome
	@chmod 755 ${DESTDIR}${PREFIX}/bin/awesome-client
	@echo installing manual page to ${DESTDIR}${MANPREFIX}/man1
	@mkdir -p ${DESTDIR}${MANPREFIX}/man1
	@sed "s/VERSION/${VERSION}/g" < awesome.1 > ${DESTDIR}${MANPREFIX}/man1/awesome.1
	@chmod 644 ${DESTDIR}${MANPREFIX}/man1/awesome.1

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/awesome
	@rm -f ${DESTDIR}${PREFIX}/bin/awesome-client
	@echo removing manual page from ${DESTDIR}${MANPREFIX}/man1
	@rm -f ${DESTDIR}${MANPREFIX}/man1/awesome.1

doc:
	@echo generating documentation
	@doxygen awesome.doxygen

.PHONY: all options clean dist install uninstall doc
