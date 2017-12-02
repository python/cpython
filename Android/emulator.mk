# Android/emulator.mk

avd_name := $(BUILD_TYPE)
native_python_exe := $(native_build_dir)/python
python_cmd := $(native_python_exe) -B

# Update this variable to the current Python version in development after a
# major Python release. This is used to differentiate the emulators when a
# buildbot is running both the 3.x and the maintenance versions.
python_devpt_version := 3.7

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
emulator: emulator_checks _emulator adb_shell

emulator_install: wipe_data := -wipe-data
emulator_install: emulator_checks dist _emulator
	@echo "---> Install Python on the avd and start the emulator."
	$(python_cmd) $(py_srcdir)/Android/tools/install.py

adb_shell:
	@echo "---> Run an adb shell."
	$(python_cmd) $(py_srcdir)/Android/tools/python_shell.py
	@$(ADB) -s emulator-$(CONSOLE_PORT) shell

install: emulator_install
	@echo "---> Run an adb shell."
	$(python_cmd) $(py_srcdir)/Android/tools/python_shell.py
	@$(ADB) -s emulator-$(CONSOLE_PORT) shell

ifneq ($(PYTHON_ARGS), )
python: emulator_install
	@echo "---> Run <python $(PYTHON_ARGS)>."
	-$(python_cmd) $(py_srcdir)/Android/tools/python_shell.py "$(PYTHON_ARGS)"
	$(ROOT_MAKE) kill_emulator
endif

buildbottest: TESTRUNNER := $(python_cmd) $(py_srcdir)/Android/tools/python_shell.py
buildbottest: TESTRUNNER += $(TESTPYTHONOPTS) $(SYS_EXEC_PREFIX)/bin/run_tests.py
buildbottest: export PY_SRCDIR := $(py_srcdir)
buildbottest: export PATH := $(native_build_dir):$(PATH)
buildbottest: emulator_install
	@echo "---> Run buildbottest."
	-$(MAKE) -C $(py_host_dir) TESTRUNNER="$(TESTRUNNER)" buildbottest
	$(ROOT_MAKE) kill_emulator

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

kill_emulator:
	@echo "---> Kill the emulator."
	$(python_cmd) $(py_srcdir)/Android/tools/kill_emulator.py $(CONSOLE_PORT)

emulator_checks:
	@echo "---> Check that an emulator is not currently running."
	@if test -f "$(native_python_exe)"; then \
	    cd $(py_srcdir)/Android/tools; \
	        $(python_cmd) -c "import sys, android_utils; \
                    sys.exit(android_utils.emulator_listens($(CONSOLE_PORT)))"; \
	fi

_emulator: $(avd_dir)/$(avd_name)
	@echo "---> Start the emulator."
	$(ANDROID_SDK_ROOT)/emulator/emulator -avd $(avd_name) \
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

.PHONY: emulator adb_shell install python buildbottest gdb \
        avdclean _emulator kill_emulator emulator_checks emulator_install
