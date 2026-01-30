.. _using-ios:

===================
Using Python on iOS
===================

:Authors:
    Russell Keith-Magee (2024-03)

Python on iOS is unlike Python on desktop platforms. On a desktop platform,
Python is generally installed as a system resource that can be used by any user
of that computer. Users then interact with Python by running a :program:`python`
executable and entering commands at an interactive prompt, or by running a
Python script.

On iOS, there is no concept of installing as a system resource. The only unit
of software distribution is an "app". There is also no console where you could
run a :program:`python` executable, or interact with a Python REPL.

As a result, the only way you can use Python on iOS is in embedded mode - that
is, by writing a native iOS application, and embedding a Python interpreter
using ``libPython``, and invoking Python code using the :ref:`Python embedding
API <embedding>`. The full Python interpreter, the standard library, and all
your Python code is then packaged as a standalone bundle that can be
distributed via the iOS App Store.

If you're looking to experiment for the first time with writing an iOS app in
Python, projects such as `BeeWare <https://beeware.org>`__ and `Kivy
<https://kivy.org>`__ will provide a much more approachable user experience.
These projects manage the complexities associated with getting an iOS project
running, so you only need to deal with the Python code itself.

Python at runtime on iOS
========================

iOS version compatibility
-------------------------

The minimum supported iOS version is specified at compile time, using the
:option:`--host` option to ``configure``. By default, when compiled for iOS,
Python will be compiled with a minimum supported iOS version of 13.0. To use a
different minimum iOS version, provide the version number as part of the
:option:`!--host` argument - for example,
``--host=arm64-apple-ios15.4-simulator`` would compile an ARM64 simulator build
with a deployment target of 15.4.

Platform identification
-----------------------

When executing on iOS, ``sys.platform`` will report as ``ios``. This value will
be returned on an iPhone or iPad, regardless of whether the app is running on
the simulator or a physical device.

Information about the specific runtime environment, including the iOS version,
device model, and whether the device is a simulator, can be obtained using
:func:`platform.ios_ver`. :func:`platform.system` will report ``iOS`` or
``iPadOS``, depending on the device.

:func:`os.uname` reports kernel-level details; it will report a name of
``Darwin``.

Standard library availability
-----------------------------

The Python standard library has some notable omissions and restrictions on
iOS. See the :ref:`API availability guide for iOS <mobile-availability>` for
details.

Binary extension modules
------------------------

One notable difference about iOS as a platform is that App Store distribution
imposes hard requirements on the packaging of an application. One of these
requirements governs how binary extension modules are distributed.

The iOS App Store requires that *all* binary modules in an iOS app must be
dynamic libraries, contained in a framework with appropriate metadata, stored
in the ``Frameworks`` folder of the packaged app. There can be only a single
binary per framework, and there can be no executable binary material outside
the ``Frameworks`` folder.

This conflicts with the usual Python approach for distributing binaries, which
allows a binary extension module to be loaded from any location on
``sys.path``. To ensure compliance with App Store policies, an iOS project must
post-process any Python packages, converting ``.so`` binary modules into
individual standalone frameworks with appropriate metadata and signing. For
details on how to perform this post-processing, see the guide for :ref:`adding
Python to your project <adding-ios>`.

To help Python discover binaries in their new location, the original ``.so``
file on ``sys.path`` is replaced with a ``.fwork`` file. This file is a text
file containing the location of the framework binary, relative to the app
bundle. To allow the framework to resolve back to the original location, the
framework must contain a ``.origin`` file that contains the location of the
``.fwork`` file, relative to the app bundle.

For example, consider the case of an import ``from foo.bar import _whiz``,
where ``_whiz`` is implemented with the binary module
``sources/foo/bar/_whiz.abi3.so``, with ``sources`` being the location
registered on ``sys.path``, relative to the application bundle. This module
*must* be distributed as ``Frameworks/foo.bar._whiz.framework/foo.bar._whiz``
(creating the framework name from the full import path of the module), with an
``Info.plist`` file in the ``.framework`` directory identifying the binary as a
framework. The ``foo.bar._whiz`` module would be represented in the original
location with a ``sources/foo/bar/_whiz.abi3.fwork`` marker file, containing
the path ``Frameworks/foo.bar._whiz/foo.bar._whiz``. The framework would also
contain ``Frameworks/foo.bar._whiz.framework/foo.bar._whiz.origin``, containing
the path to the ``.fwork`` file.

When running on iOS, the Python interpreter will install an
:class:`~importlib.machinery.AppleFrameworkLoader` that is able to read and
import ``.fwork`` files. Once imported, the ``__file__`` attribute of the
binary module will report as the location of the ``.fwork`` file. However, the
:class:`~importlib.machinery.ModuleSpec` for the loaded module will report the
``origin`` as the location of the binary in the framework folder.

Compiler stub binaries
----------------------

Xcode doesn't expose explicit compilers for iOS; instead, it uses an ``xcrun``
script that resolves to a full compiler path (e.g., ``xcrun --sdk iphoneos
clang`` to get the ``clang`` for an iPhone device). However, using this script
poses two problems:

* The output of ``xcrun`` includes paths that are machine specific, resulting
  in a sysconfig module that cannot be shared between users; and

* It results in ``CC``/``CPP``/``LD``/``AR`` definitions that include spaces.
  There is a lot of C ecosystem tooling that assumes that you can split a
  command line at the first space to get the path to the compiler executable;
  this isn't the case when using ``xcrun``.

To avoid these problems, Python provided stubs for these tools. These stubs are
shell script wrappers around the underingly ``xcrun`` tools, distributed in a
``bin`` folder distributed alongside the compiled iOS framework. These scripts
are relocatable, and will always resolve to the appropriate local system paths.
By including these scripts in the bin folder that accompanies a framework, the
contents of the ``sysconfig`` module becomes useful for end-users to compile
their own modules. When compiling third-party Python modules for iOS, you
should ensure these stub binaries are on your path.

Installing Python on iOS
========================

Tools for building iOS apps
---------------------------

Building for iOS requires the use of Apple's Xcode tooling. It is strongly
recommended that you use the most recent stable release of Xcode. This will
require the use of the most (or second-most) recently released macOS version,
as Apple does not maintain Xcode for older macOS versions. The Xcode Command
Line Tools are not sufficient for iOS development; you need a *full* Xcode
install.

If you want to run your code on the iOS simulator, you'll also need to install
an iOS Simulator Platform. You should be prompted to select an iOS Simulator
Platform when you first run Xcode. Alternatively, you can add an iOS Simulator
Platform by selecting from the Platforms tab of the Xcode Settings panel.

.. _adding-ios:

Adding Python to an iOS project
-------------------------------

Python can be added to any iOS project, using either Swift or Objective C. The
following examples will use Objective C; if you are using Swift, you may find a
library like `PythonKit <https://github.com/pvieito/PythonKit>`__ to be
helpful.

To add Python to an iOS Xcode project:

1. Build or obtain a Python ``XCFramework``. See the instructions in
   :source:`Apple/iOS/README.md` (in the CPython source distribution) for details on
   how to build a Python ``XCFramework``. At a minimum, you will need a build
   that supports ``arm64-apple-ios``, plus one of either
   ``arm64-apple-ios-simulator`` or ``x86_64-apple-ios-simulator``.

2. Drag the ``XCframework`` into your iOS project. In the following
   instructions, we'll assume you've dropped the ``XCframework`` into the root
   of your project; however, you can use any other location that you want by
   adjusting paths as needed.

3. Add your application code as a folder in your Xcode project. In the
   following instructions, we'll assume that your user code is in a folder
   named ``app`` in the root of your project; you can use any other location by
   adjusting paths as needed. Ensure that this folder is associated with your
   app target.

4. Select the app target by selecting the root node of your Xcode project, then
   the target name in the sidebar that appears.

5. In the "General" settings, under "Frameworks, Libraries and Embedded
   Content", add ``Python.xcframework``, with "Embed & Sign" selected.

6. In the "Build Settings" tab, modify the following:

   - Build Options

     * User Script Sandboxing: No
     * Enable Testability: Yes

   - Search Paths

     * Framework Search Paths: ``$(PROJECT_DIR)``
     * Header Search Paths: ``"$(BUILT_PRODUCTS_DIR)/Python.framework/Headers"``

   - Apple Clang - Warnings - All languages

     * Quoted Include In Framework Header: No

7. Add a build step that processes the Python standard library, and your own
   Python binary dependencies. In the "Build Phases" tab, add a new "Run
   Script" build step *before* the "Embed Frameworks" step, but *after* the
   "Copy Bundle Resources" step. Name the step "Process Python libraries",
   disable the "Based on dependency analysis" checkbox, and set the script
   content to:

   .. code-block:: bash

      set -e
      source $PROJECT_DIR/Python.xcframework/build/build_utils.sh
      install_python Python.xcframework app

   If you have placed your XCframework somewhere other than the root of your
   project, modify the path to the first argument.

8. Add Objective C code to initialize and use a Python interpreter in embedded
   mode. You should ensure that:

   * UTF-8 mode (:c:member:`PyPreConfig.utf8_mode`) is *enabled*;
   * Buffered stdio (:c:member:`PyConfig.buffered_stdio`) is *disabled*;
   * Writing bytecode (:c:member:`PyConfig.write_bytecode`) is *disabled*;
   * Signal handlers (:c:member:`PyConfig.install_signal_handlers`) are *enabled*;
   * System logging (:c:member:`PyConfig.use_system_logger`) is *enabled*
     (optional, but strongly recommended; this is enabled by default);
   * :envvar:`PYTHONHOME` for the interpreter is configured to point at the
     ``python`` subfolder of your app's bundle; and
   * The :envvar:`PYTHONPATH` for the interpreter includes:

     - the ``python/lib/python3.X`` subfolder of your app's bundle,
     - the ``python/lib/python3.X/lib-dynload`` subfolder of your app's bundle, and
     - the ``app`` subfolder of your app's bundle

   Your app's bundle location can be determined using ``[[NSBundle mainBundle]
   resourcePath]``.

Steps 7 and 8 of these instructions assume that you have a single folder of
pure Python application code, named ``app``. If you have third-party binary
modules in your app, some additional steps will be required:

* You need to ensure that any folders containing third-party binaries are
  either associated with the app target, or are explicitly copied as part of
  step 7. Step 7 should also purge any binaries that are not appropriate for
  the platform a specific build is targeting (i.e., delete any device binaries
  if you're building an app targeting the simulator).

* If you're using a separate folder for third-party packages, ensure that
  folder is added to the end of the call to ``install_python`` in step 7, and
  as part of the :envvar:`PYTHONPATH` configuration in step 8.

* If any of the folders that contain third-party packages will contain ``.pth``
  files, you should add that folder as a *site directory* (using
  :meth:`site.addsitedir`), rather than adding to :envvar:`PYTHONPATH` or
  :attr:`sys.path` directly.

Testing a Python package
------------------------

The CPython source tree contains :source:`a testbed project <Apple/iOS/testbed>` that
is used to run the CPython test suite on the iOS simulator. This testbed can also
be used as a testbed project for running your Python library's test suite on iOS.

After building or obtaining an iOS XCFramework (see :source:`Apple/iOS/README.md`
for details), create a clone of the Python iOS testbed project. If you used the
``Apple`` build script to build the XCframework, you can run:

.. code-block:: bash

    $ python cross-build/iOS/testbed clone --app <path/to/module1> --app <path/to/module2> app-testbed

Or, if you've sourced your own XCframework, by running:

.. code-block:: bash

    $ python Apple/testbed clone --platform iOS --framework <path/to/Python.xcframework> --app <path/to/module1> --app <path/to/module2> app-testbed

Any folders specified with the ``--app`` flag will be copied into the cloned
testbed project. The resulting testbed will be created in the ``app-testbed``
folder. In this example, the ``module1`` and ``module2`` would be importable
modules at runtime. If your project has additional dependencies, they can be
installed into the ``app-testbed/Testbed/app_packages`` folder (using ``pip
install --target app-testbed/Testbed/app_packages`` or similar).

You can then use the ``app-testbed`` folder to run the test suite for your app,
For example, if ``module1.tests`` was the entry point to your test suite, you
could run:

.. code-block:: bash

    $ python app-testbed run -- module1.tests

This is the equivalent of running ``python -m module1.tests`` on a desktop
Python build. Any arguments after the ``--`` will be passed to the testbed as
if they were arguments to ``python -m`` on a desktop machine.

You can also open the testbed project in Xcode by running:

.. code-block:: bash

    $ open app-testbed/iOSTestbed.xcodeproj

This will allow you to use the full Xcode suite of tools for debugging.

The arguments used to run the test suite are defined as part of the test plan.
To modify the test plan, select the test plan node of the project tree (it
should be the first child of the root node), and select the "Configurations"
tab. Modify the "Arguments Passed On Launch" value to change the testing
arguments.

The test plan also disables parallel testing, and specifies the use of the
``Testbed.lldbinit`` file for providing configuration of the debugger. The
default debugger configuration disables automatic breakpoints on the
``SIGINT``, ``SIGUSR1``, ``SIGUSR2``, and ``SIGXFSZ`` signals.

App Store Compliance
====================

The only mechanism for distributing apps to third-party iOS devices is to
submit the app to the iOS App Store; apps submitted for distribution must pass
Apple's app review process. This process includes a set of automated validation
rules that inspect the submitted application bundle for problematic code. There
are some steps that must be taken to ensure that your app will be able to pass
these validation steps.

Incompatible code in the standard library
-----------------------------------------

The Python standard library contains some code that is known to violate these
automated rules. While these violations appear to be false positives, Apple's
review rules cannot be challenged; so, it is necessary to modify the Python
standard library for an app to pass App Store review.

The Python source tree contains
:source:`a patch file <Mac/Resources/app-store-compliance.patch>` that will remove
all code that is known to cause issues with the App Store review process. This
patch is applied automatically when building for iOS.

Privacy manifests
-----------------

In April 2025, Apple introduced a requirement for `certain third-party
libraries to provide a Privacy Manifest
<https://developer.apple.com/support/third-party-SDK-requirements>`__.
As a result, if you have a binary module that uses one of the affected
libraries, you must provide an ``.xcprivacy`` file for that library.
OpenSSL is one library affected by this requirement, but there are others.

If you produce a binary module named ``mymodule.so``, and use you the Xcode
build script described in step 7 above, you can place a ``mymodule.xcprivacy``
file next to ``mymodule.so``, and the privacy manifest will be installed into
the required location when the binary module is converted into a framework.
