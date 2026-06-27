# Python for Android

If you obtained this README as part of a release package, then the only
applicable sections are "Prerequisites", "Testing", and "Using in your own app".

If you obtained this README as part of the CPython source tree, then you can
also follow the other sections to compile Python for Android yourself.

However, most app developers should not need to do any of these things manually.
Instead, use one of the tools listed
[here](https://docs.python.org/3/using/android.html), which will provide a much
easier experience.

## Prerequisites

If you already have an Android SDK installed, export the `ANDROID_HOME`
environment variable to point at its location. Otherwise, here's how to install
it:

* Download the "Command line tools" from <https://developer.android.com/studio>.
* Create a directory `android-sdk/cmdline-tools`, and unzip the command line
  tools package into it.
* Rename `android-sdk/cmdline-tools/cmdline-tools` to
  `android-sdk/cmdline-tools/latest`.
* `export ANDROID_HOME=/path/to/android-sdk`

The `Platforms/Android` script will automatically use the SDK's `sdkmanager` to install
any packages it needs.

The script also requires the following commands to be on the `PATH`:

* `curl`
* `java` (or set the `JAVA_HOME` environment variable)

## Building

Python can be built for Android on any POSIX platform supported by the Android
development tools, which currently means Linux or macOS.

First we'll make a "build" Python (for your development machine), then use it to
help produce a "host" Python for Android. So make sure you have all the usual
tools and libraries needed to build Python for your development machine.

The easiest way to do a build is to use the `Platforms/Android` script. You can either
have it perform the entire build process from start to finish in one step, or
you can do it in discrete steps that mirror running `configure` and `make` for
each of the two builds of Python you end up producing.

The discrete steps for building via `Platforms/Android` are:

```sh
python3 Platforms/Android configure-build
python3 Platforms/Android make-build
python3 Platforms/Android configure-host HOST
python3 Platforms/Android make-host HOST
```

`HOST` identifies which architecture to build. To see the possible values, run
`python3 Platforms/Android configure-host --help`.

To do all steps in a single command, run:

```sh
python3 Platforms/Android build HOST
```
In the end you should have a build Python in `cross-build/build`, and a host
Python in `cross-build/HOST`.

You can use `--` as a separator for any of the `configure`-related commands –
including `build` itself – to pass arguments to the underlying `configure`
call. For example, if you want a pydebug build that also caches the results from
`configure`, you can do:

```sh
python3 Platforms/Android build HOST -- -C --with-pydebug
```

## Packaging

After building an architecture as described in the section above, you can
package it for release with this command:

```sh
python3 Platforms/Android package HOST
```

`HOST` is defined in the section above.

This will generate a tarball in `cross-build/HOST/dist`, whose structure is
similar to the `Android` directory of the CPython source tree.

## Testing

Tests can be run  on Linux, macOS, or Windows, using either an Android emulator
or a physical device.

On Linux, the emulator needs access to the KVM virtualization interface. This may
require adding your user to a group, or changing your udev rules. On GitHub
Actions, the test script will do this automatically using the commands shown
[here](https://github.blog/changelog/2024-04-02-github-actions-hardware-accelerated-android-virtualization-now-available/).

The test script supports the following modes:

* In `--connected` mode, it runs on a device or emulator you have already
  connected to the build machine. List the available devices with
  `$ANDROID_HOME/platform-tools/adb devices -l`, then pass a device ID to the
  script like this:

  ```sh
  python3 Platforms/Android test --connected emulator-5554
  ```

* In `--managed` mode, it uses a temporary headless emulator defined in the
  `managedDevices` section of testbed/app/build.gradle.kts. This mode is slower,
  but more reproducible.

  We currently define two devices: `minVersion` and `maxVersion`, corresponding
  to our minimum and maximum supported Android versions. For example:

  ```sh
  python3 Platforms/Android test --managed maxVersion
  ```

By default, the only messages the script will show are Python's own stdout and
stderr. Add the `-v` option to also show Gradle output, and non-Python logcat
messages.

### Testing Python

You can run the test suite by doing a build as described above, and then running
`python3 Platforms/Android test`. On Windows, you won't be able to do the build
on the same machine, so you'll have to copy the `cross-build/HOST/prefix` directory
from somewhere else.

Extra arguments on the `Platforms/Android test` command line will be passed through
to `python -m test` – use `--` to separate them from `Platforms/Android`'s own options.
See the [Python Developer's
Guide](https://devguide.python.org/testing/run-write-tests/) for common options
– most of them will work on Android, except for those that involve subprocesses,
such as `-j`.

Every time you run `python3 Platforms/Android test`, changes in pure-Python files in the
repository's `Lib` directory will be picked up immediately. Changes in C files,
and architecture-specific files such as sysconfigdata, will not take effect
until you re-run `python3 Platforms/Android make-host` or `build`.

### Testing a third-party package

The `Platforms/Android` script is also included as `android.py` in the root of a
release package (i.e., the one built using `Platforms/Android package`).

You can use this script to test third-party packages by taking a release
package, extracting it wherever you want, and using the `android.py` script to
run the test suite for your third-party package.

Any argument that can be passed to `python3 Platforms/Android test` can also be
passed to `android.py`. The following options will be of particular use when
configuring the execution of a third-party test suite:

* `--cwd`: the directory of content to copy into the testbed app as the working
  directory.
* `--site-packages`: the directory to copy into the testbed app to use as site
  packages.

Extra arguments on the `android.py test` command line will be passed through to
Python – use `--` to separate them from `android.py`'s own options. You must include
either a `-c` or `-m` argument to specify how the test suite should be started.

For more details, run `android.py test --help`.

## Using in your own app

See https://docs.python.org/3/using/android.html.
