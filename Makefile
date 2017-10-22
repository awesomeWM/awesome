ifeq (,$(VERBOSE))
    MAKEFLAGS:=$(MAKEFLAGS)s
    ECHO=echo
else
    ECHO=@:
endif

TARGETS=awesome
BUILDDIR=build

all: $(TARGETS) ;

$(TARGETS): cmake-build

$(BUILDDIR)/Makefile:
	$(ECHO) "Creating build directory and running cmake in it. You can also run CMake directly, if you want."
	$(ECHO)
	mkdir -p $(BUILDDIR)
	$(ECHO) "Running cmake…"
	cd $(BUILDDIR) && cmake $(CMAKE_ARGS) "$(@D)" ..

cmake-build: $(BUILDDIR)/Makefile
	$(ECHO) "Building…"
	$(MAKE) -C $(BUILDDIR)

tags:
	git ls-files | xargs ctags

install:
	$(ECHO) "Installing…"
	$(MAKE) -C $(BUILDDIR) install

distclean:
	$(ECHO) -n "Cleaning up build directory…"
	$(RM) -r $(BUILDDIR)

# Use an explicit rule to not "update" the Makefile via the implicit rule below.
Makefile: ;

%: $(BUILDDIR)/Makefile
	$(ECHO) "Running make $@ in $(BUILDDIR)…"
	$(MAKE) -C $(BUILDDIR) $@

.PHONY: cmake-build install distclean tags
