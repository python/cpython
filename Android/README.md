# Python for Android

These instructions are only needed if you're planning to compile Python for
Android yourself. Most users should *not* need to do this. If you're looking to
use Python on Android, one of the following tools will provide a much more
approachable user experience:

* [Briefcase](https://briefcase.readthedocs.io), from the BeeWare project
* [Buildozer](https://buildozer.readthedocs.io), from the Kivy project
* [Chaquopy](https://chaquo.com/chaquopy/)


## Prerequisites

Export the `ANDROID_HOME` environment variable to point at your Android SDK. If
you don't already have the SDK, here's how to install it:

* Download the "Command line tools" from <https://developer.android.com/studio>.
* Create a directory `android-sdk/cmdline-tools`, and unzip the command line
  tools package into it.
* Rename `android-sdk/cmdline-tools/cmdline-tools` to
  `android-sdk/cmdline-tools/latest`.
* `export ANDROID_HOME=/path/to/android-sdk`

The `android.py` script also requires the following commands to be on the `PATH`:

* `curl`
* `java`
* `tar`
* `unzip`


## Building

Python can be built for Android on any POSIX platform supported by the Android
development tools, which currently means Linux or macOS. This involves doing a
cross-build where you use a "build" Python (for your development machine) to
help produce a "host" Python for Android.

First, make sure you have all the usual tools and libraries needed to build
Python for your development machine. The only Android tool you need to install
is the command line tools package above: the build script will download the
rest.

The easiest way to do a build is to use the `android.py` script. You can either
have it perform the entire build process from start to finish in one step, or
you can do it in discrete steps that mirror running `configure` and `make` for
each of the two builds of Python you end up producing.

The discrete steps for building via `android.py` are:

```sh
./android.py configure-build
./android.py make-build
./android.py configure-host HOST
./android.py make-host HOST
```

`HOST` identifies which architecture to build. To see the possible values, run
`./android.py configure-host --help`.

To do all steps in a single command, run:

```sh
./android.py build HOST
```

In the end you should have a build Python in `cross-build/build`, and an Android
build in `cross-build/HOST`.

You can use `--` as a separator for any of the `configure`-related commands –
including `build` itself – to pass arguments to the underlying `configure`
call. For example, if you want a pydebug build that also caches the results from
`configure`, you can do:

```sh
./android.py build HOST -- -C --with-pydebug
```


## Testing

To run the Python test suite on Android:

* Install Android Studio, if you don't already have it.
* Follow the instructions in the previous section to build all supported
  architectures.
* Run `./android.py setup-testbed` to download the Gradle wrapper.
* Open the `testbed` directory in Android Studio.
* In the *Device Manager* dock, connect a device or start an emulator.
  Then select it from the drop-down list in the toolbar.
* Click the "Run" button in the toolbar.
* The testbed app displays nothing on screen while running. To see its output,
  open the [Logcat window](https://developer.android.com/studio/debug/logcat).

To run specific tests, or pass any other arguments to the test suite, edit the
command line in testbed/app/src/main/python/main.py.
