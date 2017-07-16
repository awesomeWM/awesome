# This Makefile handles building and testing awesomeWM in Docker containers.
#
# Tests can be run using "docker_test", and a single test file can be given:
#
#     % make docker_test TEST_RUN_ARGS=test-menubar.lua

# Path to the main build dir.  We use a "docker" subdirectory in there.
BUILDDIR=../build

AWESOME_UID=$(shell id -u)
DOCKER_RUN_ARGS:=-e AWESOME_UID=$(AWESOME_UID)

DOCKER_VOLUME_CCACHE=awesome-ccache
DOCKER_VOLUME_BUILD:=awesome-build-$(LUA_PKG)

# Args for docker-build.
LUA_PKG:=lua5.3
LUA_LIB_PREFIX:=$(if $(filter luajit,$(LUA_PKG)),lua5.1,$(LUA_PKG))
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

DOCKER_SUFFIX=$(LUA_PKG)$(if $(DOCKER_FLAVOR),-$(DOCKER_FLAVOR))

$(BUILDDIR)/stamp.docker_image_deps.$(DOCKER_SUFFIX): Makefile Dockerfile | $(BUILDDIR_VOLUME)
$(BUILDDIR)/stamp.docker_image_deps.$(DOCKER_SUFFIX):
	$(if $(LUA_PKG),,$(error LUA_PKG is missing, use e.g. lua5.3 or luajit.))
	cd .. && docker build \
	  --build-arg DOCKER_FLAVOR=$(DOCKER_FLAVOR) \
	  --build-arg LUA_PKG=$(LUA_PKG) \
	  --build-arg LUA_LIB_PREFIX=$(LUA_LIB_PREFIX) \
	  --build-arg DO_COVERAGE=$(DO_COVERAGE) \
	  -f docker/Dockerfile -t awesomewm/awesome-base-$(DOCKER_SUFFIX) .
	touch $@

ifeq ($(CI), true)
DOCKER_RUN_ARGS+=-e CI -e TRAVIS -e SHIPPABLE -e TRAVIS_BRANCH -e TRAVIS_COMMIT -e TRAVIS_JOB_NUMBER -e TRAVIS_PULL_REQUEST -e TRAVIS_JOB_ID -e TRAVIS_REPO_SLUG -e TRAVIS_TAG -e TRAVIS_OS_NAME
endif

docker_test: TEST_RUN_ARGS:=
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

docker_test_sh: DOCKER_RUN_ARGS:=--entrypoint=sh
docker_test_sh: docker_test

docker_test_make: DOCKER_RUN_ARGS:=--entrypoint=make
docker_test_make: docker_test
