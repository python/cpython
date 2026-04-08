# test-single-process.mk - Single-process deployment mode tests
#
# Copyright(c) The Maintainers of Nanvix.
# Licensed under the MIT License.
#
# Single-process mode: placeholder for future implementation.
# Currently behaves identically to multi-process mode.

include .nanvix/mk/test-common.mk

# test-hello-single-process-prepare: build and install into staging.
test-hello-single-process-prepare: test-stage

# test-hello-single-process-run: execute hello test via nanvixd.
# Runs on the host — no cross-toolchain needed.
test-hello-single-process-run:
	@echo "Test: Hello world (single-process)..."
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

# test-hello-single-process: combined prepare + run (for native Linux builds)
test-hello-single-process: test-hello-single-process-prepare test-hello-single-process-run

# test-regrtest-single-process-prepare: build and install into staging.
test-regrtest-single-process-prepare: test-stage

# test-regrtest-single-process-run: execute stdlib regression tests via nanvixd.
test-regrtest-single-process-run:
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

# test-regrtest-single-process: combined prepare + run (for native Linux builds)
test-regrtest-single-process: test-regrtest-single-process-prepare test-regrtest-single-process-run

# Aggregate test target for single-process mode
test: test-hello-single-process test-regrtest-single-process test-cleanup

.PHONY: test-hello-single-process-prepare test-hello-single-process-run test-hello-single-process test-regrtest-single-process-prepare test-regrtest-single-process-run test-regrtest-single-process test
