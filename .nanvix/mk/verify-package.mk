# verify-package.mk - Release tarball verification
#
# Copyright(c) The Maintainers of Nanvix.
# Licensed under the MIT License.

# Artifact base name for release tarballs (must match package-common.mk)
ARTIFACT_BASE ?= cpython-$(PLATFORM)-$(PROCESS_MODE)-$(MEMORY_SIZE)

# Verify package target: validate release tarballs
verify-package:
	@echo "Verifying release tarballs..."
	@test -f "$(CURDIR)/dist/$(ARTIFACT_BASE).tar.bz2" || \
		{ echo "Error: sysroot tarball not found: dist/$(ARTIFACT_BASE).tar.bz2"; exit 1; }
	@test -f "$(CURDIR)/dist/$(ARTIFACT_BASE)-buildroot.tar.bz2" || \
		{ echo "Error: buildroot tarball not found: dist/$(ARTIFACT_BASE)-buildroot.tar.bz2"; exit 1; }
	@tar -tjf "$(CURDIR)/dist/$(ARTIFACT_BASE).tar.bz2" > /dev/null || \
		{ echo "Error: sysroot tarball is corrupt"; exit 1; }
	@tar -tjf "$(CURDIR)/dist/$(ARTIFACT_BASE)-buildroot.tar.bz2" > /dev/null || \
		{ echo "Error: buildroot tarball is corrupt"; exit 1; }
	@# Verify tarball contains the python.elf binary (outside ramfs, in bin/)
	@tar -tjf "$(CURDIR)/dist/$(ARTIFACT_BASE).tar.bz2" | grep -q 'bin/python.elf' || \
		{ echo "Error: sysroot tarball missing python.elf binary"; exit 1; }
	@echo "		*** Package verification PASSED ***"

.PHONY: verify-package
