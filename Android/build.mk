# Android/build.mk

# Directory names.
py_version := $(shell cat $(py_srcdir)/configure | \
                sed -e "s/^PACKAGE_VERSION=['\"]*\([0-9]*\.[0-9]*\)['\"]*$$\|^.*$$/\1/" -e "/^$$/d")
export BUILD_DIR := $(MAKEFILE_DIR)build
export DIST_DIR := $(MAKEFILE_DIR)dist
avd_dir := $(MAKEFILE_DIR)avd
native_build_dir := $(BUILD_DIR)/python-native
py_name := python$(py_version)
export BUILD_TYPE := android-$(ANDROID_API)-$(ANDROID_ARCH)
py_host_dir := $(BUILD_DIR)/$(py_name)-$(BUILD_TYPE)
export PY_EXTDIR := $(BUILD_DIR)/$(py_name)-extlibs-$(BUILD_TYPE)
PY_DESTDIR := $(BUILD_DIR)/$(py_name)-install-$(BUILD_TYPE)
abiflags = $(shell echo $$(cat $(py_host_dir)/Makefile | \
    sed -e "s/^ABIFLAGS[= \t]*\(.*\)[ \t]*$$\|^.*$$/\1/" -e "/^$$/d"))
py_fullname = python$(py_version)$(abiflags)
export STDLIB_DIR := lib/python$(py_version)


ROOT_MAKE := $(MAKE) -f Makefile-$(BUILD_TYPE)

# Target variables names.
python := $(py_host_dir)/python
config_status := $(py_host_dir)/config.status
export PYTHON_ZIP := $(DIST_DIR)/$(py_name)-$(BUILD_TYPE).zip
export PY_STDLIB_ZIP := $(DIST_DIR)/$(py_name)-$(BUILD_TYPE)-stdlib.zip


# Rules.
build: $(python)
dist: $(PYTHON_ZIP) $(PY_STDLIB_ZIP)

include $(py_srcdir)/Android/emulator.mk

$(native_build_dir)/Modules/Setup: $(py_srcdir)/Modules/Setup.dist
	cp $(py_srcdir)/Modules/Setup.dist $(native_build_dir)/Modules/Setup

$(native_build_dir)/config.status: $(py_srcdir)/configure
	@# Run distclean upon the switch to a new Python release.
	@# 'Touch' the 'Makefile' target first to prevent a useless run of configure.
	@cur=$$(cat $(py_srcdir)/configure | sed -e "s/^VERSION=\([0-9]\+\.[0-9]\+\)$$\|^.*$$/\1/" -e "/^$$/d"); \
	prev=$$cur; \
	if test -f "$(native_build_dir)/python"; then \
	    prev=$$($(native_build_dir)/python -c "from sys import version_info; print('.'.join(str(i) for i in version_info[:2]))"); \
	fi; \
	if test "$$prev" != "$$cur"; then \
	    echo "---> Cleanup before the switch from Python version $$prev to $$cur"; \
	    $(MAKE) -C $(native_build_dir) -t Makefile; \
	    $(MAKE) -C $(native_build_dir) distclean; \
	fi
	@echo "---> Run the native configure script."
	mkdir -p $(native_build_dir)
	cd $(native_build_dir); $(py_srcdir)/configure

native_python: $(native_build_dir)/config.status $(native_build_dir)/Modules/Setup
	@echo "---> Build the native interpreter."
	$(MAKE) -C $(native_build_dir)

# Target-specific exported variables.
build configure host python_dist setup hostclean: \
                    export PATH := $(native_build_dir):$(PATH)
external_libraries: export CC := $(CC)
external_libraries: export AR := $(AR)
external_libraries: export LD := $(LD)
external_libraries: export RANLIB := $(RANLIB)
external_libraries: export READELF := $(READELF)
external_libraries: export CFLAGS := $(CFLAGS)
external_libraries: export LDFLAGS := $(LDFLAGS)
external_libraries: export ANDROID_ARCH := $(ANDROID_ARCH)
external_libraries:
	mkdir -p $(BUILD_DIR)/external-libraries
ifdef WITH_LIBFFI
	$(MAKE) -C $(py_srcdir)/Android/external-libraries libffi
endif
ifdef WITH_NCURSES
	$(MAKE) -C $(py_srcdir)/Android/external-libraries ncurses
endif
ifdef WITH_READLINE
	$(MAKE) -C $(py_srcdir)/Android/external-libraries readline
endif
ifdef WITH_SQLITE
	$(MAKE) -C $(py_srcdir)/Android/external-libraries sqlite
endif

openssl:
ifdef WITH_OPENSSL
	mkdir -p $(BUILD_DIR)/external-libraries
	$(MAKE) -C $(py_srcdir)/Android/external-libraries -f Makefile.openssl
endif

$(config_status): $(makefile) $(py_srcdir)/configure
	mkdir -p $(py_host_dir)
	@#echo "---> Ensure that nl_langinfo is broken."
	@#cd $(py_host_dir); PY_SRCDIR=$(py_srcdir) $(py_srcdir)/Android/tools/nl_langinfo.sh
	@echo "---> Run configure for $(BUILD_TYPE)."
	cd $(py_host_dir); \
	    PKG_CONFIG_PATH=$(PY_EXTDIR)/$(SYS_EXEC_PREFIX)/lib/pkgconfig \
	    CPPFLAGS=-I$(PY_EXTDIR)/$(SYS_EXEC_PREFIX)/include \
	    LDFLAGS=-L$(PY_EXTDIR)/$(SYS_EXEC_PREFIX)/lib \
	    $(py_srcdir)/configure-android \
	    --prefix=$(SYS_PREFIX) --exec-prefix=$(SYS_EXEC_PREFIX) \
	    $(config_args)

$(python): native_python prefixes external_libraries openssl $(config_status)
	$(ROOT_MAKE) host

configure: native_python external_libraries openssl
	@rm -f $(config_status)
	$(ROOT_MAKE) $(config_status)

prefixes:
	$(eval PREFIXES := $(shell cd $(py_srcdir)/Android/tools; $(python_cmd) \
	    -c 'from android_utils import parse_prefixes; parse_prefixes("$(config_args)")'))
	$(eval export SYS_PREFIX := $(word 1, $(PREFIXES)))
	$(eval export SYS_EXEC_PREFIX := $(word 2, $(PREFIXES)))
	@echo "---> Prefixes are $(SYS_PREFIX), $(SYS_EXEC_PREFIX)."

disabled_modules: setup_tmp := $(py_host_dir)/Modules/_Setup.tmp
disabled_modules: setup_file := $(py_host_dir)/Modules/Setup
disabled_modules:
	cp $(py_srcdir)/Modules/Setup.dist $(setup_tmp)
	echo "*disabled*" >> $(setup_tmp)
	echo "_uuid" >> $(setup_tmp)
	echo "grp" >> $(setup_tmp)
	echo "_crypt" >> $(setup_tmp)
	@if test -f $(setup_file); then \
	    if test $$(md5sum $(setup_tmp) | awk '{print $$1}') != \
	            $$(md5sum $(setup_file) | awk '{print $$1}'); then \
	        cp $(setup_tmp) $(setup_file); \
	    fi \
	fi
	rm $(setup_tmp)

host: disabled_modules
	@echo "---> Build Python for $(BUILD_TYPE)."
	@if test -f $(config_status); then \
	    $(MAKE) -C $(py_host_dir) all; \
	else \
	    echo "Error: please run 'make config', missing $(config_status)"; \
	    false; \
	fi

python_dist: $(python)
	@echo "---> Install Python for $(BUILD_TYPE)."
	$(MAKE) DESTDIR=$(PY_DESTDIR) -C $(py_host_dir) install > make_install.log
	cp --no-dereference $(PY_EXTDIR)/$(SYS_EXEC_PREFIX)/lib/*.so* $(PY_DESTDIR)/$(SYS_EXEC_PREFIX)/lib
	chmod u+w $(PY_DESTDIR)/$(SYS_EXEC_PREFIX)/lib/*.so*
	tdir=$(SYS_EXEC_PREFIX)/share/terminfo/l; mkdir -p $(PY_DESTDIR)/$$tdir && \
	    cp $(PY_EXTDIR)/$$tdir/linux $(PY_DESTDIR)/$$tdir
	if test -d "$(PY_EXTDIR)/$(SYS_EXEC_PREFIX)/etc"; then \
	    cp -r $(PY_EXTDIR)/$(SYS_EXEC_PREFIX)/etc $(PY_DESTDIR)/$(SYS_EXEC_PREFIX); \
	fi

$(PYTHON_ZIP): python_dist
	@echo "---> Zip the machine-specific Python library."
	mkdir -p $(DIST_DIR)
	rm -f $(PYTHON_ZIP)

	cd $(PY_DESTDIR)/$(SYS_EXEC_PREFIX)/bin; \
	    ln -sf python3 python

	mkdir -p $(PY_DESTDIR)/$(SYS_EXEC_PREFIX)/etc; \
	    cp $(py_srcdir)/Android/resources/inputrc $(PY_DESTDIR)/$(SYS_EXEC_PREFIX)/etc

	cd $(PY_DESTDIR)/$(SYS_EXEC_PREFIX); \
	    $(STRIP) bin/python$(py_version); \
	    chmod 755 lib/*.so*; find . -type f -name "*.so*" -print0 | xargs -0 $(STRIP)

	@# Remove the first '/' to avoid unzip warnings and an exit code of '1'.
	cd $(PY_DESTDIR); export SYS_EXEC_PREFIX=$${SYS_EXEC_PREFIX#/}; \
	    zip -g $(PYTHON_ZIP) $$SYS_EXEC_PREFIX/bin/python3 \
	        $$SYS_EXEC_PREFIX/bin/python $$SYS_EXEC_PREFIX/bin/python$(py_version); \
	    zip -rg $(PYTHON_ZIP) \
	        $$SYS_EXEC_PREFIX/include/$(py_fullname)/pyconfig.h \
	        $$SYS_EXEC_PREFIX/$(STDLIB_DIR)/ \
	        $$SYS_EXEC_PREFIX/share/terminfo/l/linux \
	        $$SYS_EXEC_PREFIX/etc/ \
	        -x \*failed.so

	# Zip the shared libraries excluding symlinks.
	cd $(PY_DESTDIR); export SYS_EXEC_PREFIX=$${SYS_EXEC_PREFIX#/}; \
	    libs=""; \
	    for l in $$SYS_EXEC_PREFIX/lib/*.so*; do \
	        test ! -h $$l && libs="$$libs $$l"; \
	    done; \
	    zip -g $(PYTHON_ZIP) $$libs

$(PY_STDLIB_ZIP): python_dist
	@echo "---> Zip the Python library."
	mkdir -p $(DIST_DIR)
	rm -f $(PY_STDLIB_ZIP)
	cd $(PY_DESTDIR); export SYS_PREFIX=$${SYS_PREFIX#/}; \
	    zip -Drg $(PY_STDLIB_ZIP) $$SYS_PREFIX/$(STDLIB_DIR)* \
	        -x \*.so \*.pyo \*opt-\*.pyc

# Build and install a third-party extension module from the setup.py directory:
#     make -f /path/to/Makefile/generated/by/makesetup setup
setup: required
	CROSS_BUILD_PYTHON_DIR=$(PY_DESTDIR)/$(SYS_EXEC_PREFIX) \
	    $(native_python_exe) setup.py install; \

# Make things clean, before making a distribution.
distclean: kill_emulator hostclean
	rm -f $(PYTHON_ZIP)
	rm -f $(PY_STDLIB_ZIP)
	rm -f $(DIST_DIR)/*.sh
	-rmdir $(DIST_DIR)
	rm -f $(BUILD_DIR)/python
	rm -rf $(PY_DESTDIR)

hostclean:
	-$(MAKE) -C $(py_host_dir) distclean

# Remove everything for the given ANDROID_API and ANDROID_ARCH except the avd.
clean: distclean
	rm -rf $(BUILD_DIR)/python-native
	rm -rf $(py_host_dir)
	rm -rf $(BUILD_DIR)/external-libraries/*-$(BUILD_TYPE)
	-rmdir $(BUILD_DIR)/external-libraries
	rm -rf $(PY_EXTDIR)
	-rmdir $(BUILD_DIR)
	-rmdir $(avd_dir)
	rm -rf $(DIST_DIR)/gdb/android-$(ANDROID_API)-$(ANDROID_ARCH)
	-rmdir $(DIST_DIR)/gdb
	-rmdir $(DIST_DIR)
	rm Makefile Makefile-android-$(ANDROID_API)-$(ANDROID_ARCH)


# Tool names.
wget := $(shell which wget 2>/dev/null)
curl := $(shell which curl 2>/dev/null)
ifeq (curl, $(findstring curl, $(curl)))
    export DOWNLOAD := $(curl) -O
else ifeq (wget, $(findstring wget, $(wget)))
    export DOWNLOAD := $(wget)
else
    $(warning *** Cannot download the external libraries with wget or curl, \
      please download those libraries to the 'Android/external-libraries' \
      directory.)
endif

.PHONY: build configure prefixes disabled_modules host native_python \
        external_libraries openssl setup \
        dist python_dist distclean hostclean clean
