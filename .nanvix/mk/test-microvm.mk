# test-microvm.mk - MicroVM machine type test overrides
#
# Copyright(c) The Maintainers of Nanvix.
# Licensed under the MIT License.
#
# MicroVM-specific test overrides.
# Override NANVIXD_EXTRA_ARGS here when MicroVM requires
# platform-specific nanvixd flags.

NANVIXD_EXTRA_ARGS ?=

.PHONY:
