# package-common.mk - Release packaging
#
# Copyright(c) The Maintainers of Nanvix.
# Licensed under the MIT License.
#
# Builds release tarballs (sysroot + buildroot) using the same ramfs trimming
# strategy as the test pipeline.  Binaries are NOT included in the ramfs image.

# Package staging directory
RELEASE_STAGING = $(CURDIR)/.nanvix/release

# Artifact base name for release tarballs
ARTIFACT_BASE ?= cpython-$(PLATFORM)-$(PROCESS_MODE)-$(MEMORY_SIZE)

# Package staging wrapper for ramfs operations
RAMFS_STAGING_PKG = $(RELEASE_STAGING)/sysroot-pkg-wrap
RAMFS_IMG_PKG = $(RELEASE_STAGING)/cpython-ramfs.img

# Package target: install into staging, split into sysroot and buildroot tarballs
package: build
	@echo "Packaging CPython release..."
	@rm -rf $(RELEASE_STAGING)
ifdef CONFIG_NANVIX_DOCKER
	$(DOCKER_RUN) make install DESTDIR="$(DOCKER_WORKSPACE_PATH)/.nanvix/release"
else
	make install DESTDIR="$(RELEASE_STAGING)"
endif
	@test -d "$(RELEASE_STAGING)/sysroot" || { echo "Error: install did not produce $(RELEASE_STAGING)/sysroot"; exit 1; }
	@# ---------- buildroot: build dependencies ----------
	@mkdir -p $(RELEASE_STAGING)/buildroot-pkg/lib $(RELEASE_STAGING)/buildroot-pkg/bin
	@test -d "$(RELEASE_STAGING)/sysroot/include" && \
		cp -a "$(RELEASE_STAGING)/sysroot/include" "$(RELEASE_STAGING)/buildroot-pkg/include" || true
	@find "$(RELEASE_STAGING)/sysroot/lib" -maxdepth 1 -name '*.a' -exec cp -a {} "$(RELEASE_STAGING)/buildroot-pkg/lib/" \; 2>/dev/null || true
	@test -d "$(RELEASE_STAGING)/sysroot/lib/pkgconfig" && \
		cp -a "$(RELEASE_STAGING)/sysroot/lib/pkgconfig" "$(RELEASE_STAGING)/buildroot-pkg/lib/pkgconfig" || true
	@test -d "$(RELEASE_STAGING)/sysroot/lib/python3.12/config-3.12" && \
		mkdir -p "$(RELEASE_STAGING)/buildroot-pkg/lib/python3.12" && \
		cp -a "$(RELEASE_STAGING)/sysroot/lib/python3.12/config-3.12" "$(RELEASE_STAGING)/buildroot-pkg/lib/python3.12/config-3.12" || true
	@for f in python3-config python3.12-config 2to3 2to3-3.12 idle3 idle3.12 pydoc3 pydoc3.12; do \
		test -f "$(RELEASE_STAGING)/sysroot/bin/$$f" && cp -a "$(RELEASE_STAGING)/sysroot/bin/$$f" "$(RELEASE_STAGING)/buildroot-pkg/bin/" || true; \
	done
	@test -d "$(RELEASE_STAGING)/sysroot/share" && \
		cp -a "$(RELEASE_STAGING)/sysroot/share" "$(RELEASE_STAGING)/buildroot-pkg/share" || true
	@# ---------- sysroot: runtime stdlib (apply same ramfs-trim as tests) ----------
	@mkdir -p $(RAMFS_STAGING_PKG)/sysroot/lib
	@test -d "$(RELEASE_STAGING)/sysroot/lib/python3.12" && \
		cp -a "$(RELEASE_STAGING)/sysroot/lib/python3.12" "$(RAMFS_STAGING_PKG)/sysroot/lib/python3.12" || true
	$(MAKE) -f Makefile.nanvix ramfs-trim RAMFS_STAGING="$(RAMFS_STAGING_PKG)" CONFIG_NANVIX=$(CONFIG_NANVIX) NANVIX_HOME=$(NANVIX_HOME)
	@# ---------- Include python.elf binary (outside ramfs) ----------
	@if [ -f "$(CURDIR)/python$(EXE)" ]; then \
		mkdir -p "$(RELEASE_STAGING)/bin"; \
		cp "$(CURDIR)/python$(EXE)" "$(RELEASE_STAGING)/bin/python.elf"; \
		if [ -x "$(TOOLCHAIN_PREFIX)/bin/i686-nanvix-strip" ]; then \
			"$(TOOLCHAIN_PREFIX)/bin/i686-nanvix-strip" --strip-all "$(RELEASE_STAGING)/bin/python.elf" 2>/dev/null || true; \
		fi; \
		echo "Included bin/python.elf ($$(du -h "$(RELEASE_STAGING)/bin/python.elf" | cut -f1))"; \
	else \
		echo "Warning: python$(EXE) not found — binary will not be included in release"; \
	fi
	@# ---------- Build cpython-ramfs.img (reuses ramfs-build) ----------
	$(MAKE) -f Makefile.nanvix ramfs-build \
		RAMFS_STAGING="$(RAMFS_STAGING_PKG)" \
		RAMFS_IMG="$(RAMFS_IMG_PKG)" \
		CONFIG_NANVIX=$(CONFIG_NANVIX) \
		NANVIX_HOME=$(NANVIX_HOME)
	@# ---------- Create release tarballs ----------
	@mkdir -p $(CURDIR)/dist
	@mv "$(RAMFS_STAGING_PKG)/sysroot" "$(RELEASE_STAGING)/sysroot-runtime"
	@mkdir -p "$(RELEASE_STAGING)/sysroot-tar"
	@cp -a "$(RELEASE_STAGING)/sysroot-runtime" "$(RELEASE_STAGING)/sysroot-tar/sysroot"
	@if [ -d "$(RELEASE_STAGING)/bin" ]; then cp -a "$(RELEASE_STAGING)/bin" "$(RELEASE_STAGING)/sysroot-tar/bin"; fi
	@if [ -f "$(RAMFS_IMG_PKG)" ]; then cp "$(RAMFS_IMG_PKG)" "$(RELEASE_STAGING)/sysroot-tar/cpython-ramfs.img"; fi
	@cd "$(RELEASE_STAGING)/sysroot-tar" && \
		SYSROOT_TAR_ARGS="sysroot"; \
		if [ -d bin ]; then SYSROOT_TAR_ARGS="$$SYSROOT_TAR_ARGS bin"; fi; \
		if [ -f cpython-ramfs.img ]; then SYSROOT_TAR_ARGS="$$SYSROOT_TAR_ARGS cpython-ramfs.img"; fi; \
		tar -cjf "$(CURDIR)/dist/$(ARTIFACT_BASE).tar.bz2" $$SYSROOT_TAR_ARGS
	@# Buildroot tarball: headers, static libs, config
	@mv "$(RELEASE_STAGING)/buildroot-pkg" "$(RELEASE_STAGING)/sysroot"
	tar -cjf "$(CURDIR)/dist/$(ARTIFACT_BASE)-buildroot.tar.bz2" -C "$(RELEASE_STAGING)" sysroot
	@rm -rf $(RELEASE_STAGING)
	@echo "Release tarballs created in dist/"
	@ls -lh $(CURDIR)/dist/$(ARTIFACT_BASE)*.tar.bz2

.PHONY: package
