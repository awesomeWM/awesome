BUILT_SOURCES =
CLEANFILES =
EXTRA_DIST =
SUFFIXES =
bin_PROGRAMS =
doc_DATA =
man_MANS =

LAYOUTS =
WIDGETS =

include layouts/Makefile-files
include widgets/Makefile-files

EXTRA_DIST += awesomerc
doc_DATA   += awesomerc

doc_DATA   += README

EXTRA_DIST += LICENSE
doc_DATA   += LICENSE

EXTRA_DIST += STYLE
doc_DATA   += STYLE

CLEANFILES    += defconfig.h
BUILT_SOURCES += defconfig.h
defconfig.h: $(srcdir)/awesomerc
	@echo "generating defconfig.h from $(srcdir)/awesomerc"
	@echo "#define AWESOME_DEFAULT_CONFIG \\" > defconfig.h
	@echo -n "\"" >> defconfig.h
	@$(SED) 's,\\,\\\\,g;s/$$/ \\/;s/"/\\"/g' "$(srcdir)/awesomerc" >> defconfig.h
	@echo "\"" >> defconfig.h

AWESOME_CFLAGS = -std=gnu99 -ggdb3 -pipe \
		 -Wall -Wextra \
		 -Wundef -Wshadow -Wcast-align -Wwrite-strings -Wsign-compare \
		 -Wunused -Winit-self -Wpointer-arith -Wredundant-decls \
		 -Wmissing-prototypes -Wmissing-format-attribute -Wmissing-noreturn
AM_CPPFLAGS = $(XFT_CFLAGS) $(X_CFLAGS) $(CAIRO_CFLAGS) $(CONFUSE_CFLAGS) $(XRANDR_CFLAGS) $(XINERAMA_CFLAGS) $(AWESOME_CFLAGS)

bin_PROGRAMS += awesome
awesome_SOURCES = \
	client.c client.h \
	focus.c focus.h \
	draw.c draw.h \
	event.c event.h \
	layout.c layout.h \
	awesome.c awesome.h \
	tag.c tag.h \
	util.c util.h \
	xutil.c xutil.h \
	config.c config.h \
	screen.c screen.h \
	statusbar.c statusbar.h \
	uicb.c uicb.h \
	window.c window.h \
	rules.c rules.h \
	mouse.c mouse.h \
	awesome-client-common.c \
	widget.c widget.h \
	ewmh.c ewmh.h
awesome_SOURCES += $(LAYOUTS)
awesome_SOURCES += $(WIDGETS)
awesome_LDADD = $(XFT_LIBS) $(X_LIBS) $(CAIRO_LIBS) $(CONFUSE_LIBS) $(XRANDR_LIBS) $(XINERAMA_LIBS)

bin_PROGRAMS += awesome-client
awesome_client_SOURCES = \
	awesome-client.c awesome-client.h \
	awesome-client-common.c \
	util.c util.h

EXTRA_DIST += awesome.1.txt
man_MANS   += awesome.1

EXTRA_DIST += awesome-client.1.txt
man_MANS   += awesome-client.1

EXTRA_DIST += awesomerc.1.txt
man_MANS   += awesomerc.1

clean-local:
	rm -f awesome*.1 awesome*.1.xml

SUFFIXES += .1.xml .1
if HAVE_XMLTO
.1.xml.1:
	$(XMLTO) man $<
endif

SUFFIXES += .1.txt .1.xml
if HAVE_ASCIIDOC
.1.txt.1.xml:
	$(ASCIIDOC) -d manpage -b docbook -o $@ $<
endif

EXTRA_DIST += awesome.doxygen.in

# Workaround for buggy pre-1.5.3 doxygen.
#
# Some pre-1.5.3 versions of doxygen segfault on reading these lines.
# Call make with DOXYGEN_SEGFAULT=-segfault as a workaround.
CLEANFILES += awesome.doxygen-segfault
awesome.doxygen-segfault: awesome.doxygen
	$(SED) '/^QT_AUTOBRIEF/d; /^EXTRACT_ANON_NSPACES/d; /^HTML_DYNAMIC_SECTIONS/d' \
		< awesome.doxygen > awesome.doxygen-segfault

if HAVE_DOXYGEN
doc: doc/html/index.html

doc/html/index.html: awesome.doxygen$(DOXYGEN_SEGFAULT) $(awesome_SOURCES) $(awesome_client_SOURCES)
	$(DOXYGEN) awesome.doxygen$(DOXYGEN_SEGFAULT)
endif
