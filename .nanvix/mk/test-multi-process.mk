# test-multi-process.mk - Multi-process deployment mode tests
#
# Copyright(c) The Maintainers of Nanvix.
# Licensed under the MIT License.
#
# In multi-process mode the runtime accesses files directly via the host
# filesystem — no ramfs image is needed.

include .nanvix/mk/test-common.mk

# test-hello-multi-process-prepare: build and install into staging.
test-hello-multi-process-prepare: test-stage

# test-hello-multi-process-run: execute hello test via nanvixd.
# Runs on the host — no cross-toolchain needed.
test-hello-multi-process-run:
	@echo "Test: Hello world..."
	cd $(TEST_STAGING)/sysroot && \
		{ \
			: > /tmp/cpython_test.log; \
			start_time=$$(date +%s%N); \
			timeout 120 ./bin/nanvixd.elf $(NANVIXD_EXTRA_ARGS) -- ./bin/python3.12 ./test_hello.py < /dev/null > /tmp/cpython_test.log 2>&1; \
			test_status=$$?; \
			end_time=$$(date +%s%N); \
			elapsed=$$(( (end_time - start_time) / 1000000 )); \
			echo "  Execution time: $${elapsed} ms"; \
			if [ $$test_status -ne 0 ]; then \
				echo "  FAIL: Hello test timed out or exited with status $$test_status"; cat /tmp/cpython_test.log; exit 1; \
			fi; \
		}
	$(call validate-hello,/tmp/cpython_test.log)

# test-hello-multi-process: combined prepare + run (for native Linux builds)
test-hello-multi-process: test-hello-multi-process-prepare test-hello-multi-process-run

# test-regrtest-multi-process-prepare: build and install into staging.
test-regrtest-multi-process-prepare: test-stage

# test-regrtest-multi-process-run: execute stdlib regression tests via nanvixd.
test-regrtest-multi-process-run:
ifneq ($(NANVIX_RELEASE),yes)
	@echo "Test: regrtest ($(words $(NANVIX_TEST_LIST)) modules)..."
	cd $(TEST_STAGING)/sysroot && \
		{ \
			: > /tmp/cpython_regrtest.log; \
			timeout 600 ./bin/nanvixd.elf $(NANVIXD_EXTRA_ARGS) -- ./bin/python3.12 -m test \
			  --timeout=120 $(NANVIX_TEST_LIST) \
			  < /dev/null > /tmp/cpython_regrtest.log 2>&1; \
			regrtest_status=$$?; \
			if [ $$regrtest_status -ne 0 ]; then \
				echo "  FAIL: regrtest exited with status $$regrtest_status"; cat /tmp/cpython_regrtest.log; exit 1; \
			fi; \
			echo "  PASS: regrtest completed"; \
		}
else
	@echo "Test: regrtest skipped (NANVIX_RELEASE=yes)"
endif

# test-regrtest-multi-process: combined prepare + run (for native Linux builds)
test-regrtest-multi-process: test-regrtest-multi-process-prepare test-regrtest-multi-process-run

# Aggregate test target for multi-process mode
test: test-hello-multi-process test-regrtest-multi-process test-cleanup

.PHONY: test-hello-multi-process-prepare test-hello-multi-process-run test-hello-multi-process test-regrtest-multi-process-prepare test-regrtest-multi-process-run test-regrtest-multi-process test
