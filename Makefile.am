BUILT_SOURCES =
CLEANFILES =
EXTRA_DIST =
SUFFIXES =
bin_PROGRAMS =
doc_DATA =
man_MANS =
iconslayoutsdir = $(pkgdatadir)/icons/layouts
dist_iconslayouts_DATA =

LAYOUTS =

LAYOUTS += layouts/tile.c
LAYOUTS += layouts/tile.h

LAYOUTS += layouts/floating.c
LAYOUTS += layouts/floating.h

LAYOUTS += layouts/max.c
LAYOUTS += layouts/max.h

LAYOUTS += layouts/fibonacci.c
LAYOUTS += layouts/fibonacci.h


WIDGETS =
WIDGETS += widgets/taglist.c
WIDGETS += widgets/layoutinfo.c
WIDGETS += widgets/textbox.c
WIDGETS += widgets/iconbox.c
WIDGETS += widgets/focusicon.c
WIDGETS += widgets/progressbar.c
WIDGETS += widgets/tasklist.c
WIDGETS += widgets/graph.c


doc_DATA   += README

doc_DATA   += UPGRADE
EXTRA_DIST += UPGRADE

EXTRA_DIST += LICENSE
EXTRA_DIST += UPGRADE

EXTRA_DIST += STYLE
doc_DATA   += STYLE


EXTRA_DIST    += awesomerc.in
CLEANFILES    += awesomerc
BUILT_SOURCES += awesomerc
doc_DATA      += awesomerc
awesomerc: $(srcdir)/awesomerc.in
	$(SED) -e "s|[@]iconslayoutsdir@|$(iconslayoutsdir)|" \
		< $(srcdir)/awesomerc.in > awesomerc


CLEANFILES    += defconfig.h
BUILT_SOURCES += defconfig.h
defconfig.h: awesomerc
	@echo "generating defconfig.h from awesomerc"
	@echo "static const char *AWESOME_DEFAULT_CONFIG = " > defconfig.h
	@echo -n "\"" >> defconfig.h
	@$(SED) 's,\\,\\\\,g;s/$$/ \\n\\/;s/"/\\"/g' "awesomerc" >> defconfig.h
	@echo "\";" >> defconfig.h

CLEANFILES    += uicbgen.h
BUILT_SOURCES += uicbgen.h
EXTRA_DIST    += build-utils/uicbgen.sh
uicbgen.h: $(awesome_SOURCES)
	@echo "generating uicbgen.h from headers file"
	$(top_srcdir)/build-utils/uicbgen.sh "$(top_srcdir)" > uicbgen.h

CLEANFILES    += layoutgen.h
BUILT_SOURCES += layoutgen.h
EXTRA_DIST    += build-utils/layoutgen.sh
layoutgen.h: $(awesome_SOURCES)
	@echo "generating layoutgen.h from layouts headers file"
	$(top_srcdir)/build-utils/layoutgen.sh "$(top_srcdir)" > layoutgen.h

CLEANFILES    += widgetgen.h
BUILT_SOURCES += widgetgen.h
EXTRA_DIST    += build-utils/widgetgen.sh
widgetgen.h: widget.h
	@echo "generating widgetgen.h from widget.h"
	$(top_srcdir)/build-utils/widgetgen.sh "$(top_srcdir)" > widgetgen.h

A_V = awesome-version-internal
CLEANFILES    += $(A_V).h
BUILT_SOURCES += $(A_V).h.stamp
$(A_V).h.stamp:
	@current_ver=`$(SHELL) $(top_srcdir)/build-utils/package-version $(top_srcdir) version-stamp`; \
	{ echo '#ifndef AWESOME_VERSION_INTERNAL'; \
	  echo "#define AWESOME_VERSION_INTERNAL \"$${current_ver}\""; \
	  echo "#endif"; } > "$(A_V).h.new"
	@if test -f "$(A_V).h" \
        && cmp "$(A_V).h.new" "$(A_V).h"; then :; \
	else cat "$(A_V).h.new" > "$(A_V).h"; fi; \
	rm -f "$(A_V).h.new"

if USING_GCC
# If you are using gcc, and want to deactivate this default set of
# compile flags, run make with AWESOME_CFLAGS="".
AWESOME_CFLAGS = -std=gnu99 -pipe \
		 -Wall -Wextra \
		 -Wundef -Wshadow -Wcast-align -Wwrite-strings -Wsign-compare \
		 -Wunused -Winit-self -Wpointer-arith -Wredundant-decls \
		 -Wmissing-prototypes -Wmissing-format-attribute -Wmissing-noreturn
endif
AM_CPPFLAGS = $(XFT_CFLAGS) $(X_CFLAGS) $(CAIRO_CFLAGS) $(CONFUSE_CFLAGS) $(XRANDR_CFLAGS) $(XINERAMA_CFLAGS) $(AWESOME_CFLAGS)

bin_PROGRAMS += awesome
awesome_SOURCES = \
	client.c client.h \
	placement.c placement.h \
	common/version.c common/version.h \
	focus.c focus.h \
	common/draw.c common/draw.h \
	event.c event.h \
	layout.c layout.h \
	awesome.c awesome.h \
	tag.c tag.h \
	common/util.c common/util.h \
	xutil.c xutil.h \
	config.c config.h \
	screen.c screen.h \
	statusbar.c statusbar.h \
	uicb.c uicb.h \
	window.c window.h \
	rules.c rules.h \
	mouse.c mouse.h \
	common/socket.c common/socket.h \
	common/swindow.c common/swindow.h \
	widget.c widget.h \
	ewmh.c ewmh.h \
	common/list.h structs.h \
	common/configopts.h common/configopts.c
awesome_SOURCES += $(LAYOUTS)
awesome_SOURCES += $(WIDGETS)
awesome_LDADD = $(XFT_LIBS) $(X_LIBS) $(CAIRO_LIBS) $(CONFUSE_LIBS) $(XRANDR_LIBS) $(XINERAMA_LIBS)

bin_PROGRAMS += awesome-client
awesome_client_SOURCES = \
	awesome-client.c \
	common/socket.c common/socket.h \
	common/version.c common/version.h \
	common/util.c common/util.h

bin_PROGRAMS += awesome-message
awesome_message_SOURCES = \
        common/swindow.c common/swindow.h \
        common/draw.c common/draw.h \
	common/util.h common/util.c \
	common/version.h common/version.c \
	common/configopts.h common/configopts.c \
	awesome-message.c

awesome_message_LDADD = $(XFT_LIBS) $(X_LIBS) $(CAIRO_LIBS) $(CONFUSE_LIBS)

if HAVE_XMLTO
if HAVE_ASCIIDOC
if XMLTO_MAN_WORKS
man_MANS   += awesome.1
man_MANS   += awesome-client.1
man_MANS   += awesome-message.1
man_MANS   += awesomerc.5
endif
endif
endif

EXTRA_DIST += awesome.1.txt
EXTRA_DIST += awesome-client.1.txt
EXTRA_DIST += awesome-message.1.txt
EXTRA_DIST += awesomerc.5.txt

dist_iconslayouts_DATA += icons/layouts/dwindle.png
dist_iconslayouts_DATA += icons/layouts/dwindlew.png
dist_iconslayouts_DATA += icons/layouts/floating.png
dist_iconslayouts_DATA += icons/layouts/floatingw.png
dist_iconslayouts_DATA += icons/layouts/max.png
dist_iconslayouts_DATA += icons/layouts/maxw.png
dist_iconslayouts_DATA += icons/layouts/spiral.png
dist_iconslayouts_DATA += icons/layouts/spiralw.png
dist_iconslayouts_DATA += icons/layouts/tile.png
dist_iconslayouts_DATA += icons/layouts/tilebottom.png
dist_iconslayouts_DATA += icons/layouts/tilebottomw.png
dist_iconslayouts_DATA += icons/layouts/tileleft.png
dist_iconslayouts_DATA += icons/layouts/tileleftw.png
dist_iconslayouts_DATA += icons/layouts/tiletop.png
dist_iconslayouts_DATA += icons/layouts/tiletopw.png
dist_iconslayouts_DATA += icons/layouts/tilew.png

clean-local:
	rm -f awesome*.1 awesome*.1.xml awesome*.5 awesome*.5.xml

SUFFIXES += .1.xml .1
SUFFIXES += .5.xml .5
if HAVE_XMLTO
.1.xml.1:
	$(XMLTO) man $<
.5.xml.5:
	$(XMLTO) man $<
endif

SUFFIXES += .1.txt .1.xml
SUFFIXES += .5.txt .5.xml
if HAVE_ASCIIDOC
.1.txt.1.xml:
	$(ASCIIDOC) -d manpage -b docbook -o $@ $<
.5.txt.5.xml:
	$(ASCIIDOC) -d manpage -b docbook -o $@ $<
endif


# Check that package version matches git version before creating dist tarballs
dist-hook: git-version-check
distcheck-hook: git-version-check

# Note: We cannot run autogen.sh from here, because we would need some way to
#       restart the whole dist process from the start and there is none.
EXTRA_DIST += build-utils/package-version
git-version-check:
	@git_ver=`$(top_srcdir)/build-utils/package-version $(top_srcdir) version-stamp`; \
	if test "x$${git_ver}" = "x$(PACKAGE_VERSION)"; then :; else \
		echo "ERROR: PACKAGE_VERSION and 'git describe' version do not match:"; \
		echo "         current 'git describe' version: $${git_ver}"; \
		echo "         current PACKAGE_VERSION:        $(PACKAGE_VERSION)"; \
		echo "Update PACKAGE_VERSION by running $(top_srcdir)/autogen.sh."; \
		exit 1; \
	fi

# Version stamp files can only exist in tarball source trees.
#
# So there is no need to generate them anywhere else or to clean them
# up anywhere.
dist-hook:
	echo "$(PACKAGE_VERSION)" > "$(distdir)/version-stamp"

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


# Presuming we are running awesome on this screen, and this is screen 0, ...
# ...we replace the running awesome instance by a freshly compiled one.
restart-uninstalled: awesome awesome-client
	screen=`echo "$${DISPLAY}" | sed 's/.*:[0-9]\.\([0-9]*\)$$/\1/; s/.*:\([0-9]*\)$$/0/'`; \
	echo "$${screen}" exec "$${PWD}/awesome" | "$${PWD}/awesome-client"

# ...we replace the running awesome instance by the last installed one.
restart-installed:
	screen=`echo "$${DISPLAY}" | sed 's/.*:[0-9]\.\([0-9]*\)$$/\1/; s/.*:\([0-9]*\)$$/0/'`; \
	echo "$${screen}" exec "$(DESTDIR)$(bindir)/awesome" | "$(DESTDIR)$(bindir)/awesome-client"

