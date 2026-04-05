# test-hyperlight.mk - Hyperlight machine type test overrides
#
# Copyright(c) The Maintainers of Nanvix.
# Licensed under the MIT License.
#
# Hyperlight currently uses the default test settings.
# Override NANVIXD_EXTRA_ARGS here when Hyperlight requires
# platform-specific nanvixd flags.

NANVIXD_EXTRA_ARGS ?=

.PHONY:
