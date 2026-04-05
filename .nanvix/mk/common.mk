# common.mk - Shared variables, toolchain detection, configure, build, install, clean
#
# Copyright(c) The Maintainers of Nanvix.
# Licensed under the MIT License.

# Nanvix Docker image for cross-compilation (used as fallback if native toolchain not found)
NANVIX_DOCKER_IMAGE ?= nanvix/toolchain:latest-minimal

# Platform and deployment configuration (passed by nanvix-zutil / z.py)
PLATFORM ?= hyperlight
PROCESS_MODE ?= multi-process
MEMORY_SIZE ?= 128mb

# Set to 'yes' for release packaging: disables C test extension modules and
# skips regrtest.  Dev builds (the default) compile test extensions and run
# regrtest so that every `./z test` invocation exercises the test suite.
# NOTE: changing this between builds requires 'make -f Makefile.nanvix distclean'
# followed by a full rebuild so the new configure flags are picked up.
NANVIX_RELEASE ?= no

# Space-separated list of stdlib test modules to run via 'python3 -m test'.
# Keep this list to pure-Python, non-networking tests known to pass on Nanvix.
# Expand it as more tests are enabled (see issues linked from #320).
#
# Trimmed to validated-green modules only. The remaining modules fail due to
# missing platform capabilities (fork/exec, tempdir, asyncio event loop) or
# memory limits (test_json MemoryError). Those belong to #321 and #322 where
# each module gets individual skip/xfail annotations before being re-added.
#   Deferred: test_builtin test_dict test_list test_str test_tuple test_set
#             test_bytes test_int test_json test_datetime test_os test_pathlib
#             test_io
NANVIX_TEST_LIST ?= test_float test_complex test_bool test_struct

# Nanvix cross-compilation configuration
ifdef CONFIG_NANVIX
  NANVIX_TOOLCHAIN ?= /opt/nanvix
  NANVIX_HOME ?= $(HOME)/nanvix

  # Check if Docker is explicitly requested or native toolchain is not available
  ifndef CONFIG_NANVIX_DOCKER
    ifeq ($(wildcard $(NANVIX_TOOLCHAIN)/bin/i686-nanvix-gcc),)
      # Native toolchain not found, check for Docker
      DOCKER_AVAILABLE := $(shell command -v docker 2>/dev/null)
      ifdef DOCKER_AVAILABLE
        DOCKER_IMAGE_EXISTS := $(shell docker image inspect $(NANVIX_DOCKER_IMAGE) >/dev/null 2>&1 && echo yes)
        ifdef DOCKER_IMAGE_EXISTS
          # Use Docker for cross-compilation
          CONFIG_NANVIX_DOCKER := y
          $(info [INFO] Native toolchain not found at $(NANVIX_TOOLCHAIN), using Docker image $(NANVIX_DOCKER_IMAGE))
        else
          $(error Docker image $(NANVIX_DOCKER_IMAGE) not found. Run: docker pull $(NANVIX_DOCKER_IMAGE))
        endif
      else
        $(error Nanvix toolchain not found at $(NANVIX_TOOLCHAIN) and Docker is not available)
      endif
    endif
  else
    # Docker explicitly requested via CONFIG_NANVIX_DOCKER=y
    DOCKER_AVAILABLE := $(shell command -v docker 2>/dev/null)
    ifndef DOCKER_AVAILABLE
      $(error CONFIG_NANVIX_DOCKER=y but Docker is not installed)
    endif
    DOCKER_IMAGE_EXISTS := $(shell docker image inspect $(NANVIX_DOCKER_IMAGE) >/dev/null 2>&1 && echo yes)
    ifndef DOCKER_IMAGE_EXISTS
      $(error Docker image $(NANVIX_DOCKER_IMAGE) not found. Run: docker pull $(NANVIX_DOCKER_IMAGE))
    endif
    $(info [INFO] Using Docker image $(NANVIX_DOCKER_IMAGE) (explicitly requested))
  endif

  EXE=.elf

  ifdef CONFIG_NANVIX_DOCKER
    # Docker-based cross-compilation
    # All paths inside Docker
    DOCKER_TOOLCHAIN_PATH := /opt/nanvix
    DOCKER_SYSROOT_PATH := /mnt/sysroot
    DOCKER_WORKSPACE_PATH := /mnt/workspace
    DOCKER_UID := $(shell id -u)
    DOCKER_GID := $(shell id -g)
    DOCKER_RUN := docker run --rm --user $(DOCKER_UID):$(DOCKER_GID) \
      -v $(CURDIR):$(DOCKER_WORKSPACE_PATH) \
      -v $(abspath $(NANVIX_HOME)):$(DOCKER_SYSROOT_PATH):ro \
      -w $(DOCKER_WORKSPACE_PATH) \
      -e HOME=/tmp \
      $(NANVIX_DOCKER_IMAGE)

    # For Docker, use Docker-internal paths in configure
    TOOLCHAIN_PREFIX := $(DOCKER_TOOLCHAIN_PATH)
    SYSROOT_PATH := $(DOCKER_SYSROOT_PATH)

    # Libraries for linking (Docker paths)
    LIBC := $(DOCKER_TOOLCHAIN_PATH)/i686-nanvix/lib/libc.a
    LIBM := $(DOCKER_TOOLCHAIN_PATH)/i686-nanvix/lib/libm.a
    LIBPOSIX := $(DOCKER_SYSROOT_PATH)/lib/libposix.a
    LIBZ := $(DOCKER_SYSROOT_PATH)/lib/libz.a
    LIBSQLITE3 := $(DOCKER_SYSROOT_PATH)/lib/libsqlite3.a
    LIBSSL := $(DOCKER_SYSROOT_PATH)/lib/libssl.a
    LIBCRYPTO := $(DOCKER_SYSROOT_PATH)/lib/libcrypto.a
    BUILD_PYTHON := $(DOCKER_TOOLCHAIN_PATH)/bin/python3
  else
    # Native toolchain paths
    TOOLCHAIN_PREFIX := $(NANVIX_TOOLCHAIN)
    SYSROOT_PATH := $(abspath $(NANVIX_HOME))

    # Libraries for linking (native paths)
    LIBC := $(NANVIX_TOOLCHAIN)/i686-nanvix/lib/libc.a
    LIBM := $(NANVIX_TOOLCHAIN)/i686-nanvix/lib/libm.a
    LIBPOSIX := $(abspath $(NANVIX_HOME))/lib/libposix.a
    LIBZ := $(abspath $(NANVIX_HOME))/lib/libz.a
    LIBSQLITE3 := $(abspath $(NANVIX_HOME))/lib/libsqlite3.a
    LIBSSL := $(abspath $(NANVIX_HOME))/lib/libssl.a
    LIBCRYPTO := $(abspath $(NANVIX_HOME))/lib/libcrypto.a
    BUILD_PYTHON := $(NANVIX_TOOLCHAIN)/bin/python3
  endif

  # NANVIX_SYSROOT: exported so that Modules/Setup.local can reference
  # archive paths portably (e.g. $(NANVIX_SYSROOT)/lib/numpy/*.a).
  NANVIX_SYSROOT := $(SYSROOT_PATH)
  export NANVIX_SYSROOT

  # INSTALL_PREFIX: the prefix baked into the python binary (sys.prefix, sys.path).
  # Defaults to /sysroot which matches the runtime layout on Nanvix and keeps
  # DESTDIR installs predictable (files land under DESTDIR/sysroot/...).
  INSTALL_PREFIX ?= /sysroot
else
  # Allow clean target without CONFIG_NANVIX
  ifneq ($(MAKECMDGOALS),clean)
    ifneq ($(MAKECMDGOALS),distclean)
      $(error CONFIG_NANVIX must be set to 'y'. Usage: make -f Makefile.nanvix CONFIG_NANVIX=y NANVIX_HOME=/path/to/nanvix)
    endif
  endif
  EXE=.elf
endif

# Configure environment variables
CONFIGURE_ENV = \
	CC="$(TOOLCHAIN_PREFIX)/bin/i686-nanvix-gcc" \
	CXX="$(TOOLCHAIN_PREFIX)/bin/i686-nanvix-g++" \
	LD="$(TOOLCHAIN_PREFIX)/bin/i686-nanvix-ld" \
	AR="$(TOOLCHAIN_PREFIX)/bin/i686-nanvix-ar" \
	RANLIB="$(TOOLCHAIN_PREFIX)/bin/i686-nanvix-ranlib" \
	CFLAGS="-L$(SYSROOT_PATH)/lib -I$(SYSROOT_PATH)/include" \
	CFLAGS_NODIST="-fno-semantic-interposition" \
	LDFLAGS="-T$(SYSROOT_PATH)/lib/user.ld -Wl,--allow-multiple-definition -Wl,-pie -Wl,--export-dynamic -Wl,--no-dynamic-linker" \
	LIBS="-Wl,--start-group $(LIBPOSIX) $(LIBC) $(LIBM) -lsqlite3 -lssl -lcrypto -lz -lbz2 -lffi -Wl,--end-group" \
	LIBSQLITE3_LIBS="-L$(SYSROOT_PATH)/lib -lsqlite3" \
	LIBSQLITE3_CFLAGS="-I$(SYSROOT_PATH)/include" \
	ZLIB_LIBS="-L$(SYSROOT_PATH)/lib -lz" \
	ZLIB_CFLAGS="-I$(SYSROOT_PATH)/include" \
	BZIP2_LIBS="-L$(SYSROOT_PATH)/lib -lbz2" \
	BZIP2_CFLAGS="-I$(SYSROOT_PATH)/include" \
	LIBFFI_LIBS="-L$(SYSROOT_PATH)/lib -lffi" \
	LIBFFI_CFLAGS="-I$(SYSROOT_PATH)/include"

# Configure options for Nanvix
CONFIGURE_OPTS = \
	--with-lto \
	--disable-shared \
	--build=x86_64-pc-linux-gnux32 \
	--host=i686-nanvix \
	--with-build-python="$(BUILD_PYTHON)" \
	$(if $(filter yes,$(NANVIX_RELEASE)),--disable-test-modules,) \
	--with-libc="$(LIBC)" \
	--with-libm="$(LIBM)" \
	--prefix="$(INSTALL_PREFIX)" \
	--exec-prefix="$(INSTALL_PREFIX)" \
	--with-ensurepip=no \
	--with-pkg-config=no \
	--with-openssl="$(SYSROOT_PATH)" \
	--disable-ipv6 \
	ac_cv_file__dev_ptmx=no \
	ac_cv_file__dev_ptc=no \
	ac_cv_pthread_is_default=yes \
	ac_cv_pthread=yes \
	ac_cv_kthread=no \
	ac_cv_func_dlopen=yes \
	ac_cv_header_dlfcn_h=yes

# Marker file to track if configure has been run
CONFIGURED_MARKER = .nanvix-configured

# Configure target
configure: $(CONFIGURED_MARKER)

$(CONFIGURED_MARKER):
ifdef CONFIG_NANVIX_DOCKER
	@echo "Running configure inside Docker..."
	$(DOCKER_RUN) sh -c '$(CONFIGURE_ENV) ./configure $(CONFIGURE_OPTS)'
else
	$(CONFIGURE_ENV) ./configure $(CONFIGURE_OPTS)
endif
	touch $(CONFIGURED_MARKER)

# Build target
build: $(CONFIGURED_MARKER)
ifdef CONFIG_NANVIX_DOCKER
	@echo "Building inside Docker..."
	$(DOCKER_RUN) make -j$$(nproc) all
	@# Rename python to python.elf for consistency (always use the latest build)
	@if [ -f python ]; then mv -f python python$(EXE); fi
	@# Strip debug symbols from the final binary for size reduction
	$(DOCKER_RUN) sh -c '\
		if [ ! -x "$(DOCKER_TOOLCHAIN_PATH)/bin/i686-nanvix-strip" ]; then \
			echo "Warning: i686-nanvix-strip not found in Docker toolchain; skipping strip."; \
		elif [ ! -f "$(DOCKER_WORKSPACE_PATH)/python$(EXE)" ]; then \
			echo "Warning: $(DOCKER_WORKSPACE_PATH)/python$(EXE) not found; skipping strip."; \
		else \
			"$(DOCKER_TOOLCHAIN_PATH)/bin/i686-nanvix-strip" --strip-debug "$(DOCKER_WORKSPACE_PATH)/python$(EXE)" || { \
				echo "Error: failed to strip debug symbols from python$(EXE) inside Docker."; exit 1; }; \
		fi'
else
	make -j$$(nproc) all
	@# Rename python to python.elf for consistency (always use the latest build)
	@if [ -f python ]; then mv -f python python$(EXE); fi
	@# Strip debug symbols from the final binary for size reduction
	@if [ -x "$(TOOLCHAIN_PREFIX)/bin/i686-nanvix-strip" ]; then \
		if [ -f "python$(EXE)" ]; then \
			"$(TOOLCHAIN_PREFIX)/bin/i686-nanvix-strip" --strip-debug "python$(EXE)" || { \
				echo "Error: failed to strip debug symbols from python$(EXE)."; exit 1; }; \
		else \
			echo "Warning: python$(EXE) not found; skipping strip."; \
		fi; \
	else \
		echo "Warning: i686-nanvix-strip not found in $(TOOLCHAIN_PREFIX); skipping strip."; \
	fi
endif

# Install target (for Nanvix sysroot)
# Use DESTDIR for staged installs: make install DESTDIR=/path/to/staging
# When using Docker, DESTDIR must be under $(CURDIR) so it's accessible via the
# workspace volume mount.  The path is translated to its Docker-internal equivalent.
install: build
ifdef CONFIG_NANVIX_DOCKER
	$(DOCKER_RUN) make install DESTDIR="$(patsubst $(CURDIR)%,$(DOCKER_WORKSPACE_PATH)%,$(abspath $(DESTDIR)))"
else
	make install DESTDIR="$(DESTDIR)"
endif

# Clean target
clean:
	-make clean 2>/dev/null || true
	rm -f $(CONFIGURED_MARKER)
	rm -f python$(EXE)
	rm -rf $(TEST_STAGING)
	rm -rf $(CURDIR)/_test_staging $(CURDIR)/staging

# Distclean target - full cleanup
distclean: clean
	-make distclean 2>/dev/null || true
	git clean -fdx -e Makefile.nanvix -e NANVIX.md -e .github -e .nanvix/z.py -e .nanvix/nanvix.toml -e .nanvix/mk -e z -e z.sh -e z.ps1 2>/dev/null || true

.PHONY: configure build install clean distclean
