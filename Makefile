builddir=.build-$(shell hostname)-$(shell gcc -dumpmachine)-$(shell gcc -dumpversion)

ifeq (,$(VERBOSE))
    MAKEFLAGS:=$(MAKEFLAGS)s
    ECHO=echo
else
    ECHO=@:
endif

TARGETS=awesome
BUILDLN=build

all: $(TARGETS) $(BUILDLN) ;

$(TARGETS): cmake-build
	ln -s -f ${builddir}/$@ $@

$(BUILDLN):
	test -e $(BUILDLN) || ln -s -f ${builddir} $(BUILDLN)

cmake ${builddir}/CMakeCache.txt:
	$(ECHO) "Creating build directory and running cmake in it. You can also run CMake directly, if you want."
	$(ECHO)
	mkdir -p ${builddir}
	$(ECHO) "Running cmake…"
	cd ${builddir} && cmake $(CMAKE_ARGS) "$(@D)" ..

cmake-build: ${builddir}/CMakeCache.txt
	$(ECHO) "Building…"
	$(MAKE) -C ${builddir}

tags:
	git ls-files | xargs ctags

install:
	$(ECHO) "Installing…"
	$(MAKE) -C ${builddir} install

distclean:
	$(ECHO) -n "Cleaning up build directory…"
	$(RM) -r ${builddir} $(BUILDLN) $(TARGETS)
	$(ECHO) " done"

%: cmake
	$(ECHO) "Running make $@…"
	$(MAKE) -C ${builddir} $@
	$(and $(filter clean,$@),$(RM) $(BUILDLN) $(TARGETS))

.PHONY: cmake-build cmake install distclean $(BUILDLN) tags
