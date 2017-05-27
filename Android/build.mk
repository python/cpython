# Android/build.mk

# Directory names.
export BUILD_DIR := $(CURDIR)/build
export DIST_DIR := $(CURDIR)/dist
avd_dir := $(CURDIR)/avd
native_build_dir := $(BUILD_DIR)/python-native
py_version := $(shell echo $$(cat $(py_srcdir)/configure | \
    sed -e "s/^PACKAGE_VERSION=['\"]*\([0-9]*\.[0-9]*\)['\"]*$$\|^.*$$/\1/" -e "/^$$/d"))
py_name := python$(py_version)
export BUILD_TYPE := android-$(ANDROID_API)-$(ANDROID_ARCH)
py_host_dir := $(BUILD_DIR)/$(py_name)-$(BUILD_TYPE)
export PY_DESTDIR := $(BUILD_DIR)/$(py_name)-install-$(BUILD_TYPE)
abiflags = $(shell echo $$(cat $(py_host_dir)/Makefile | \
    sed -e "s/^ABIFLAGS[= \t]*\(.*\)[ \t]*$$\|^.*$$/\1/" -e "/^$$/d"))
py_fullname = python$(py_version)$(abiflags)
export STDLIB_DIR := lib/python$(py_version)


# Target variables names.
python := $(py_host_dir)/python
config_status := $(py_host_dir)/config.status
export PYTHON_ZIP := $(DIST_DIR)/$(py_name)-$(BUILD_TYPE).zip
export PY_STDLIB_ZIP := $(DIST_DIR)/$(py_name)-$(BUILD_TYPE)-stdlib.zip


# Rules.
build: $(python)
dist: $(PYTHON_ZIP) $(PY_STDLIB_ZIP)

include $(py_srcdir)/Android/emulator.mk

$(native_build_dir)/config.status: $(py_srcdir)/configure
	@echo "---> Run the native configure script."
	mkdir -p $(native_build_dir)
	cd $(native_build_dir); $(py_srcdir)/configure

native_python: $(native_build_dir)/config.status
	@echo "---> Build the native interpreter."
	$(MAKE) -C $(native_build_dir)

# Target-specific exported variables.
$(config_status):           export CPPFLAGS := -I$(PY_DESTDIR)/$(SYS_EXEC_PREFIX)/include
$(config_status):           export LDFLAGS := -L$(PY_DESTDIR)/$(SYS_EXEC_PREFIX)/lib
build configure host python_dist: \
                            export PATH := $(native_build_dir):$(PATH)
external_libraries:         export CC := $(CC)
external_libraries openssl: export AR := $(AR)
external_libraries openssl: export LD := $(LD)
external_libraries openssl: export RANLIB := $(RANLIB)
external_libraries openssl: export READELF := $(READELF)
external_libraries openssl: export CFLAGS := $(CFLAGS)
external_libraries openssl: export LDFLAGS := $(LDFLAGS)
openssl:                    export GCC := $(GCC)
openssl:                    export SYSROOT := $(SYSROOT)
openssl:                    export ANDROID_ARCH := $(ANDROID_ARCH)
openssl:                    export ANDROID_API := android-$(ANDROID_API)

external_libraries:
	@echo "---> Build the external libraries."
	mkdir -p $(BUILD_DIR)/external-libraries
	$(MAKE) -C $(py_srcdir)/Android/external-libraries

openssl:
ifneq ($(ANDROID_ARCH), x86_64)
  ifneq ($(ANDROID_ARCH), arm64)
	@echo "---> Build openssl."
	mkdir -p $(BUILD_DIR)/external-libraries
	$(MAKE) -C $(py_srcdir)/Android/external-libraries -f Makefile.openssl
  endif
endif

$(config_status): $(makefile) $(py_srcdir)/configure
	mkdir -p $(py_host_dir)
	@echo "---> Ensure that nl_langinfo is broken."
	@cd $(py_host_dir); \
	    PY_SRCDIR=$(py_srcdir) $(py_srcdir)/Android/tools/nl_langinfo.sh
	@echo "---> Run configure for $(BUILD_TYPE)."
	cd $(py_host_dir); $(py_srcdir)/configure-android \
	    --prefix=$(SYS_PREFIX) --exec-prefix=$(SYS_EXEC_PREFIX) \
	    $(config_args)

$(python): native_python external_libraries openssl $(config_status)
	$(MAKE) host

configure: native_python external_libraries openssl
	@rm -f $(config_status)
	$(MAKE) $(config_status)

host:
	@echo "---> Build Python for $(BUILD_TYPE)."
	@if test -f $(config_status); then \
	    $(MAKE) -C $(py_host_dir) all; \
	else \
	    echo "Error: please run 'make config', missing $(config_status)"; \
	    false; \
	fi

python_dist: $(python)
	@echo "---> Install Python for $(BUILD_TYPE)."
	$(MAKE) DESTDIR=$(PY_DESTDIR) -C $(py_host_dir) install

$(PYTHON_ZIP): python_dist
	@echo "---> Zip the machine-specific Python library."
	mkdir -p $(DIST_DIR)
	rm -f $(BUILD_DIR)/$(ZIPBASE_DIR)
	ln -s $(PY_DESTDIR)/$(SYS_EXEC_PREFIX) $(BUILD_DIR)/$(ZIPBASE_DIR)
	rm -f $(PYTHON_ZIP)

	cd $(BUILD_DIR)/$(ZIPBASE_DIR)/bin; \
	    ln -sf python3 python

	mkdir -p $(BUILD_DIR)/$(ZIPBASE_DIR)/etc; \
	    ln -sf $(py_srcdir)/Android/resources/inputrc $(BUILD_DIR)/$(ZIPBASE_DIR)/etc

	cd $(BUILD_DIR)/$(ZIPBASE_DIR); \
	    $(STRIP) bin/python$(py_version); \
	    chmod 755 lib/*.so*; find . -type f -name "*.so*" -print0 | xargs -0 $(STRIP)

	cd $(BUILD_DIR); \
	    zip -g $(PYTHON_ZIP) $(ZIPBASE_DIR)/bin/python3 \
	        $(ZIPBASE_DIR)/bin/python $(ZIPBASE_DIR)/bin/python$(py_version); \
	    zip -rg $(PYTHON_ZIP) \
	        $(ZIPBASE_DIR)/include/$(py_fullname)/pyconfig.h \
	        $(ZIPBASE_DIR)/$(STDLIB_DIR)/ \
	        $(ZIPBASE_DIR)/share/terminfo/l/linux \
	        $(ZIPBASE_DIR)/etc/inputrc \
	        -x \*failed.so

	# Zip the shared libraries excluding symlinks.
	cd $(BUILD_DIR); \
	    libs=""; \
	    for l in $(ZIPBASE_DIR)/lib/*.so*; do \
	        test ! -h $$l && libs="$$libs $$l"; \
	    done; \
	    zip -g $(PYTHON_ZIP) $$libs

$(PY_STDLIB_ZIP): python_dist
	@echo "---> Zip the Python library."
	mkdir -p $(DIST_DIR)
	rm -f $(PY_STDLIB_ZIP)
	cd $(PY_DESTDIR)/$(SYS_PREFIX); \
	    zip -Drg $(PY_STDLIB_ZIP) $(STDLIB_DIR)* \
	        -x \*.so \*.pyo \*opt-\*.pyc \*README

# Make things clean, before making a distribution.
distclean:
	rm -f $(PYTHON_ZIP)
	rm -f $(PY_STDLIB_ZIP)
	rm -f $(DIST_DIR)/*.sh
	-rmdir $(DIST_DIR)
	rm -f $(BUILD_DIR)/python
	rm -rf $(PY_DESTDIR)

hostclean:
	$(MAKE) -C $(py_host_dir) distclean

# Remove everything for the given ANDROID_API and ANDROID_ARCH except the avd.
clean: distclean
	rm -rf $(BUILD_DIR)/python-native
	rm -rf $(py_host_dir)
	rm -rf $(BUILD_DIR)/external-libraries/*-$(BUILD_TYPE)
	-rmdir $(BUILD_DIR)/external-libraries
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

.PHONY: build configure host native_python external_libraries openssl \
        dist python_dist distclean hostclean clean
