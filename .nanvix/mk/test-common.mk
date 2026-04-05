# test-common.mk - Common test infrastructure
#
# Copyright(c) The Maintainers of Nanvix.
# Licensed under the MIT License.
#
# Provides shared test staging setup used by all deployment modes.
# The actual test execution is delegated to deployment-mode-specific includes
# (test-standalone.mk, test-multi-process.mk, test-single-process.mk).
#
# Required variables (set by includer before including this file):
#   TEST_STAGING  - directory for the staged test install

ifndef _TEST_COMMON_MK
_TEST_COMMON_MK := 1

# Staging directory for tests (inside workspace so Docker can access it)
TEST_STAGING ?= $(CURDIR)/.nanvix/_test_staging

# Stage the CPython install and test fixtures into TEST_STAGING.
# After this target, $(TEST_STAGING)/sysroot/ contains the full install tree
# plus the hello test script and Nanvix runtime binaries.
test-stage: build
	@echo "Running CPython tests on Nanvix..."
	@rm -rf $(TEST_STAGING)
	@# Install python and stdlib into a test staging area
ifdef CONFIG_NANVIX_DOCKER
	$(DOCKER_RUN) make install DESTDIR="$(DOCKER_WORKSPACE_PATH)/.nanvix/_test_staging"
else
	make install DESTDIR="$(TEST_STAGING)"
endif
	@# Copy test script into the staging sysroot
	@echo "import sys; print('CPYTHON_TEST_HELLO: Hello from Python', sys.version_info[:2])" > $(TEST_STAGING)/sysroot/test_hello.py
	@# Copy Nanvix runtime binaries into the staging sysroot
	@mkdir -p $(TEST_STAGING)/sysroot/bin
	@cp "$(abspath $(NANVIX_HOME))/bin/nanvixd.elf" $(TEST_STAGING)/sysroot/bin/ 2>/dev/null || true
	@cp "$(abspath $(NANVIX_HOME))/bin/kernel.elf"  $(TEST_STAGING)/sysroot/bin/ 2>/dev/null || true
	@cp "$(abspath $(NANVIX_HOME))/bin/linuxd.elf"  $(TEST_STAGING)/sysroot/bin/ 2>/dev/null || true
	@cp "$(abspath $(NANVIX_HOME))/bin/uservm.elf"  $(TEST_STAGING)/sysroot/bin/ 2>/dev/null || true

# Validate hello-world test output in a log file.
# Usage: $(call validate-hello,/path/to/logfile)
define validate-hello
	@if grep -q '^CPYTHON_TEST_HELLO:' $(1); then \
		echo "  PASS: $$(grep -m 1 '^CPYTHON_TEST_HELLO:' $(1))"; \
	else \
		echo "  FAIL: Hello test did not produce expected output"; cat $(1); exit 1; \
	fi
endef

# Clean up test artifacts.
test-cleanup:
	@rm -rf $(TEST_STAGING) /tmp/cpython_test.log /tmp/cpython_regrtest.log /tmp/cpython-rootfs.img
	@echo "		*** CPython tests PASSED ***"

.PHONY: test-stage test-cleanup

endif # _TEST_COMMON_MK
