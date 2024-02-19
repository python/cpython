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


## Building

Building for Android requires doing a cross-build where you have a "build"
Python to help produce an Android build of CPython. This procedure has been
tested on Linux and macOS.

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

To see the possible values of HOST, run `./android.py configure-host --help`.

Or to do it all in a single command, run:

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
