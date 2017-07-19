# This Makefile handles building and testing awesomeWM in Docker containers.
#
# Tests can be run using "docker_test", and a single test file can be given:
#
#     % make docker_test TEST_RUN_ARGS=test-menubar.lua

# Path to the main build dir.  We use a "docker" subdirectory in there.
BUILDDIR=../build

AWESOME_UID=$(shell id -u)
DOCKER_RUN_ARGS+=-e AWESOME_UID=$(AWESOME_UID)

DOCKER_VOLUME_CCACHE=awesome-ccache
DOCKER_VOLUME_BUILD:=awesome-build-$(LUA_PKG)$(if $(DO_COVERAGE),-coverage)

# Args for docker-build.
LUA_PKG:=lua5.3
LUA_LIB_PREFIX:=$(if $(filter luajit,$(LUA_PKG)),lua5.1,$(LUA_PKG))
CMAKE_ARGS+=$(if $(filter luajit,$(LUA_PKG)),-DLUA_INCLUDE_DIR=/usr/include/luajit-2.1 -DLUA_LIBRARY=/usr/lib/libluajit-5.1.so)
DOCKER_FLAVOR:=

$(BUILDDIR):
	mkdir -p $@

BUILDDIR_VOLUME:=$(BUILDDIR)/docker
$(BUILDDIR_VOLUME):
	mkdir -p $@

docker_clean:
	$(RM) $(BUILDDIR)/stamp.docker_*
	$(RM) -rf $(BUILDDIR)/docker

	volumes="$$(docker volume ls -q | grep -E '^awesome-(build-|ccache)')"; \
	  if [ -n "$$volumes" ]; then \
	    echo "Removing Docker volumes: $$volumes"; \
	    for v in $$volumes; do \
	      docker volume rm $$v >/dev/null; \
	    done; \
	  fi

DOCKER_SUFFIX=$(LUA_PKG)$(if $(DOCKER_FLAVOR),-$(DOCKER_FLAVOR))$(if $(DO_COVERAGE),-cov_$(DO_COVERAGE))

DOCKER_IMGS_DIR?=

$(BUILDDIR)/stamp.docker_image_deps.$(DOCKER_SUFFIX): Makefile Dockerfile | $(BUILDDIR_VOLUME)
$(BUILDDIR)/stamp.docker_image_deps.$(DOCKER_SUFFIX):
	$(if $(LUA_PKG),,$(error LUA_PKG is missing, use e.g. lua5.3 or luajit.))
	set -ex; if [ -n "${DOCKER_IMGS_DIR}" ] && [ -e "${DOCKER_IMGS_DIR}/base-${DOCKER_SUFFIX}" ]; then \
	  docker load -i "${DOCKER_IMGS_DIR}/base-${DOCKER_SUFFIX}"; \
	else \
	  cd .. && docker build \
	    --build-arg DOCKER_FLAVOR=$(DOCKER_FLAVOR) \
	    --build-arg LUA_PKG=$(LUA_PKG) \
	    --build-arg LUA_LIB_PREFIX=$(LUA_LIB_PREFIX) \
	    --build-arg DO_COVERAGE=$(DO_COVERAGE) \
	    --build-arg CMAKE_ARGS="$(CMAKE_ARGS)" \
	    -f docker/Dockerfile -t awesomewm/awesome-base-$(DOCKER_SUFFIX) . ; \
	  if [ -n "${DOCKER_IMGS_DIR}" ]; then \
	    mkdir -p ${DOCKER_IMGS_DIR}; \
	    docker save awesomewm/awesome-base-$(DOCKER_SUFFIX) > "${DOCKER_IMGS_DIR}/base-${DOCKER_SUFFIX}"; \
	  fi; \
	fi
	touch $@

docker_test: DOCKER_FLAVOR:=test
docker_test: $(BUILDDIR)/stamp.docker_image_deps.$(DOCKER_SUFFIX)
	docker run -ti --rm $(DOCKER_RUN_ARGS) \
	  -e CI=true -e VERBOSE -e DO_COVERAGE \
	  -v $(CURDIR)/..:/src/awesome:ro \
	  -v $(DOCKER_VOLUME_CCACHE):/tmp/ccache \
	  -e CCACHE_DIR=/tmp/ccache \
	  -v $(DOCKER_VOLUME_BUILD):/src/awesome/build \
	  awesomewm/awesome-base-$(DOCKER_SUFFIX) \
	  $(TEST_RUN_ARGS)

docker_test_sh: override TEST_RUN_ARGS:=
docker_test_sh: DOCKER_RUN_ARGS+=--entrypoint=sh
docker_test_sh: docker_test

docker_test_make: DOCKER_RUN_ARGS+=--entrypoint=make
docker_test_make: docker_test
