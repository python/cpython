====================
Python on iOS README
====================

:Authors:
    Russell Keith-Magee (2023-11)

This document provides a quick overview of some iOS specific features in the
Python distribution.

These instructions are only needed if you're planning to compile Python for iOS
yourself. Most users should *not* need to do this. If you're looking to
experiment with writing an iOS app in Python on iOS, tools such as `BeeWare's
Briefcase <https://briefcase.readthedocs.io>`__ and `Kivy's Builddozer
<https://buildozer.readthedocs.io>`__ will provide a much more approachable user
experience.

Compilers for building on iOS
=============================

Building for iOS requires the use of Apple's Xcode tooling. It is strongly
recommended that you use the most recent stable release of Xcode. This will
require the use of the most (or second-most) recently released macOS version,
as Apple does not maintain Xcode for older macOS versions.

iOS specific arguments to configure
===================================

* ``--enable-framework[=DIR]``

  This argument specifies the location where the Python.framework will
  be installed.

* ``--with-framework-name=NAME``

  Specify the name for the python framework; defaults to ``Python``.

Building Python on iOS
======================

ABIs and Architectures
----------------------

iOS apps can be deployed on physical devices, and on the iOS simulator. Although
the API used on these devices is identical, the ABI is different - you need to
link against different libraries for an iOS device build (``iphoneos``) or an
iOS simulator build (``iphonesimulator``).

Apple uses the ``XCframework`` format to allow specifying a single dependency
that supports multiple ABIs. An ``XCframework`` is a wrapper around multiple
ABI-specific frameworks that share a common API.

iOS can also support different CPU architectures within each ABI. At present,
there is only a single supported architecture on physical devices - ARM64.
However, the *simulator* supports 2 architectures - ARM64 (for running on Apple
Silicon machines), and x86_64 (for running on older Intel-based machines).

To support multiple CPU architectures on a single platform, Apple uses a "fat
binary" format - a single physical file that contains support for multiple
architectures. It is possible to compile and use a "thin" single architecture
version of a binary for testing purposes; however, the "thin" binary will not be
portable to machines using other architectures.

Building a single-architecture framework
----------------------------------------

The Python build system will create a ``Python.framework`` that supports a
*single* ABI with a *single* architecture. Unlike macOS, iOS does not allow a
framework to contain non-library content, so the iOS build will produce a
``bin`` and ``lib`` folder in the same output folder as ``Python.framework``.
The ``lib`` folder will be needed at runtime to support the Python library.

If you want to use Python in a real iOS project, you need to produce multiple
``Python.framework`` builds, one for each ABI and architecture. iOS builds of
Python *must* be constructed as framework builds. To support this, you must
provide the ``--enable-framework`` flag when configuring the build. The build
also requires the use of cross-compilation. The minimal commands for building
Python for the ARM64 iOS simulator will look something like::

  $ export PATH="`pwd`/iOS/Resources/bin:/usr/bin:/bin:/usr/sbin:/sbin:/Library/Apple/usr/bin"
  $ ./configure \
        AR=arm64-apple-ios-simulator-ar \
        CC=arm64-apple-ios-simulator-clang \
        CPP=arm64-apple-ios-simulator-cpp \
        CXX=arm64-apple-ios-simulator-clang \
        --enable-framework=/path/to/install \
        --host=aarch64-apple-ios-simulator \
        --build=aarch64-apple-darwin \
        --with-build-python=/path/to/python.exe
  $ make
  $ make install

In this invocation:

* ``iOS/Resources/bin`` has been added to the path, providing some shims for the
  compilers and linkers needed by the build. Xcode requires the use of ``xcrun``
  to invoke compiler tooling; however, ``xcrun`` embeds user- and
  version-specific paths into the sysconfig data, which limits the portability
  of the compiled Python. It also requires that compiler variables like ``CC``
  include spaces, which can cause significant problems with many C configuration
  systems which assume that ``CC`` will be a single executable. The
  ``iOS/Resources/bin`` folder contains some wrapper scripts that present as
  simple compilers and linkers, but wrap underlying calls to ``xcrun``.

  The path has also been cleared of any user customizations. A common source of
  bugs is for tools like Homebrew to accidentally leak macOS binaries into an iOS
  build. Resetting the path to a known "bare bones" value is the easiest way to
  avoid these problems.

* ``/path/to/install`` is the location where the final ``Python.framework`` will
  be output.

* ``--host`` is the architecture and ABI that you want to build, in GNU compiler
  triple format. This will be one of:

  - ``aarch64-apple-ios`` for ARM64 iOS devices.
  - ``aarch64-apple-ios-simulator`` for the iOS simulator running on Apple
    Silicon devices.
  - ``x86_64-apple-ios-simulator`` for the iOS simulator running on Intel
    devices.

* ``--build`` is the GNU compiler triple for the machine that will be running
  the compiler. This is one of:

  - ``aarch64-apple-darwin`` for Apple Silicon devices.
  - ``x86_64-apple-darwin`` for Intel devices.

* ``/path/to/python.exe`` is the path to a Python binary on the machine that
  will be running the compiler. This is needed because the Python compilation
  process involves running some Python code. On a normal desktop build of
  Python, you can compile a python interpreter and then use that interpreter to
  run Python code. However, the binaries produced for iOS won't run on macOS, so
  you need to provide an external Python interpreter. This interpreter must be
  the version as the Python that is being compiled.

In practice, you will likely also need to specify the paths to iOS builds of the
binary libraries that CPython depends on (XZ, BZip2, LibFFI and OpenSSL).

Merge thin frameworks into fat frameworks
-----------------------------------------

Once you've built a ``Python.framework`` for each ABI and and architecture, you
must produce a "fat" framework for each ABI that contains all the architectures
for that ABI.

The ``iphoneos`` build only needs to support a single architecture, so it can be
used without modification.

If you only want to support a single simulator architecture, (e.g., only support
ARM64 simulators), you can use a single architecture ``Python.framework`` build.
However, if you want to create ``Python.xcframework`` that supports *all*
architectures, you'll need to merge the ``iphonesimulator`` builds for ARM64 and
x86_64 into a single "fat" framework.

The "fat" framework can be constructed by performing a directory merge of the
content of the two "thin" ``Python.framework`` directories, plus the ``bin`` and
``lib`` folders for each thin framework. When performing this merge:

* The pure Python standard library content is identical for each architecture,
  except for a handful of platform-specific files (such as the ``sysconfig``
  module). Ensure that the "fat" framework has the union of all standard library
  files.

* Any binary files in the standard library, plus the main
  ``libPython3.X.dylib``, can be merged using the ``lipo`` tool, provide by
  Xcode::

    $ lipo -create -output module.dylib path/to/x86_64/module.dylib path/to/arm64/module.dylib

* The header files will be indentical on both architectures, except for
  ``pyconfig.h``. Copy all the headers from one platform (say, arm64), rename
  ``pyconfig.h`` to ``pyconfig-arm64.h``, and copy the ``pyconfig.h`` for the
  other architecture into the merged header folder as ``pyconfig-x86_64.h``.
  Then copy the ``iOS/Resources/pyconfig.h`` file from the CPython sources into
  the merged headers folder. This will allow the two Python architectures to
  share a common ``pyconfig.h`` header file.

At this point, you should have 2 Python.framework folders - one for ``iphoneos``,
and one for ``iphonesimulator`` that is a merge of x86+64 and ARM64 content.

Merge frameworks into an XCframework
------------------------------------

Now that we have 2 (potentially fat) ABI-specific frameworks, we can merge those
frameworks into a single ``XCframework``.

The initial skeleton of an ``XCframework`` is built using::

    xcodebuild -create-xcframework -output Python.xcframework -framework path/to/iphoneos/Python.framework -framework path/to/iphonesimulator/Python.framework

Then, copy the ``bin`` and ``lib`` folders into the architecture-specific slices of
the XCframework::

    cp path/to/iphoneos/bin Python.xcframework/ios-arm64
    cp path/to/iphoneos/lib Python.xcframework/ios-arm64

    cp path/to/iphonesimulator/bin Python.xcframework/ios-arm64_x86-64-simulator
    cp path/to/iphonesimulator/lib Python.xcframework/ios-arm64_x86-64-simulator

Note that the name of the architecture-specific slice for the simulator will
depend on the CPU architecture that you build.

Then, add symbolic links to "common" platform names for each slice::

    ln -si ios-arm64 Python.xcframework/iphoneos
    ln -si ios-arm64_x86-64-simulator Python.xcframework/iphonesimulator

You now have a Python.xcframework that can be used in a project.

Using Python on iOS
===================

To add Python to an iOS Xcode project:

1. Build a Python ``XCFramework`` using the instructions above. At a minimum,
   you will need a build for `arm64-apple-ios`, plus one of either
   `arm64-apple-ios-simulator` or `x86_64-apple-ios-simulator`.

2. Drag the ``XCframework`` into your iOS project. In the following
   instructions, we'll assume you've dropped the ``XCframework`` into the root
   of your project; however, you can use any other location that you want.

3. Drag the ``iOS/Resources/dylib-Info-template.plist`` file into your project,
   and ensure it is associated with the app target.

4. Select the app target by selecting the root node of your Xcode project, then
   the target name in the sidebar that appears.

5. In the "General" settings, under "Frameworks, Libraries and Embedded
   Content", Add ``Python.xcframework``, with "Embed & Sign" selected.

6. In the "Build Settings" tab, modify the following:

   - Build Options
     * User script sandboxing: No
     * Enable Testability: Yes
   - Search Paths
     * Framework Search Paths: ``$(PROJECT_DIR)``
     * Header Search Paths: ``"$(BUILT_PRODUCTS_DIR)/Python.framework/Headers"``
   - Apple Clang - Warnings - All languages
     * Quoted Include in Framework Header: No

7. In the "Build Phases" tab, add a new "Run Script" build step *before* the
   "Embed Frameworks" step. Name the step "Install Target Specific Python
   Standard Library", disable the "Based on dependency analysis" checkbox, and
   set the script content to::

    set -e

    mkdir -p "$CODESIGNING_FOLDER_PATH/python/lib"
    if [ "$EFFECTIVE_PLATFORM_NAME" = "-iphonesimulator" ]; then
        echo "Installing Python modules for iOS Simulator"
        rsync -au --delete "$PROJECT_DIR/Python.xcframework/iphonesimulator/lib/" "$CODESIGNING_FOLDER_PATH/python/lib/"
    else
        echo "Installing Python modules for iOS Device"
        rsync -au --delete "$PROJECT_DIR/Python.xcframework/iphoneos/lib/" "$CODESIGNING_FOLDER_PATH/python/lib/"
    fi

8. Add a second "Run Script" build step *directly after* the step you just
   added, named "Prepare Python Binary Modules". It should also have "Based on
   dependency analysis" unchecked, with the following script content::

    set -e

    install_dylib () {
        INSTALL_BASE=$1
        FULL_DYLIB=$2

        # The name of the .dylib file
        DYLIB=$(basename "$FULL_DYLIB")
        # The name of the .dylib file, relative to the install base
        RELATIVE_DYLIB=${FULL_DYLIB#$CODESIGNING_FOLDER_PATH/$INSTALL_BASE/}
        # The full dotted name of the binary module, constructed from the file path.
        FULL_MODULE_NAME=$(echo $RELATIVE_DYLIB | cut -d "." -f 1 | tr "/" ".");
        # A bundle identifier; not actually used, but required by Xcode framework packaging
        FRAMEWORK_BUNDLE_ID=$(echo $PRODUCT_BUNDLE_IDENTIFIER.$FULL_MODULE_NAME | tr "_" "-")
        # The name of the framework folder.
        FRAMEWORK_FOLDER="Frameworks/$FULL_MODULE_NAME.framework"

        # If the framework folder doesn't exist, create it.
        if [ ! -d "$CODESIGNING_FOLDER_PATH/$FRAMEWORK_FOLDER" ]; then
            echo "Creating framework for $RELATIVE_DYLIB"
            mkdir -p "$CODESIGNING_FOLDER_PATH/$FRAMEWORK_FOLDER"

            cp "$CODESIGNING_FOLDER_PATH/dylib-Info-template.plist" "$CODESIGNING_FOLDER_PATH/$FRAMEWORK_FOLDER/Info.plist"
            plutil -replace CFBundleExecutable -string "$DYLIB" "$CODESIGNING_FOLDER_PATH/$FRAMEWORK_FOLDER/Info.plist"
            plutil -replace CFBundleIdentifier -string "$FRAMEWORK_BUNDLE_ID" "$CODESIGNING_FOLDER_PATH/$FRAMEWORK_FOLDER/Info.plist"
        fi

        echo "Installing binary for $RELATIVE_DYLIB"
        mv "$FULL_DYLIB" "$CODESIGNING_FOLDER_PATH/$FRAMEWORK_FOLDER"
    }

    PYTHON_VER=$(ls "$CODESIGNING_FOLDER_PATH/python/lib")
    echo "Install Python $PYTHON_VER standard library dylibs..."
    find "$CODESIGNING_FOLDER_PATH/python/lib/$PYTHON_VER/lib-dynload" -name "*.dylib" | while read FULL_DYLIB; do
        install_dylib python/lib/$PYTHON_VER/lib-dynload "$FULL_DYLIB"
    done

    # Clean up dylib template
    rm -f "$CODESIGNING_FOLDER_PATH/dylib-Info-template.plist"

    echo "Signing frameworks as $EXPANDED_CODE_SIGN_IDENTITY_NAME ($EXPANDED_CODE_SIGN_IDENTITY)..."
    find "$CODESIGNING_FOLDER_PATH/Frameworks" -name "*.framework" -exec /usr/bin/codesign --force --sign "$EXPANDED_CODE_SIGN_IDENTITY" ${OTHER_CODE_SIGN_FLAGS:-} -o runtime --timestamp=none --preserve-metadata=identifier,entitlements,flags --generate-entitlement-der "{}" \;

9. Add Objective C code to initialize and use a Python interpreter in embedded
   mode. When configuring the interpreter, you can use:

      [NSString stringWithFormat:@"%@/python", [[NSBundle mainBundle] resourcePath], nil]

   as the value of ``PYTHONHOME``; the standard library will be installed as the
   ``lib/python3.X`` subfolder of that ``PYTHONHOME``.

If you have third-party binary modules in your app, they will need to be:

* Compiled for both on-device and simulator platforms;
* Copied into your project as part of the script in step 9;
* Installed and signed as part of the script in step 10.

Testing Python on iOS
=====================

The ``Tools/iOSTestbed`` folder that contains an Xcode project that is able to run
the iOS test suite. This project converts the Python test suite into a single
test case in Xcode's XCTest framework. The single XCTest passes if the test
suite passes.

To run the test suite, configure a Python build for an iOS simulator (i.e.,
``--host=aarch64-apple-ios-simulator`` or ``--host=x86_64-apple-ios-simulator``
), setting the framework location to the testbed project::

    --enable-framework="./Tools/iOSTestbed/Python.xcframework/ios-arm64_x86_64-simulator"

Then run ``make all install testiOS``. This will build an iOS framework for your
chosen architecture, install the Python iOS framework into the testbed project,
and run the test suite on an "iPhone SE (3rd generation)" simulator.

While the test suite is running, Xcode does not display any console output.
After showing some Xcode build commands, the console output will print ``Testing
started``, and then appear to stop. It will remain in this state until the test
suite completes. On a 2022 M1 MacBook Pro, the test suite takes approximately 12
minutes to run; a couple of extra minutes is required to boot and prepare the
iOS simulator.

On success, the test suite will exit and report successful completion of the
test suite. No output of the Python test suite will be displayed.

On failure, the output of the Python test suite *will* be displayed. This will
show the details of the tests that failed.

Debuging test failures
----------------------

The easiest way to diagnose a single test failure is to open the testbed project
in Xcode and run the tests from there using the "Product > Test" menu item.

Running specific tests
^^^^^^^^^^^^^^^^^^^^^^

As the test suite is being executed on an iOS simulator, it is not possible to
pass in command line arguments to configure test suite operation. To work around
this limitation, the arguments that would normally be passed as command line
arguments are configured as a static string at the start of the XCTest method
``- (void)testPython`` in ``iOSTestbedTests.m``. To pass an argument to the test
suite, add a a string to the ``argv`` defintion. These arguments will be passed
to the test suite as if they had been passed to ``python -m test`` at the
command line.

Disabling automated breakpoints
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

By default, Xcode will inserts an automatic breakpoint whenever a signal is
raised. The Python test suite raises many of these signals as part of normal
operation; unless you are trying to diagnose an issue with signals, the
automatic breakpoints can be inconvenient. However, they can be disabled by
creating a symbolic breakpoint that is triggered at the start of the test run.

Select "Debug > Breakpoints > Create Symbolic Breakpoint" from the Xcode menu, and
populate the new brewpoint with the following details:

* **Name**: IgnoreSignals
* **Symbol**: UIApplicationMain
* **Action**: Add debugger commands for:
  - ``process handle SIGINT -n true -p true -s false``
  - ``process handle SIGUSR1 -n true -p true -s false``
  - ``process handle SIGUSR2 -n true -p true -s false``
  - ``process handle SIGXFSZ -n true -p true -s false``
* Check the "Automatically continue after evaluating" box.

All other details can be left blank. When the process executes the
``UIApplicationMain`` entry point, the breakpoint will trigger, run the debugger
commands to disable the automatic breakpoints, and automatically resume.
