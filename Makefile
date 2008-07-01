builddir=.build-$(shell hostname)-$(shell gcc -dumpmachine)-$(shell gcc -dumpversion)

all: cmake
	@echo "Building…"
	make -C build

install: cmake
	@echo "Installing…"
	make -C build install

cmake: build CMakeLists.txt
CMakeLists.txt:	awesomeConfig.cmake
awesomeConfig.cmake:
	@echo "Running cmake…"
	cd ${builddir} && cmake "$@" ..

build:
	@echo -n "Creating new build directory…"
	@mkdir -p ${builddir}
	@echo " done"

	@echo -n "Setting up links…"
	@rm build
	@ln -s ${builddir} build

	@rm awesome awesome-client
	@ln -s ${builddir}/awesome awesome
	@ln -s ${builddir}/awesome-client awesome-client
	@echo " done"

clean:
	@echo -n "Cleaning up build directory…"
	@rm -rf ${builddir}
	@echo " done"

.PHONY: clean
