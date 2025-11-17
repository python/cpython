# Python on iOS README

**iOS support is [tier 3](https://peps.python.org/pep-0011/#tier-3).**

This document provides a quick overview of some iOS specific features in the
Python distribution.

These instructions are only needed if you're planning to compile Python for iOS
yourself. Most users should *not* need to do this. If you're looking to
experiment with writing an iOS app in Python, tools such as [BeeWare's
Briefcase](https://briefcase.readthedocs.io) and [Kivy's
Buildozer](https://buildozer.readthedocs.io) will provide a much more
approachable user experience.

## Compilers for building on iOS

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

## Building Python on iOS

### ABIs and Architectures

iOS apps can be deployed on physical devices, and on the iOS simulator. Although
the API used on these devices is identical, the ABI is different - you need to
link against different libraries for an iOS device build (`iphoneos`) or an
iOS simulator build (`iphonesimulator`).

Apple uses the `XCframework` format to allow specifying a single dependency
that supports multiple ABIs. An `XCframework` is a wrapper around multiple
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

### Building a multi-architecture iOS XCframework

The `Apple` subfolder of the Python repository acts as a build script that
can be used to coordinate the compilation of a complete iOS XCframework. To use
it, run::

  python Apple build iOS

This will:

* Configure and compile a version of Python to run on the build machine
* Download pre-compiled binary dependencies for each platform
* Configure and build a `Python.framework` for each required architecture and
  iOS SDK
* Merge the multiple `Python.framework` folders into a single `Python.xcframework`
* Produce a `.tar.gz` archive in the `cross-build/dist` folder containing
  the `Python.xcframework`, plus a copy of the Testbed app pre-configured to
  use the XCframework.

The `Apple` build script has other entry points that will perform the
individual parts of the overall `build` target, plus targets to test the
build, clean the `cross-build` folder of iOS build products, and perform a
complete "build and test" CI run. The `--clean` flag can also be used on
individual commands to ensure that a stale build product are removed before
building.

### Building a single-architecture framework

If you're using the `Apple` build script, you won't need to build
individual frameworks. However, if you do need to manually configure an iOS
Python build for a single framework, the following options are available.

#### iOS specific arguments to configure

* `--enable-framework[=DIR]`

  This argument specifies the location where the Python.framework will be
  installed. If `DIR` is not specified, the framework will be installed into
  a subdirectory of the `iOS/Frameworks` folder.

  This argument *must* be provided when configuring iOS builds. iOS does not
  support non-framework builds.

* `--with-framework-name=NAME`

  Specify the name for the Python framework; defaults to `Python`.

  > [!NOTE]
  > Unless you know what you're doing, changing the name of the Python
  > framework on iOS is not advised. If you use this option, you won't be able
  > to run the `Apple` build script without making significant manual
  > alterations, and you won't be able to use any binary packages unless you
  > compile them yourself using your own framework name.

#### Building Python for iOS

The Python build system will create a `Python.framework` that supports a
*single* ABI with a *single* architecture. Unlike macOS, iOS does not allow a
framework to contain non-library content, so the iOS build will produce a
`bin` and `lib` folder in the same output folder as `Python.framework`.
The `lib` folder will be needed at runtime to support the Python library.

If you want to use Python in a real iOS project, you need to produce multiple
`Python.framework` builds, one for each ABI and architecture. iOS builds of
Python *must* be constructed as framework builds. To support this, you must
provide the `--enable-framework` flag when configuring the build. The build
also requires the use of cross-compilation. The minimal commands for building
Python for the ARM64 iOS simulator will look something like:
```
export PATH="$(pwd)/Apple/iOS/Resources/bin:/usr/bin:/bin:/usr/sbin:/sbin:/Library/Apple/usr/bin"
./configure \
    --enable-framework \
    --host=arm64-apple-ios-simulator \
    --build=arm64-apple-darwin \
    --with-build-python=/path/to/python.exe
make
make install
```

In this invocation:

* `Apple/iOS/Resources/bin` has been added to the path, providing some shims for the
  compilers and linkers needed by the build. Xcode requires the use of `xcrun`
  to invoke compiler tooling. However, if `xcrun` is pre-evaluated and the
  result passed to `configure`, these results can embed user- and
  version-specific paths into the sysconfig data, which limits the portability
  of the compiled Python. Alternatively, if `xcrun` is used *as* the compiler,
  it requires that compiler variables like `CC` include spaces, which can
  cause significant problems with many C configuration systems which assume that
  `CC` will be a single executable.

  To work around this problem, the `Apple/iOS/Resources/bin` folder contains some
  wrapper scripts that present as simple compilers and linkers, but wrap
  underlying calls to `xcrun`. This allows configure to use a `CC`
  definition without spaces, and without user- or version-specific paths, while
  retaining the ability to adapt to the local Xcode install. These scripts are
  included in the `bin` directory of an iOS install.

  These scripts will, by default, use the currently active Xcode installation.
  If you want to use a different Xcode installation, you can use
  `xcode-select` to set a new default Xcode globally, or you can use the
  `DEVELOPER_DIR` environment variable to specify an Xcode install. The
  scripts will use the default `iphoneos`/`iphonesimulator` SDK version for
  the select Xcode install; if you want to use a different SDK, you can set the
  `IOS_SDK_VERSION` environment variable. (e.g, setting
  `IOS_SDK_VERSION=17.1` would cause the scripts to use the `iphoneos17.1`
  and `iphonesimulator17.1` SDKs, regardless of the Xcode default.)

  The path has also been cleared of any user customizations. A common source of
  bugs is for tools like Homebrew to accidentally leak macOS binaries into an iOS
  build. Resetting the path to a known "bare bones" value is the easiest way to
  avoid these problems.

* `--host` is the architecture and ABI that you want to build, in GNU compiler
  triple format. This will be one of:

  - `arm64-apple-ios` for ARM64 iOS devices.
  - `arm64-apple-ios-simulator` for the iOS simulator running on Apple
    Silicon devices.
  - `x86_64-apple-ios-simulator` for the iOS simulator running on Intel
    devices.

* `--build` is the GNU compiler triple for the machine that will be running
  the compiler. This is one of:

  - `arm64-apple-darwin` for Apple Silicon devices.
  - `x86_64-apple-darwin` for Intel devices.

* `/path/to/python.exe` is the path to a Python binary on the machine that
  will be running the compiler. This is needed because the Python compilation
  process involves running some Python code. On a normal desktop build of
  Python, you can compile a python interpreter and then use that interpreter to
  run Python code. However, the binaries produced for iOS won't run on macOS, so
  you need to provide an external Python interpreter. This interpreter must be
  the same version as the Python that is being compiled. To be completely safe,
  this should be the *exact* same commit hash. However, the longer a Python
  release has been stable, the more likely it is that this constraint can be
  relaxed - the same micro version will often be sufficient.

* The `install` target for iOS builds is slightly different to other
  platforms. On most platforms, `make install` will install the build into
  the final runtime location. This won't be the case for iOS, as the final
  runtime location will be on a physical device.

  However, you still need to run the `install` target for iOS builds, as it
  performs some final framework assembly steps. The location specified with
  `--enable-framework` will be the location where `make install` will
  assemble the complete iOS framework. This completed framework can then
  be copied and relocated as required.

For a full CPython build, you also need to specify the paths to iOS builds of
the binary libraries that CPython depends on (such as XZ, LibFFI and OpenSSL).
This can be done by defining library specific environment variables (such as
`LIBLZMA_CFLAGS`, `LIBLZMA_LIBS`), and the `--with-openssl` configure
option. Versions of these libraries pre-compiled for iOS can be found in [this
repository](https://github.com/beeware/cpython-apple-source-deps/releases).
LibFFI is especially important, as many parts of the standard library
(including the `platform`, `sysconfig` and `webbrowser` modules) require
the use of the `ctypes` module at runtime.

By default, Python will be compiled with an iOS deployment target (i.e., the
minimum supported iOS version) of 13.0. To specify a different deployment
target, provide the version number as part of the `--host` argument - for
example, `--host=arm64-apple-ios15.4-simulator` would compile an ARM64
simulator build with a deployment target of 15.4.

## Testing Python on iOS

### Testing a multi-architecture framework

Once you have a built an XCframework, you can test that framework by running:

  $ python Apple test iOS

This test will attempt to find an "SE-class" simulator (i.e., an iPhone SE, or
iPhone 16e, or similar), and run the test suite on the most recent version of
iOS that is available. You can specify a simulator using the `--simulator`
command line argument, providing the name of the simulator (e.g., `--simulator
'iPhone 16 Pro'`). You can also use this argument to control the OS version used
for testing; `--simulator 'iPhone 16 Pro,OS=18.2'` would attempt to run the
tests on an iPhone 16 Pro running iOS 18.2.

If the test runner is executed on GitHub Actions, the `GITHUB_ACTIONS`
environment variable will be exposed to the iOS process at runtime.

### Testing a single-architecture framework

The `Apple/testbed` folder that contains an Xcode project that is able to run
the Python test suite on Apple platforms. This project converts the Python test
suite into a single test case in Xcode's XCTest framework. The single XCTest
passes if the test suite passes.

To run the test suite, configure a Python build for an iOS simulator (i.e.,
`--host=arm64-apple-ios-simulator` or `--host=x86_64-apple-ios-simulator`
), specifying a framework build (i.e. `--enable-framework`). Ensure that your
`PATH` has been configured to include the `Apple/iOS/Resources/bin` folder and
exclude any non-iOS tools, then run:
```
make all
make install
make testios
```

This will:

* Build an iOS framework for your chosen architecture;
* Finalize the single-platform framework;
* Make a clean copy of the testbed project;
* Install the Python iOS framework into the copy of the testbed project; and
* Run the test suite on an "entry-level device" simulator (i.e., an iPhone SE,
  iPhone 16e, or a similar).

On success, the test suite will exit and report successful completion of the
test suite. On a 2022 M1 MacBook Pro, the test suite takes approximately 15
minutes to run; a couple of extra minutes is required to compile the testbed
project, and then boot and prepare the iOS simulator.

### Debugging test failures

Running `python Apple test iOS` generates a standalone version of the
`Apple/testbed` project, and runs the full test suite. It does this using
`Apple/testbed` itself - the folder is an executable module that can be used
to create and run a clone of the testbed project. The standalone version of the
testbed will be created in a directory named
`cross-build/iOS-testbed.<timestamp>`.

You can generate your own standalone testbed instance by running:
```
python cross-build/iOS/testbed clone my-testbed
```

In this invocation, `my-testbed` is the name of the folder for the new
testbed clone.

If you've built your own XCframework, or you only want to test a single architecture,
you can construct a standalone testbed instance by running:
```
python Apple/testbed clone --platform iOS --framework <path/to/framework> my-testbed
```

The framework path can be the path path to a `Python.xcframework`, or the
path to a folder that contains a single-platform `Python.framework`.

You can then use the `my-testbed` folder to run the Python test suite,
passing in any command line arguments you may require. For example, if you're
trying to diagnose a failure in the `os` module, you might run:
```
python my-testbed run -- test -W test_os
```

This is the equivalent of running `python -m test -W test_os` on a desktop
Python build. Any arguments after the `--` will be passed to testbed as if
they were arguments to `python -m` on a desktop machine.

### Testing in Xcode

You can also open the testbed project in Xcode by running:
```
open my-testbed/iOSTestbed.xcodeproj
```

This will allow you to use the full Xcode suite of tools for debugging.

The arguments used to run the test suite are defined as part of the test plan.
To modify the test plan, select the test plan node of the project tree (it
should be the first child of the root node), and select the "Configurations"
tab. Modify the "Arguments Passed On Launch" value to change the testing
arguments.

The test plan also disables parallel testing, and specifies the use of the
`Testbed.lldbinit` file for providing configuration of the debugger. The
default debugger configuration disables automatic breakpoints on the
`SIGINT`, `SIGUSR1`, `SIGUSR2`, and `SIGXFSZ` signals.

### Testing on an iOS device

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
