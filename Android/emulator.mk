# Android/emulator.mk

avd_name := $(BUILD_TYPE)
native_python_exe := $(native_build_dir)/python
python_cmd := $(native_python_exe) -B

# This variable is used to differentiate the emulators when a buildbot is
# running both the 3.x and the maintenance versions.
python_devpt_version := 3.7
ifdef PYTHON_DEVPT_VERSION
   python_devpt_version := $(PYTHON_DEVPT_VERSION)
endif

ifeq ($(py_version), $(python_devpt_version))
    ifeq ($(ANDROID_ARCH), x86)
        export CONSOLE_PORT := 5554
    else ifeq ($(ANDROID_ARCH), x86_64)
        export CONSOLE_PORT := 5556
    else ifeq ($(ANDROID_ARCH), armv7)
        export CONSOLE_PORT := 5558
    else ifeq ($(ANDROID_ARCH), arm64)
        export CONSOLE_PORT := 5560
    endif
else
    ifeq ($(ANDROID_ARCH), x86)
        export CONSOLE_PORT := 5562
    else ifeq ($(ANDROID_ARCH), x86_64)
        export CONSOLE_PORT := 5564
    else ifeq ($(ANDROID_ARCH), armv7)
        export CONSOLE_PORT := 5566
    else ifeq ($(ANDROID_ARCH), arm64)
        export CONSOLE_PORT := 5568
    endif
endif
export ADB := $(ANDROID_SDK_ROOT)/platform-tools/adb


# Rules.
exists_python_cmd:
	@if test ! -f "$(native_python_exe)"; then \
	    echo "Error: the native python executable is missing"; \
	    exit 1; \
	fi

adb_shell: exists_python_cmd _emulator
	@echo "---> Run an adb shell."
	$(python_cmd) $(py_srcdir)/Android/tools/python_shell.py
	@$(ADB) -s emulator-$(CONSOLE_PORT) shell

# The 'all' target is used by the buildbots.
# Abort when the emulator is already running so that we install on a clean
# emulator.
all: wipe_data := -wipe-data
all: export PY_SRCDIR := $(py_srcdir)
all: dist is_emulator_listening _emulator
	@echo "---> Install Python on the avd and start the emulator."
	$(python_cmd) $(py_srcdir)/Android/tools/install.py

install: all
	@echo "---> Run an adb shell."
	$(python_cmd) $(py_srcdir)/Android/tools/python_shell.py
	@$(ADB) -s emulator-$(CONSOLE_PORT) shell

ifneq ($(PYTHON_ARGS), )
python: exists_python_cmd _emulator
	@echo "---> Run <python $(PYTHON_ARGS)>."
	$(python_cmd) $(py_srcdir)/Android/tools/python_shell.py "$(PYTHON_ARGS)"
endif

pythoninfo: exists_python_cmd _emulator
	@echo "---> Run pythoninfo."
	@ndk_version=$$(sed -n -e 's/Pkg.Revision\s*=\s*\(\S*\).*/\1/p' \
	                $(ANDROID_NDK_ROOT)/source.properties); \
	    echo "Python debug information"; \
	    echo "========================"; \
	    echo "NDK version: $$ndk_version"; echo
	$(python_cmd) $(py_srcdir)/Android/tools/python_shell.py -m test.pythoninfo

buildbottest: TESTRUNNER := $(python_cmd) $(py_srcdir)/Android/tools/python_shell.py
buildbottest: TESTRUNNER += $(TESTPYTHONOPTS) $(SYS_EXEC_PREFIX)/bin/run_tests.py
buildbottest: export PATH := $(native_build_dir):$(PATH)
buildbottest: exists_python_cmd _emulator
	@echo "---> Run buildbottest."
	-@if which pybuildbot.identify >/dev/null 2>&1; then \
	    pybuildbot.identify "CC='$(CC)'" "CXX='$(CXX)'"; \
	fi
	$(MAKE) -C $(py_host_dir) TESTRUNNER="$(TESTRUNNER)" \
	    TESTOPTS="$(TESTOPTS)" TESTTIMEOUT="$(TESTTIMEOUT)" \
	    buildbottest

gdb: export APP_ABI := $(APP_ABI)
gdb: export PY_HOST_DIR := $(py_host_dir)
gdb: export LIB_DYNLOAD = $(py_host_dir)/$(shell cat $(py_host_dir)/pybuilddir.txt)
gdb:
	@echo "---> Start gdb."
	$(py_srcdir)/Android/tools/ndk_gdb.py $(ANDROID_API) $(py_srcdir) python

$(avd_dir)/$(avd_name):
	@echo "---> Create the avd."
	mkdir -p $(avd_dir)
	echo "no" | $(ANDROID_SDK_ROOT)/tools/bin/avdmanager create avd --force --name $(avd_name) \
	    --package "system-images;android-$(ANDROID_API);default;$(APP_ABI)" \
	    --abi default/$(APP_ABI) --path $(avd_dir)/$(avd_name)

kill_emulator: exists_python_cmd
	@echo "---> Kill the emulator."
	-$(python_cmd) $(py_srcdir)/Android/tools/kill_emulator.py $(CONSOLE_PORT)

is_emulator_listening: exists_python_cmd
	@echo "---> Check that an emulator is not currently running."
	cd $(py_srcdir)/Android/tools; \
	    $(python_cmd) -c "import sys, android_utils; \
	        sys.exit(android_utils.is_emulator_listening($(CONSOLE_PORT)))"

_emulator: exists_python_cmd $(avd_dir)/$(avd_name)
	@echo "---> Start the emulator if not already running."
	$(python_cmd) $(py_srcdir)/Android/tools/start_emulator.py -avd $(avd_name) \
	    -port $(CONSOLE_PORT) $(wipe_data) $(EMULATOR_CMD_LINE_OPTIONS) &
	@ echo "---> Waiting for device to be ready."
	@$(ADB) -s emulator-$(CONSOLE_PORT) wait-for-device shell getprop init.svc.bootanim; \
	    echo "---> Device ready."

avdclean:
	@echo "---> Remove the AVD."
	if test -d "$(avd_dir)/$(avd_name)"; then \
	    $(ANDROID_SDK_ROOT)/tools/bin/avdmanager delete avd --name $(avd_name); \
	    rm -rf $(avd_dir)/$(avd_name); \
	fi
	-rmdir $(avd_dir)

.PHONY: exists_python_cmd adb_shell all install python pythoninfo buildbottest gdb \
        kill_emulator is_emulator_listening _emulator avdclean
