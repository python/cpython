# Python on visionOS README

This document provides a quick overview of some visionOS specific features in the
Python distribution.

Python on visionOS is largely the same as Python on iOS; most of the instructions in
the [iOS README](../iOS/README.md) can be used for visionOS builds, substituting:

- `visionOS` as the identifier for `Platforms/Apple` invocations; e.g.,
    - `python Platforms/Apple build visionOS`
    - `python Platforms/Apple ci visionOS`
    - `python Platforms/Apple test visionOS`

- visionOS resources are stored in the `Platforms/Apple/visionOS/Resources`
  directory.

- The visionOS SDK version is controlled with the `XROS_SDK_VERSION` environment
  variable.

- The minimum supported visionOS SDK version is controlled with the
  `XROS_DEPLOYMENT_TARGET` environment variable.

- When referencing platforms, the visionOS compiler triples are:
  - `arm64-apple-xros` for Apple Vision devices
  - `arm64-apple-xros-simulator` for Apple Vision simulators

- The visionOS testbed Xcode template project is stored in
  `Platforms/Apple/testbed/visionOSTestbed`.

- `platform.ios_ver()` will return visionOS-relevant version information.
