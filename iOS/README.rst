====================
Python on iOS README
====================

:Authors:
    Russell Keith-Magee (2023-11)

This document provides a quick overview of some iOS specific features in the
Python distribution.

These instructions are only needed if you're planning to compile Python for iOS
yourself. Most users should *not* need to do this. If you're looking to
experiment with writing an iOS app in Python, tools such as `BeeWare's Briefcase
<https://briefcase.readthedocs.io>`__ and `Kivy's Buildozer
<https://buildozer.readthedocs.io>`__ will provide a much more approachable
user experience.

Compilers for building on iOS
=============================

Building for iOS requires the use of Apple's Xcode tooling. It is strongly
recommended that you use the most recent stable release of Xcode. This will
require the use of the most (or second-most) recently released macOS version,
as Apple does not maintain Xcode for older macOS versions. The Xcode Command
Line Tools are not sufficient for iOS development; you need a *full* Xcode
install.

If you want to run your code on the iOS simulator, you'll also need to install
an iOS Simulator Platform. You should be prompted to select an iOS Simulator
Platform when you first run Xcode. Alternatively, you can add an iOS Simulator
Platform by selecting an open the Platforms tab of the Xcode Settings panel.

iOS specific arguments to configure
===================================

* ``--enable-framework[=DIR]``

  This argument specifies the location where the Python.framework will be
  installed. If ``DIR`` is not specified, the framework will be installed into
  a subdirectory of the ``iOS/Frameworks`` folder.

  This argument *must* be provided when configuring iOS builds. iOS does not
  support non-framework builds.

* ``--with-framework-name=NAME``

  Specify the name for the Python framework; defaults to ``Python``.

  .. admonition:: Use this option with care!

    Unless you know what you're doing, changing the name of the Python
    framework on iOS is not advised. If you use this option, you won't be able
    to run the ``make testios`` target without making significant manual
    alterations, and you won't be able to use any binary packages unless you
    compile them yourself using your own framework name.

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

  $ export PATH="$(pwd)/iOS/Resources/bin:/usr/bin:/bin:/usr/sbin:/sbin:/Library/Apple/usr/bin"
  $ ./configure \
        --enable-framework \
        --host=arm64-apple-ios-simulator \
        --build=arm64-apple-darwin \
        --with-build-python=/path/to/python.exe
  $ make
  $ make install

In this invocation:

* ``iOS/Resources/bin`` has been added to the path, providing some shims for the
  compilers and linkers needed by the build. Xcode requires the use of ``xcrun``
  to invoke compiler tooling. However, if ``xcrun`` is pre-evaluated and the
  result passed to ``configure``, these results can embed user- and
  version-specific paths into the sysconfig data, which limits the portability
  of the compiled Python. Alternatively, if ``xcrun`` is used *as* the compiler,
  it requires that compiler variables like ``CC`` include spaces, which can
  cause significant problems with many C configuration systems which assume that
  ``CC`` will be a single executable.

  To work around this problem, the ``iOS/Resources/bin`` folder contains some
  wrapper scripts that present as simple compilers and linkers, but wrap
  underlying calls to ``xcrun``. This allows configure to use a ``CC``
  definition without spaces, and without user- or version-specific paths, while
  retaining the ability to adapt to the local Xcode install. These scripts are
  included in the ``bin`` directory of an iOS install.

  These scripts will, by default, use the currently active Xcode installation.
  If you want to use a different Xcode installation, you can use
  ``xcode-select`` to set a new default Xcode globally, or you can use the
  ``DEVELOPER_DIR`` environment variable to specify an Xcode install. The
  scripts will use the default ``iphoneos``/``iphonesimulator`` SDK version for
  the select Xcode install; if you want to use a different SDK, you can set the
  ``IOS_SDK_VERSION`` environment variable. (e.g, setting
  ``IOS_SDK_VERSION=17.1`` would cause the scripts to use the ``iphoneos17.1``
  and ``iphonesimulator17.1`` SDKs, regardless of the Xcode default.)

  The path has also been cleared of any user customizations. A common source of
  bugs is for tools like Homebrew to accidentally leak macOS binaries into an iOS
  build. Resetting the path to a known "bare bones" value is the easiest way to
  avoid these problems.

* ``--host`` is the architecture and ABI that you want to build, in GNU compiler
  triple format. This will be one of:

  - ``arm64-apple-ios`` for ARM64 iOS devices.
  - ``arm64-apple-ios-simulator`` for the iOS simulator running on Apple
    Silicon devices.
  - ``x86_64-apple-ios-simulator`` for the iOS simulator running on Intel
    devices.

* ``--build`` is the GNU compiler triple for the machine that will be running
  the compiler. This is one of:

  - ``arm64-apple-darwin`` for Apple Silicon devices.
  - ``x86_64-apple-darwin`` for Intel devices.

* ``/path/to/python.exe`` is the path to a Python binary on the machine that
  will be running the compiler. This is needed because the Python compilation
  process involves running some Python code. On a normal desktop build of
  Python, you can compile a python interpreter and then use that interpreter to
  run Python code. However, the binaries produced for iOS won't run on macOS, so
  you need to provide an external Python interpreter. This interpreter must be
  the same version as the Python that is being compiled. To be completely safe,
  this should be the *exact* same commit hash. However, the longer a Python
  release has been stable, the more likely it is that this constraint can be
  relaxed - the same micro version will often be sufficient.

* The ``install`` target for iOS builds is slightly different to other
  platforms. On most platforms, ``make install`` will install the build into
  the final runtime location. This won't be the case for iOS, as the final
  runtime location will be on a physical device.

  However, you still need to run the ``install`` target for iOS builds, as it
  performs some final framework assembly steps. The location specified with
  ``--enable-framework`` will be the location where ``make install`` will
  assemble the complete iOS framework. This completed framework can then
  be copied and relocated as required.

For a full CPython build, you also need to specify the paths to iOS builds of
the binary libraries that CPython depends on (XZ, BZip2, LibFFI and OpenSSL).
This can be done by defining the ``LIBLZMA_CFLAGS``, ``LIBLZMA_LIBS``,
``BZIP2_CFLAGS``, ``BZIP2_LIBS``, ``LIBFFI_CFLAGS``, and ``LIBFFI_LIBS``
environment variables, and the ``--with-openssl`` configure option. Versions of
these libraries pre-compiled for iOS can be found in `this repository
<https://github.com/beeware/cpython-apple-source-deps/releases>`__. LibFFI is
especially important, as many parts of the standard library (including the
``platform``, ``sysconfig`` and ``webbrowser`` modules) require the use of the
``ctypes`` module at runtime.

By default, Python will be compiled with an iOS deployment target (i.e., the
minimum supported iOS version) of 13.0. To specify a different deployment
target, provide the version number as part of the ``--host`` argument - for
example, ``--host=arm64-apple-ios15.4-simulator`` would compile an ARM64
simulator build with a deployment target of 15.4.

Merge thin frameworks into fat frameworks
-----------------------------------------

Once you've built a ``Python.framework`` for each ABI and architecture, you
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

* The header files will be identical on both architectures, except for
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

    cp path/to/iphonesimulator/bin Python.xcframework/ios-arm64_x86_64-simulator
    cp path/to/iphonesimulator/lib Python.xcframework/ios-arm64_x86_64-simulator

Note that the name of the architecture-specific slice for the simulator will
depend on the CPU architecture(s) that you build.

You now have a Python.xcframework that can be used in a project.

Testing Python on iOS
=====================

The ``iOS/testbed`` folder that contains an Xcode project that is able to run
the iOS test suite. This project converts the Python test suite into a single
test case in Xcode's XCTest framework. The single XCTest passes if the test
suite passes.

To run the test suite, configure a Python build for an iOS simulator (i.e.,
``--host=arm64-apple-ios-simulator`` or ``--host=x86_64-apple-ios-simulator``
), specifying a framework build (i.e. ``--enable-framework``). Ensure that your
``PATH`` has been configured to include the ``iOS/Resources/bin`` folder and
exclude any non-iOS tools, then run::

    $ make all
    $ make install
    $ make testios

This will:

* Build an iOS framework for your chosen architecture;
* Finalize the single-platform framework;
* Make a clean copy of the testbed project;
* Install the Python iOS framework into the copy of the testbed project; and
* Run the test suite on an "iPhone SE (3rd generation)" simulator.

On success, the test suite will exit and report successful completion of the
test suite. On a 2022 M1 MacBook Pro, the test suite takes approximately 15
minutes to run; a couple of extra minutes is required to compile the testbed
project, and then boot and prepare the iOS simulator.

Debugging test failures
-----------------------

Running ``make test`` generates a standalone version of the ``iOS/testbed``
project, and runs the full test suite. It does this using ``iOS/testbed``
itself - the folder is an executable module that can be used to create and run
a clone of the testbed project.

You can generate your own standalone testbed instance by running::

    $ python iOS/testbed clone --framework iOS/Frameworks/arm64-iphonesimulator my-testbed

This invocation assumes that ``iOS/Frameworks/arm64-iphonesimulator`` is the
path to the iOS simulator framework for your platform (ARM64 in this case);
``my-testbed`` is the name of the folder for the new testbed clone.

You can then use the ``my-testbed`` folder to run the Python test suite,
passing in any command line arguments you may require. For example, if you're
trying to diagnose a failure in the ``os`` module, you might run::

    $ python my-testbed run -- test -W test_os

This is the equivalent of running ``python -m test -W test_os`` on a desktop
Python build. Any arguments after the ``--`` will be passed to testbed as if
they were arguments to ``python -m`` on a desktop machine.

You can also open the testbed project in Xcode by running::

    $ open my-testbed/iOSTestbed.xcodeproj

This will allow you to use the full Xcode suite of tools for debugging.

Testing on an iOS device
^^^^^^^^^^^^^^^^^^^^^^^^

To test on an iOS device, the app needs to be signed with known developer
credentials. To obtain these credentials, you must have an iOS Developer
account, and your Xcode install will need to be logged into your account (see
the Accounts tab of the Preferences dialog).

Once the project is open, and you're signed into your Apple Developer account,
select the root node of the project tree (labeled "iOSTestbed"), then the
"Signing & Capabilities" tab in the details page. Select a development team
(this will likely be your own name), and plug in a physical device to your
macOS machine with a USB cable. You should then be able to select your physical
device from the list of targets in the pulldown in the Xcode titlebar.

Running specific tests
^^^^^^^^^^^^^^^^^^^^^^

As the test suite is being executed on an iOS simulator, it is not possible to
pass in command line arguments to configure test suite operation. To work
around this limitation, the arguments that would normally be passed as command
line arguments are configured as part of the ``iOSTestbed-Info.plist`` file
that is used to configure the iOS testbed app. In this file, the ``TestArgs``
key is an array containing the arguments that would be passed to ``python -m``
on the command line (including ``test`` in position 0, the name of the test
module to be executed).

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
