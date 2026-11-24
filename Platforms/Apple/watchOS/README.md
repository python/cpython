# Python on watchOS README

This document provides a quick overview of some watchOS specific features in the
Python distribution.

Python on watchOS is largely the same as Python on iOS; most of the instructions in
the [iOS README](../iOS/README.md) can be used for watchOS builds, substituting:

- `watchOS` as the identifier for `Platforms/Apple` invocations; e.g.,
    - `python Platforms/Apple build watchOS`
    - `python Platforms/Apple ci watchOS`
    - `python Platforms/Apple test watchOS`

- watchOS resources are stored in the `Platforms/Apple/watchOS/Resources`
  directory

- The watchOS SDK version is controlled with the `WATCHOS_SDK_VERSION` environment
  variable.

- The minimum supported watchOS SDK version is controlled with the
  `WATCHOS_DEPLOYMENT_TARGET` environment variable.

- When referencing platforms, the watchOS compiler triples are:
  - `arm64_32-apple-watchos` for Apple Watch devices
  - `arm64-apple-watchos-simulator` for Apple Watch simulators on ARM64 hardware
  - `x86_64-apple-watchos-simulator` for Apple Watch simulators on x86_64 hardware

- The watchOS testbed Xcode template project is stored in
  `Platforms/Apple/testbed/watchOSTestbed`.

- `platform.ios_ver()` will return watchOS-relevant version information.
