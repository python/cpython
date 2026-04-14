# test-standalone.mk - Standalone deployment mode tests
#
# Copyright(c) The Maintainers of Nanvix.
# Licensed under the MIT License.
#
# In standalone mode the runtime filesystem is a ramfs image.  Binaries
# (nanvixd.elf, python3.12, etc.) are NOT included in the ramfs — they are
# passed to nanvixd via -bin-dir.  The ramfs contains only the stdlib and
# test fixtures.
#
# A single full-test ramfs image (~103 MB) is built once during ramfs-stage.
# All test modules are included in this image — no per-batch ramfs rebuilding.
# run-tests.py spawns parallel nanvixd instances that share this image, each
# running a 4-module batch in a fresh VM process (avoiding heap fragmentation
# OOM).
#
# regrtest is used for all modes including standalone.  A /tmp directory is
# created on the ramfs so tempfile.gettempdir() works.  Per-mode test
# exclusions use regrtest --ignore via NANVIX_EXCLUDE_TESTS.

include .nanvix/mk/test-common.mk

# Separate directory for ramfs content (copy of sysroot for ramfs image).
# The main TEST_STAGING/sysroot is left intact so binaries remain available
# for the nanvixd invocation on the host.
# Cached under .nanvix/_ramfs_cache/ so it survives test-stage wipes.
TEST_RAMFS_CONTENT = $(CURDIR)/.nanvix/_ramfs_cache
RAMFS_STAGING = $(TEST_RAMFS_CONTENT)
RAMFS_IMG = /tmp/cpython-rootfs.img
MKRAMFS = $(abspath $(NANVIX_HOME))/bin/mkramfs.elf

# Per-mode test exclusions for standalone (passed to regrtest --ignore).
#   test_filter_dealloc: creates 1M nested filter objects, OOMs the 32MB heap.
NANVIX_STANDALONE_EXCLUDE = test_filter_dealloc

include .nanvix/mk/ramfs.mk

# ramfs-stage: copy the full sysroot (including tests) and build a single
# ramfs image.  The sysroot is trimmed with RAMFS_KEEP_TESTS=1 so that
# lib/python3.12/test/ and config-3.12 are preserved.  The resulting image
# (~103 MB) contains all test modules and fits within the 256 MB VM budget.
# The sysroot copy and ramfs image persist across make test runs; they are
# stored under .nanvix/_ramfs_cache/ and /tmp/cpython-rootfs.img respectively.
# test-cleanup does NOT remove the ramfs image; only test-distclean clears
# both the cache directory and the ramfs image.  Delete the cache directory
# (or run `make test-distclean`) to force a full rebuild.
ramfs-stage: test-stage
	@if [ -f $(RAMFS_IMG) ] && [ -d $(TEST_RAMFS_CONTENT)/sysroot ]; then \
		echo "  Using cached ramfs: $(RAMFS_IMG)"; \
	else \
		rm -rf $(TEST_RAMFS_CONTENT); \
		mkdir -p $(TEST_RAMFS_CONTENT); \
		cp -a $(TEST_STAGING)/sysroot $(TEST_RAMFS_CONTENT)/sysroot; \
		mkdir -p $(TEST_RAMFS_CONTENT)/sysroot/tmp; \
		$(MAKE) -f Makefile.nanvix ramfs-build \
			RAMFS_STAGING="$(TEST_RAMFS_CONTENT)" \
			RAMFS_IMG="$(RAMFS_IMG)" \
			RAMFS_KEEP_TESTS=1 \
			CONFIG_NANVIX=$(CONFIG_NANVIX) \
			NANVIX_HOME=$(NANVIX_HOME); \
	fi

# test-hello-standalone: run hello test via nanvixd with the full-test ramfs
test-hello-standalone: ramfs-stage
	@cp "$(MKRAMFS)" $(TEST_STAGING)/sysroot/bin/ 2>/dev/null || true
ifdef CONFIG_NANVIX_DOCKER
	$(DOCKER_RUN) sh -c '\
		if [ -x "$(DOCKER_TOOLCHAIN_PATH)/bin/i686-nanvix-strip" ] && [ -f "$(DOCKER_WORKSPACE_PATH)/.nanvix/_test_staging/sysroot/bin/python3.12" ]; then \
			"$(DOCKER_TOOLCHAIN_PATH)/bin/i686-nanvix-strip" --strip-debug \
				"$(DOCKER_WORKSPACE_PATH)/.nanvix/_test_staging/sysroot/bin/python3.12" || true; \
		fi'
else
	$(if $(wildcard $(TOOLCHAIN_PREFIX)/bin/i686-nanvix-strip),\
		$(TOOLCHAIN_PREFIX)/bin/i686-nanvix-strip --strip-debug $(TEST_STAGING)/sysroot/bin/python3.12 2>/dev/null || true)
endif
	@echo "Test: Hello world (standalone)..."
	cd $(TEST_STAGING)/sysroot && \
		{ \
			: > /tmp/cpython_test.log; \
			start_time=$$(date +%s%N); \
			timeout 120 ./bin/nanvixd.elf \
			  -bin-dir ./bin -ramfs $(RAMFS_IMG) $(NANVIXD_EXTRA_ARGS) \
			  -- ./bin/python3.12 \
			  "-B ./test_hello.py;PYTHONHOME=/ PYTHONDONTWRITEBYTECODE=1" \
			  < /dev/null > /tmp/cpython_test.log 2>&1; \
			test_status=$$?; \
			end_time=$$(date +%s%N); \
			elapsed=$$(( (end_time - start_time) / 1000000 )); \
			echo "  Execution time: $${elapsed} ms"; \
			if [ $$test_status -ne 0 ]; then \
				echo "  FAIL: Hello test timed out or exited with status $$test_status"; cat /tmp/cpython_test.log; exit 1; \
			fi; \
		}
	$(call validate-hello,/tmp/cpython_test.log)

# test-regrtest-standalone: parallel batched regrtest via run-tests.py.
#
# Uses a single pre-built full-test ramfs image shared by all parallel
# nanvixd instances.  Each batch runs 4 modules in a fresh VM process.
# No per-batch ramfs rebuilding needed.
test-regrtest-standalone: ramfs-stage
ifneq ($(NANVIX_RELEASE),yes)
	@echo "Test: regrtest ($(words $(NANVIX_TEST_LIST)) modules, standalone)..."
	cd $(TEST_STAGING)/sysroot && \
		NANVIX_STANDALONE=1 \
		NANVIXD_EXTRA_ARGS="-bin-dir ./bin -ramfs $(RAMFS_IMG) $(NANVIXD_EXTRA_ARGS)" \
		NANVIX_EXCLUDE_TESTS="$(NANVIX_STANDALONE_EXCLUDE)" \
		NANVIX_TEST_BATCH_SIZE=$(NANVIX_TEST_BATCH_SIZE) \
		python3 $(CURDIR)/.nanvix/run-tests.py $(NANVIX_TEST_LIST)
else
	@echo "Test: regrtest skipped (NANVIX_RELEASE=yes)"
endif

# Aggregate test target for standalone mode
test: test-hello-standalone test-regrtest-standalone test-cleanup

.PHONY: ramfs-stage test-hello-standalone test-regrtest-standalone test
