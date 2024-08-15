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
* `java` (or set the `JAVA_HOME` environment variable)
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

The tests can be run on Linux, macOS, or Windows, although on Windows you'll
have to build the `cross-build/HOST` subdirectory on one of the other platforms
and copy it over.

The test suite can usually be run on a device with 2 GB of RAM, though for some
configurations or test orders you may need to increase this. As of Android
Studio Koala, 2 GB is the default for all emulators, although the user interface
may indicate otherwise. The effective setting is `hw.ramSize` in
~/.android/avd/*.avd/hardware-qemu.ini, whereas Android Studio displays the
value from config.ini. Changing the value in Android Studio will update both of
these files.

Before running the test suite, follow the instructions in the previous section
to build the architecture you want to test. Then run the test script in one of
the following modes:

* In `--connected` mode, it runs on a device or emulator you have already
  connected to the build machine. List the available devices with
  `$ANDROID_HOME/platform-tools/adb devices -l`, then pass a device ID to the
  script like this:

  ```sh
  ./android.py test --connected emulator-5554
  ```

* In `--managed` mode, it uses a temporary headless emulator defined in the
  `managedDevices` section of testbed/app/build.gradle.kts. This mode is slower,
  but more reproducible.

  We currently define two devices: `minVersion` and `maxVersion`, corresponding
  to our minimum and maximum supported Android versions. For example:

  ```sh
  ./android.py test --managed maxVersion
  ```

By default, the only messages the script will show are Python's own stdout and
stderr. Add the `-v` option to also show Gradle output, and non-Python logcat
messages.

Any other arguments on the `android.py test` command line will be passed through
to `python -m test` – use `--` to separate them from android.py's own options.
See the [Python Developer's
Guide](https://devguide.python.org/testing/run-write-tests/) for common options
– most of them will work on Android, except for those that involve subprocesses,
such as `-j`.

Every time you run `android.py test`, changes in pure-Python files in the
repository's `Lib` directory will be picked up immediately. Changes in C files,
and architecture-specific files such as sysconfigdata, will not take effect
until you re-run `android.py make-host` or `build`.
