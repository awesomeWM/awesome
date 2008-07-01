builddir=.build-$(shell hostname)-$(shell gcc -dumpmachine)-$(shell gcc -dumpversion)

all: cmake
	@echo "Building…"
	make -C build

install: cmake
	@echo "Installing…"
	make -C build install

cmake: build/cmake-stamp
build/cmake-stamp: build CMakeLists.txt awesomeConfig.cmake
	@echo "Running cmake…"
	cd ${builddir} && cmake "$@" ..
	touch ${builddir}/cmake-stamp

build: awesome awesome-client
	@echo -n "Creating new build directory…"
	@mkdir -p ${builddir}
	@ln -s -f ${builddir} build
	@echo " done"

awesome:
	@ln -s -f ${builddir}/awesome awesome

awesome-client:
	@ln -s -f ${builddir}/awesome-client awesome-client

clean:
	@echo -n "Cleaning up build directory…"
	@rm -rf ${builddir} build
	@echo " done"

.PHONY: clean
