# Python on tvOS README

This document provides a quick overview of some tvOS specific features in the
Python distribution.

Python on tvOS is largely the same as Python on iOS; most of the instructions in
the [iOS README](../iOS/README.md) can be used for tvOS builds, substituting:

- `tvOS` as the identifier for `Platforms/Apple` invocations; e.g.,
    - `python Platforms/Apple build tvOS`
    - `python Platforms/Apple ci tvOS`
    - `python Platforms/Apple test tvOS`

- tvOS resources are stored in the `Platforms/Apple/tvOS/Resources` directory.

- The tvOS SDK version is controlled with the `TVOS_SDK_VERSION` environment
  variable.

- The minimum supported tvOS SDK version is controlled with the
  `TVOS_DEPLOYMENT_TARGET` environment variable.

- When referencing platforms, the tvOS compiler triples are:
  - `arm64-apple-tvos` for AppleTV devices
  - `arm64-apple-tvos-simulator` for AppleTV simulators on ARM64 hardware
  - `x86_64-apple-tvos-simulator` for AppleTV simulators on x86-64 hardware

- The tvOS testbed Xcode template project is stored in
  `Platforms/Apple/testbed/tvOSTestbed`.

- `platform.ios_ver()` will return tvOS-relevant version information.
