builddir=.build-$(shell hostname)-$(shell gcc -dumpmachine)-$(shell gcc -dumpversion)

ifeq (,$(VERBOSE))
    MAKEFLAGS:=$(MAKEFLAGS)s
    ECHO=echo
else
    ECHO=@:
endif

all: cmake
	$(ECHO) "Building…"
	$(MAKE) -C build

install: cmake
	$(ECHO) "Installing…"
	$(MAKE) -C build install

cmake: build/cmake-stamp
build/cmake-stamp: build CMakeLists.txt awesomeConfig.cmake
	$(ECHO) "Running cmake…"
	cd ${builddir} && cmake "$@" ..
	touch ${builddir}/cmake-stamp

build: awesome awesome-client
	$(ECHO) -n "Creating new build directory…"
	mkdir -p ${builddir}
	ln -s -f ${builddir} build
	$(ECHO) " done"

awesome:
	@ln -s -f ${builddir}/awesome awesome

awesome-client:
	@ln -s -f ${builddir}/awesome-client awesome-client

clean:
	$(ECHO) -n "Cleaning up build directory…"
	@rm -rf ${builddir} build
	$(ECHO) " done"

.PHONY: clean
