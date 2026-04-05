# ramfs.mk - Shared ramfs staging logic for test and release
#
# Copyright(c) The Maintainers of Nanvix.
# Licensed under the MIT License.
#
# Provides reusable targets for building a trimmed sysroot tree suitable for
# ramfs images.  Both the test and package pipelines include this file:
# tests invoke ramfs-build (which chains ramfs-trim); packaging invokes
# ramfs-trim via recursive make and then calls ramfs-build for the image.
#
# Required variables (set by includer):
#   RAMFS_STAGING  - root directory of the staged install (contains sysroot/)
#
# Provided targets:
#   ramfs-trim     - strip dev-only artifacts from $(RAMFS_STAGING)/sysroot
#   ramfs-build    - build a ramfs image from the trimmed sysroot

ifndef _RAMFS_MK
_RAMFS_MK := 1

# Trim the staged sysroot to the minimal runtime footprint.
# Removes build-time headers, static libraries, config dirs, caches, and
# heavyweight stdlib packages not needed at runtime.
ramfs-trim:
	@if [ -z "$(RAMFS_STAGING)" ]; then echo "Error: RAMFS_STAGING is not set"; exit 1; fi
	@if [ ! -d "$(RAMFS_STAGING)/sysroot" ]; then echo "Error: $(RAMFS_STAGING)/sysroot does not exist"; exit 1; fi
	@echo "Trimming sysroot for ramfs..."
	@rm -rf $(RAMFS_STAGING)/sysroot/lib/python3.12/config-3.12
	@rm -rf $(RAMFS_STAGING)/sysroot/lib/python3.12/idlelib
	@rm -rf $(RAMFS_STAGING)/sysroot/lib/python3.12/tkinter
	@rm -rf $(RAMFS_STAGING)/sysroot/lib/python3.12/turtledemo
	@rm -rf $(RAMFS_STAGING)/sysroot/lib/python3.12/lib2to3
	@rm -rf $(RAMFS_STAGING)/sysroot/lib/python3.12/ensurepip
	@rm -rf $(RAMFS_STAGING)/sysroot/lib/python3.12/pydoc_data
	@rm -rf $(RAMFS_STAGING)/sysroot/lib/python3.12/venv
	@rm -rf $(RAMFS_STAGING)/sysroot/lib/python3.12/site-packages
	@rm -rf $(RAMFS_STAGING)/sysroot/lib/python3.12/__phello__
	@rm -rf $(RAMFS_STAGING)/sysroot/lib/python3.12/test
	@rm -f  $(RAMFS_STAGING)/sysroot/lib/libpython3.12.a
	@rm -rf $(RAMFS_STAGING)/sysroot/include
	@rm -rf $(RAMFS_STAGING)/sysroot/share
	@rm -rf $(RAMFS_STAGING)/sysroot/lib/pkgconfig
	@rm -f  $(RAMFS_STAGING)/sysroot/bin/2to3* $(RAMFS_STAGING)/sysroot/bin/idle3*
	@rm -f  $(RAMFS_STAGING)/sysroot/bin/pydoc3* $(RAMFS_STAGING)/sysroot/bin/python3-config
	@rm -f  $(RAMFS_STAGING)/sysroot/bin/python3.12-config $(RAMFS_STAGING)/sysroot/bin/python3
	@# Remove all ELF binaries — binaries are not included in ramfs
	@find $(RAMFS_STAGING)/sysroot/bin -name '*.elf' -delete 2>/dev/null || true
	@# Remove remaining non-python binaries if bin/ is now empty
	@rmdir $(RAMFS_STAGING)/sysroot/bin 2>/dev/null || true
	@find $(RAMFS_STAGING)/sysroot -name '__pycache__' -type d -exec rm -rf {} + 2>/dev/null || true

# Build a ramfs image from the trimmed sysroot.
# Output: $(RAMFS_IMG)
# Requires: mkramfs.elf in $(NANVIX_HOME)/bin/
RAMFS_IMG ?= /tmp/cpython-rootfs.img

ramfs-build: ramfs-trim
	@MKRAMFS="$(abspath $(NANVIX_HOME))/bin/mkramfs.elf"; \
	if [ ! -x "$$MKRAMFS" ]; then \
		echo "Error: mkramfs.elf not found at $$MKRAMFS"; exit 1; \
	fi; \
	"$$MKRAMFS" -o "$(RAMFS_IMG)" "$(RAMFS_STAGING)/sysroot"; \
	echo "Built ramfs image: $(RAMFS_IMG) ($$(du -h "$(RAMFS_IMG)" | cut -f1))"

.PHONY: ramfs-trim ramfs-build

endif # _RAMFS_MK
