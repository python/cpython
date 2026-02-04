# CPython Port for Nanvix

> **TL;DR:** This is a port of the CPython interpreter for the Nanvix operating system. Jump to [Quick Start](#quick-start) to get started immediately.

---

## Overview

This document describes the port of [CPython](https://www.python.org/) interpreter for the [Nanvix](https://github.com/nanvix/nanvix) operating system. This port enables Python to run on Nanvix, a POSIX-compatible educational operating system.

| Property | Value |
|----------|-------|
| **Base Version** | CPython 3.12.x |
| **Target Platform** | Nanvix (i686) |
| **Build System** | GNU Make (wrapping autoconf) |

**What's included:**
- ✅ Cross-compilation support for Nanvix
- ✅ Static library build (`libpython3.12.a`)
- ✅ Python interpreter executable (`python.elf`)
- ✅ Build helper scripts
- ✅ CI/CD integration

**Dependencies:**
- zlib (compression support)
- SQLite (database support)
- OpenSSL (cryptography support)

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Prerequisites](#prerequisites)
3. [Building](#building)
4. [Testing](#testing)
5. [Changes Summary](#changes-summary)
6. [Known Limitations](#known-limitations)
7. [CI/CD](#cicd)

---

## Quick Start

For experienced users who want to build quickly:

```bash
# 1. Pull the Docker image
docker pull nanvix/toolchain:v0.11.x-minimal

# 2. Download Nanvix sysroot and extract Nanvix commit SHA
curl -fsSL https://raw.githubusercontent.com/nanvix/nanvix/refs/heads/dev/scripts/get-nanvix.sh | bash -s -- nanvix-artifacts
tar -xjf nanvix-artifacts/*microvm*single*.tar.bz2 -C nanvix-artifacts
export NANVIX_HOME=$(find nanvix-artifacts -maxdepth 2 -type d -name "bin" -exec dirname {} \; | head -1)
# Extract Nanvix SHA from artifact filename for version matching
export NANVIX_SHA=$(ls nanvix-artifacts/*microvm*single*.tar.bz2 | sed -E 's/.*-([a-f0-9]{40})\.tar\.bz2$/\1/')

# 3. Download dependencies (must match Nanvix version)
# Download zlib
curl -fsSL -o zlib-release.tar.bz2 "https://github.com/nanvix/zlib/releases/latest/download/zlib-microvm-single-process.tar.bz2"
tar -xjf zlib-release.tar.bz2
cp -f zlib-*/lib/libz.a "$NANVIX_HOME/lib/"
cp -f zlib-*/include/*.h "$NANVIX_HOME/include/"

# Download SQLite
curl -fsSL -o sqlite-release.tar.bz2 "https://github.com/nanvix/sqlite/releases/latest/download/sqlite-microvm-single-process.tar.bz2"
tar -xjf sqlite-release.tar.bz2
cp -f sqlite-*/lib/libsqlite3.a "$NANVIX_HOME/lib/"
cp -f sqlite-*/include/*.h "$NANVIX_HOME/include/"

# Download OpenSSL
curl -fsSL -o openssl-release.tar.bz2 "https://github.com/nanvix/openssl/releases/latest/download/openssl-microvm-single-process.tar.bz2"
tar -xjf openssl-release.tar.bz2
cp -f openssl-*/lib/*.a "$NANVIX_HOME/lib/"
cp -rf openssl-*/include/openssl "$NANVIX_HOME/include/"

# 4. Build (Docker is used automatically if native toolchain is not found)
make -f Makefile.nanvix CONFIG_NANVIX=y NANVIX_HOME="$NANVIX_HOME"

# 5. Run tests
make -f Makefile.nanvix CONFIG_NANVIX=y NANVIX_HOME="$NANVIX_HOME" test
```

Continue reading for detailed instructions.

---

## Prerequisites

You need the following components to build CPython for Nanvix:

| Component | Description | Default Location |
|-----------|-------------|------------------|
| **Nanvix Toolchain** | i686-nanvix cross-compiler | `$HOME/toolchain` |
| **Nanvix Sysroot** | System libraries and linker script | `$HOME/nanvix` |
| **zlib** | Compression library (must match Nanvix version) | Installed in sysroot |
| **SQLite** | Database library (must match Nanvix version) | Installed in sysroot |
| **OpenSSL** | Cryptography library (must match Nanvix version) | Installed in sysroot |

> **Important:** All dependency libraries must be built against the same Nanvix version you are using. The CI workflow automatically finds and downloads matching releases based on the Nanvix commit SHA.

### Available Platform Configurations

| Platform | Process Mode | Artifact Pattern |
|----------|--------------|------------------|
| hyperlight | multi-process | `hyperlight.*multi-process` |
| hyperlight | single-process | `hyperlight.*single-process` |
| microvm | single-process | `microvm.*single-process` |
| microvm | multi-process | `microvm.*multi-process` |

### Downloading Nanvix

```bash
curl -fsSL https://raw.githubusercontent.com/nanvix/nanvix/refs/heads/dev/scripts/get-nanvix.sh | bash -s -- nanvix-artifacts
```

The script downloads all release artifacts. Extract the one matching your target platform (see [Quick Start](#quick-start) for a complete example).

### Downloading Dependencies

CPython requires zlib, SQLite, and OpenSSL. **You must use releases that were built against the same Nanvix version.** The Nanvix commit SHA is embedded in the artifact filename (e.g., `nanvix-microvm-single-process-release-<SHA>.tar.bz2`).

To find matching dependency releases:

```bash
# 1. Extract Nanvix SHA from artifact filename
NANVIX_SHA=$(ls nanvix-artifacts/*microvm*single*.tar.bz2 | sed -E 's/.*-([a-f0-9]{40})\.tar\.bz2$/\1/')

# 2. Check dependency releases for ones built with matching Nanvix SHA
# The release notes contain the Nanvix commit SHA they were built against

# 3. Download matching dependencies (replace PLATFORM and PROCESS_MODE)
for dep in zlib sqlite openssl; do
  curl -fsSL -o "${dep}-release.tar.bz2" \
    "https://github.com/nanvix/${dep}/releases/latest/download/${dep}-PLATFORM-PROCESS_MODE.tar.bz2"
done
```

> **Note:** The CI workflow automatically finds and downloads dependency releases that match the Nanvix version by checking release notes for the Nanvix commit SHA.

---

## Building

### Using Docker (Recommended)

The Makefile supports automatic Docker fallback when the native toolchain is not available:

```bash
# Pull the Nanvix toolchain Docker image
docker pull nanvix/toolchain:v0.11.x-minimal

# Build (Docker is used automatically if native toolchain is not found)
make -f Makefile.nanvix CONFIG_NANVIX=y NANVIX_HOME=/path/to/nanvix/sysroot-debug
```

> **Note:** The sysroot (`NANVIX_HOME`) must contain `lib/libposix.a`, `lib/libz.a`, `lib/libsqlite3.a`, `lib/libssl.a`, `lib/libcrypto.a`, and `lib/user.ld` from a Nanvix build.

**Docker Fallback Behavior:**
- If `NANVIX_TOOLCHAIN` points to a valid toolchain, it uses the native compiler
- If the native toolchain is not found, it automatically uses Docker if available
- Use `CONFIG_NANVIX_DOCKER=y` to force Docker usage even when native toolchain exists
- Use `NANVIX_DOCKER_IMAGE` to specify a custom Docker image (default: `nanvix/toolchain:v0.11.x-minimal`)

### Using Native Toolchain

```bash
export NANVIX_TOOLCHAIN=/path/to/toolchain  # Contains: bin/i686-nanvix-gcc
export NANVIX_HOME=/path/to/nanvix          # Contains: lib/user.ld, lib/libposix.a, dependencies
make -f Makefile.nanvix CONFIG_NANVIX=y all
```

### Build Outputs

After a successful build, you will have:

| File | Description |
|------|-------------|
| `python.elf` | Python interpreter executable |
| `libpython3.12.a` | Python static library |

---

## Testing

> **Important:** Tests must be run through the Nanvix daemon (`nanvixd.elf`).

### Running the Test Suite

```bash
# Run all tests
make -f Makefile.nanvix CONFIG_NANVIX=y NANVIX_HOME=/path/to/nanvix test
```

### Running Individual Tests

To run Python interactively:

```bash
cd "$NANVIX_HOME" && echo "print('Hello, Nanvix!')" | ./bin/nanvixd.elf -- /path/to/python.elf
```

### Test Coverage

The test target verifies:
- Python interpreter starts correctly
- Basic print functionality works
- Arithmetic operations work
- Core module imports work (e.g., `sys`)

---

## Changes Summary

The following changes were made to support Nanvix.

### Build System Changes

| Change | Description |
|--------|-------------|
| New Makefile | Added `Makefile.nanvix` for Nanvix cross-compilation |
| Cross-compilation | Uses `CONFIG_NANVIX=y` option to enable Nanvix build |
| Docker support | Automatic Docker fallback when native toolchain not available |
| Configure wrapper | Wraps standard `./configure` with Nanvix cross-compilation settings |
| Linker flags | Added Nanvix-specific flags (`-T user.ld -static`) |
| Shared libraries | Disabled (not supported on Nanvix) |
| Test target | Modified to run via `nanvixd.elf` |

### Configuration Options

| Option | Description |
|--------|-------------|
| `--disable-shared` | Build static libraries only |
| `--disable-test-modules` | Don't build test modules |
| `--with-ensurepip=no` | Don't include pip |
| `--disable-ipv6` | Disable IPv6 support |
| `ac_cv_pthread_is_default=yes` | Force pthread detection |

### New Files

| File | Purpose |
|------|---------|
| `Makefile.nanvix` | Standalone Makefile for Nanvix cross-compilation |
| `NANVIX.md` | This documentation file |
| `.github/workflows/nanvix-ci.yml` | CI workflow for automated builds |

### Legacy Build Script

The `z` script is a legacy build helper that is superseded by `Makefile.nanvix`. It remains for backward compatibility but users should prefer the Makefile-based approach for consistency with other Nanvix ports.

---

## Known Limitations

| Limitation | Impact |
|------------|--------|
| **No shared libraries** | Only static library (`libpython3.12.a`) is built |
| **No pip** | Package installer not available (`--with-ensurepip=no`) |
| **No IPv6** | IPv6 networking disabled |
| **No test modules** | Test suite modules not built |
| **Static linking only** | All executables are statically linked |
| **Limited I/O** | Some file and network operations may be limited |

---

## CI/CD

The GitHub Actions workflow at `.github/workflows/nanvix-ci.yml` automates building and testing on every change.

### Trigger Events

| Event | Description |
|-------|-------------|
| Push to `nanvix/**` | Any push to Nanvix branches |
| PR to `nanvix/**` | Pull requests targeting Nanvix branches |
| Daily schedule | Runs at midnight UTC |
| Manual dispatch | Can be triggered manually |
| Repository dispatch | Triggered by `nanvix-release` events |

### Build Matrix

The CI runs on 4 different platform/process-mode configurations:

| Platform | Process Mode | Runner |
|----------|--------------|--------|
| hyperlight | multi-process | `ubuntu-latest` (container) |
| hyperlight | single-process | `ubuntu-latest` (container) |
| microvm | single-process | `ubuntu-latest` (container) |
| microvm | multi-process | `ubuntu-latest` (container) |

All configurations run in parallel with `fail-fast: false`, ensuring that all platforms are tested even if one fails.

### Dependency Management

The CI workflow automatically downloads matching releases of zlib, SQLite, and OpenSSL from their respective `nanvix/*` repositories before building CPython, ensuring the correct platform-specific libraries are used.

---
