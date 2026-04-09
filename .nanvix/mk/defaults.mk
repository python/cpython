# defaults.mk - Shared variable defaults for all Makefile paths
#
# Copyright(c) The Maintainers of Nanvix.
# Licensed under the MIT License.
#
# Included by common.mk (native / Linux-Docker builds) and docker-host.mk
# (Windows host-side re-invocation) so that platform defaults are defined
# in exactly one place.

# Nanvix Docker image for cross-compilation
NANVIX_DOCKER_IMAGE ?= nanvix/toolchain:latest-minimal

# Platform and deployment configuration (passed by nanvix-zutil / z.py)
PLATFORM ?= microvm
PROCESS_MODE ?= standalone
MEMORY_SIZE ?= 256mb

# Install prefix baked into the python binary (sys.prefix, sys.path)
INSTALL_PREFIX ?= /sysroot

# Set to 'yes' for release packaging
NANVIX_RELEASE ?= no

# Space-separated list of stdlib test modules to run via regrtest.
NANVIX_TEST_LIST ?= test_float test_complex test_bool test_struct

# Nanvix sysroot directory (set by z.py / environment)
NANVIX_HOME ?= $(HOME)/nanvix
