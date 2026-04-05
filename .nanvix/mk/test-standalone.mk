# test-standalone.mk - Standalone deployment mode tests
#
# Copyright(c) The Maintainers of Nanvix.
# Licensed under the MIT License.
#
# In standalone mode the runtime filesystem is a ramfs image.  Binaries
# (nanvixd.elf, python3.12, etc.) are NOT included in the ramfs — they are
# passed to nanvixd via -bin-dir.  The ramfs contains only the stdlib and
# test fixtures.

include .nanvix/mk/test-common.mk

# Separate directory for ramfs content (trimmed copy of sysroot).
# The main TEST_STAGING/sysroot is left intact so binaries remain available
# for the nanvixd invocation on the host.
TEST_RAMFS_CONTENT = $(TEST_STAGING)/ramfs-content
RAMFS_STAGING = $(TEST_RAMFS_CONTENT)
RAMFS_IMG = /tmp/cpython-rootfs.img

include .nanvix/mk/ramfs.mk

# test-hello-standalone: build ramfs from a trimmed copy, run hello test via nanvixd
test-hello-standalone: test-stage
	@# Prepare ramfs content: copy sysroot into a separate tree, then trim it.
	@# This keeps the original staging tree with binaries intact for invocation.
	@rm -rf $(TEST_RAMFS_CONTENT)
	@mkdir -p $(TEST_RAMFS_CONTENT)/sysroot
	@cp -a $(TEST_STAGING)/sysroot/lib $(TEST_RAMFS_CONTENT)/sysroot/lib 2>/dev/null || true
	@cp $(TEST_STAGING)/sysroot/test_hello.py $(TEST_RAMFS_CONTENT)/sysroot/ 2>/dev/null || true
	$(MAKE) -f Makefile.nanvix ramfs-build \
		RAMFS_STAGING="$(TEST_RAMFS_CONTENT)" \
		RAMFS_IMG="$(RAMFS_IMG)" \
		CONFIG_NANVIX=$(CONFIG_NANVIX) \
		NANVIX_HOME=$(NANVIX_HOME)
	@cp "$(abspath $(NANVIX_HOME))/bin/mkramfs.elf" $(TEST_STAGING)/sysroot/bin/ 2>/dev/null || true
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
			  "-B /test_hello.py;PYTHONHOME=/ PYTHONDONTWRITEBYTECODE=1" \
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
	@rm -f $(RAMFS_IMG)
	@rm -rf $(TEST_RAMFS_CONTENT)

# test-regrtest-standalone: regrtest is skipped in standalone mode (ramfs memory limit)
test-regrtest-standalone:
	@echo "Test: regrtest ($(words $(NANVIX_TEST_LIST)) modules)..."
	@echo "  SKIP: regrtest skipped for standalone mode (ramfs memory limit)"

# Aggregate test target for standalone mode
test: test-hello-standalone test-regrtest-standalone test-cleanup

.PHONY: test-hello-standalone test-regrtest-standalone test
