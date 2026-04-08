# docker-host.mk - Windows Docker host-side re-invocation
#
# Copyright(c) The Maintainers of Nanvix.
# Licensed under the MIT License.
#
# When DOCKER_HOST_MODE=windows is passed, all build targets are re-invoked
# inside a Docker container with tar-based source copying to avoid the
# Docker Desktop bind mount I/O penalty.
#
# The inner make invocation runs without DOCKER_HOST_MODE, so it uses the
# native toolchain inside the container (no Docker wrapping).

ifdef DOCKER_HOST_MODE

include $(dir $(lastword $(MAKEFILE_LIST)))defaults.mk

# Docker paths for the inner build
_DHM_BUILD_DIR  := /tmp/build
_DHM_WS_MOUNT   := /mnt/workspace
_DHM_SYS_MOUNT  := /mnt/sysroot

# Files that need CRLF -> LF normalization for autotools
_DHM_CRLF_FILES := configure config.guess config.sub install-sh \
    Modules/makesetup Modules/Setup \
    Modules/Setup.bootstrap.in Modules/Setup.stdlib.in \
    Modules/config.c.in Modules/ld_so_aix.in \
    Makefile.pre.in pyconfig.h.in aclocal.m4 configure.ac \
    Misc/python.pc.in Misc/python-embed.pc.in \
    Misc/python-config.sh.in Misc/python-config.in

# Tar excludes (saves ~200MB of transfer)
_DHM_TAR_EXCLUDES := --exclude=.git --exclude=.nanvix/venv \
    --exclude=.nanvix/cache --exclude=.nanvix/sysroot \
    --exclude=.nanvix/buildroot --exclude=Doc \
    --exclude=Lib/idlelib --exclude=Lib/tkinter \
    --exclude=Lib/turtledemo --exclude=Lib/ensurepip \
    --exclude=PC --exclude=PCbuild

# Inner make arguments (no DOCKER_HOST_MODE => runs natively inside container)
_DHM_INNER_ARGS := make -f Makefile.nanvix \
    CONFIG_NANVIX=y \
    NANVIX_HOME=$(_DHM_SYS_MOUNT) \
    NANVIX_TOOLCHAIN=/opt/nanvix \
    PLATFORM=$(PLATFORM) \
    PROCESS_MODE=$(PROCESS_MODE) \
    MEMORY_SIZE=$(MEMORY_SIZE) \
    INSTALL_PREFIX=$(INSTALL_PREFIX) \
    NANVIX_RELEASE=$(NANVIX_RELEASE)

# Build output files to copy back to the host workspace
_DHM_OUTPUT_FILES := python python.exe python.elf python.wasm libpython3.12.a

# docker-host-run: sync sources into container, fix CRLF, run inner make,
# copy outputs back.
#   $(1) = inner make targets
define docker-host-run
docker run --rm \
    -v $(CURDIR):$(_DHM_WS_MOUNT) \
    -v $(abspath $(NANVIX_HOME)):$(_DHM_SYS_MOUNT):ro \
    -w $(_DHM_BUILD_DIR) \
    -e HOME=/tmp \
    $(NANVIX_DOCKER_IMAGE) \
    sh -c '\
        mkdir -p $(_DHM_BUILD_DIR) && \
        tar -cf - -C $(_DHM_WS_MOUNT) $(_DHM_TAR_EXCLUDES) . \
            | tar -xf - -C $(_DHM_BUILD_DIR) && \
        for f in $(_DHM_CRLF_FILES); do \
            if [ -f "$$f" ]; then sed "s/\r$$//" "$$f" > /tmp/_crlf.tmp && \
            cat /tmp/_crlf.tmp > "$$f"; fi; done && \
        rm -f .nanvix-configured conftest* config.cache && \
        $(_DHM_INNER_ARGS) $(1); rc=$$?; \
        for f in $(_DHM_OUTPUT_FILES); do \
            [ -f $(_DHM_BUILD_DIR)/$$f ] && \
            cp -f $(_DHM_BUILD_DIR)/$$f $(_DHM_WS_MOUNT)/$$f; \
        done; exit $$rc'
endef

# docker-host-test-prepare: like docker-host-run but also copies the test
# staging directory back to the host so that nanvixd can run natively.
#   $(1) = inner make prepare target(s)
define docker-host-test-prepare
docker run --rm \
    -v $(CURDIR):$(_DHM_WS_MOUNT) \
    -v $(abspath $(NANVIX_HOME)):$(_DHM_SYS_MOUNT):ro \
    -w $(_DHM_BUILD_DIR) \
    -e HOME=/tmp \
    $(NANVIX_DOCKER_IMAGE) \
    sh -c '\
        mkdir -p $(_DHM_BUILD_DIR) && \
        tar -cf - -C $(_DHM_WS_MOUNT) $(_DHM_TAR_EXCLUDES) . \
            | tar -xf - -C $(_DHM_BUILD_DIR) && \
        for f in $(_DHM_CRLF_FILES); do \
            if [ -f "$$f" ]; then sed "s/\r$$//" "$$f" > /tmp/_crlf.tmp && \
            cat /tmp/_crlf.tmp > "$$f"; fi; done && \
        rm -f .nanvix-configured conftest* config.cache && \
        $(_DHM_INNER_ARGS) $(1); rc=$$?; \
        for f in $(_DHM_OUTPUT_FILES); do \
            [ -f $(_DHM_BUILD_DIR)/$$f ] && \
            cp -f $(_DHM_BUILD_DIR)/$$f $(_DHM_WS_MOUNT)/$$f; \
        done; \
        rm -rf $(_DHM_WS_MOUNT)/.nanvix/_test_staging; \
        if [ -d $(_DHM_BUILD_DIR)/.nanvix/_test_staging ]; then \
            cp -a $(_DHM_BUILD_DIR)/.nanvix/_test_staging \
                   $(_DHM_WS_MOUNT)/.nanvix/_test_staging; \
            chmod -R a+rwX $(_DHM_WS_MOUNT)/.nanvix/_test_staging; \
        fi; \
        if [ -f /tmp/cpython-rootfs.img ]; then \
            cp -f /tmp/cpython-rootfs.img \
                   $(_DHM_WS_MOUNT)/.nanvix/_test_staging/cpython-rootfs.img; \
        fi; \
        exit $$rc'
endef

# Test staging directory on the host (populated by docker-host-test-prepare)
_DHM_TEST_STAGING := $(CURDIR)/.nanvix/_test_staging

# Determine the prepare target name and test script content based on
# deployment mode.  The test script runs via WSL on the host.
ifeq ($(PROCESS_MODE),standalone)
  _DHM_TEST_PREPARE := test-hello-standalone-prepare test-regrtest-standalone
  _DHM_RAMFS_IMG := $(_DHM_TEST_STAGING)/cpython-rootfs.img
else ifeq ($(PROCESS_MODE),single-process)
  _DHM_TEST_PREPARE := test-hello-single-process-prepare test-regrtest-single-process-prepare
else
  _DHM_TEST_PREPARE := test-hello-multi-process-prepare test-regrtest-multi-process-prepare
endif

# Convert a Windows path to a WSL /mnt/ path via wslpath at Make parse time.
_dhm_to_wsl = $(shell wsl wslpath -a '$(subst \,/,$(1))')

# Path to the host-side test runner script (runs natively on Windows and Linux)
_DHM_TEST_RUNNER := $(dir $(lastword $(MAKEFILE_LIST)))test-run-host.py

# Regrtest arguments: pass test list for non-release, non-standalone builds
ifeq ($(PROCESS_MODE),standalone)
  _DHM_REGRTEST_ARGS :=
else ifeq ($(NANVIX_RELEASE),yes)
  _DHM_REGRTEST_ARGS :=
else
  _DHM_REGRTEST_ARGS := $(NANVIX_TEST_LIST)
endif

# Override main targets when running from Windows host
all: build

build:
	$(call docker-host-run,build)

# Test: cross-compile and stage inside Docker, then run nanvixd on the host.
# Phase 1 (Docker): build, install, ramfs, strip — everything needing the
#   cross-toolchain or Linux tools.
# Phase 2 (host): execute nanvixd.elf natively on Windows.
#   nanvixd.elf is a Linux ELF binary that runs via Windows WSL interop.
#   The Python test runner invokes it directly (no explicit WSL call).
test:
	@echo "=== Phase 1: Preparing test artifacts inside Docker ==="
	$(call docker-host-test-prepare,$(_DHM_TEST_PREPARE))
	@echo "=== Phase 2: Running tests on host ==="
	python $(_DHM_TEST_RUNNER) $(subst /,\,$(_DHM_TEST_STAGING)) $(PROCESS_MODE) $(_DHM_REGRTEST_ARGS)
	-rmdir /s /q $(subst /,\,$(_DHM_TEST_STAGING)) 2>nul
	@echo "		*** CPython tests PASSED ***"

install:
	$(call docker-host-run,install)

package:
	$(call docker-host-run,package)

# Clean does not need Docker; just remove local artifacts
clean:
	-del /f .nanvix-configured python.elf python.exe 2>nul
	-rmdir /s /q .nanvix\_test_staging 2>nul
	-rmdir /s /q _test_staging 2>nul
	-rmdir /s /q staging 2>nul

distclean: clean

.PHONY: all build test install package clean distclean

endif # DOCKER_HOST_MODE
