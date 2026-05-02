# CPython Port for Nanvix

> **TL;DR:** This is a port of the CPython interpreter for the Nanvix operating system. Jump to [Quick Start](#quick-start) to get started immediately.

---

## Overview

This document describes the port of [CPython](https://www.python.org/) interpreter for the [Nanvix](https://github.com/nanvix/nanvix) operating system. This port enables Python to run on Nanvix, a POSIX-compatible educational operating system.

| Property | Value |
|----------|-------|
| **Base Version** | CPython 3.12.3 |
| **Target Platform** | Nanvix (i686) |
| **Build System** | GNU Make (wrapping autoconf) |
| **Build Orchestration** | [nanvix-zutil](https://github.com/nanvix/zutils) |

**What's included:**
- ✅ Cross-compilation support for Nanvix
- ✅ Static library build (`libpython3.12.a`)
- ✅ Python interpreter executable (`python.elf`)
- ✅ nanvix-zutil integration (`z.sh` / `z.ps1` / `.nanvix/z.py`)
- ✅ CI/CD integration via reusable workflow

**Dependencies:**
- zlib (compression support)
- SQLite (database support)
- OpenSSL (cryptography support)
- bzip2 (compression support)
- libffi (foreign function interface)

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Prerequisites](#prerequisites)
3. [Building](#building)
4. [Testing](#testing)
   - [Test Suite Status](#test-suite-status)
5. [Changes Summary](#changes-summary)
6. [Known Limitations](#known-limitations)
7. [CI/CD](#cicd)

---

## Quick Start

For experienced users who want to build quickly using nanvix-zutil:

```bash
# 1. Install nanvix-zutil
pip install nanvix-zutil

# 2. Setup (downloads Nanvix sysroot and all dependencies automatically)
./z setup

# 3. Build
./z build

# 4. Test
./z test

# 5. Package release tarballs
./z release
```

### Manual Build (without nanvix-zutil)

```bash
# 1. Pull the Docker image
docker pull nanvix/toolchain:latest-minimal

# 2. Download Nanvix sysroot
curl -fsSL https://raw.githubusercontent.com/nanvix/nanvix/refs/heads/dev/scripts/get-nanvix.sh | bash -s -- nanvix-artifacts
tar -xjf nanvix-artifacts/*microvm*single*.tar.bz2 -C nanvix-artifacts
export NANVIX_HOME=$(find nanvix-artifacts -maxdepth 2 -type d -name "bin" -exec dirname {} \; | head -1)

# 3. Download and install dependencies into sysroot
for dep in zlib sqlite openssl bzip2 libffi; do
  curl -fsSL -o ${dep}-release.tar.bz2 \
    "https://github.com/nanvix/${dep}/releases/latest/download/${dep}-microvm-single-process-128mb.tar.bz2"
  mkdir -p /tmp/${dep}-extract && tar -xjf ${dep}-release.tar.bz2 -C /tmp/${dep}-extract
  find /tmp/${dep}-extract -name '*.a' -exec cp -f {} "$NANVIX_HOME/lib/" \;
  find /tmp/${dep}-extract -name '*.h' -not -path '*/openssl/*' -exec cp -f {} "$NANVIX_HOME/include/" \;
done
mkdir -p "$NANVIX_HOME/include/openssl"
find /tmp/openssl-extract -path '*/openssl/*.h' -exec cp -f {} "$NANVIX_HOME/include/openssl/" \;

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
| **nanvix-zutil** | Build orchestration tool | `pip install nanvix-zutil` |
| **Nanvix Toolchain** | i686-nanvix cross-compiler | `$HOME/toolchain` |
| **Nanvix Sysroot** | System libraries and linker script | `$HOME/nanvix` |
| **zlib** | Compression library | Managed by nanvix-zutil |
| **SQLite** | Database library | Managed by nanvix-zutil |
| **OpenSSL** | Cryptography library | Managed by nanvix-zutil |
| **bzip2** | Compression library | Managed by nanvix-zutil |
| **libffi** | Foreign function interface | Managed by nanvix-zutil |

> **Note:** When using nanvix-zutil (`./z setup`), all dependencies are downloaded and
> installed automatically. The dependency versions are declared in `.nanvix/nanvix.toml`.

### Available Platform Configurations

| Platform | Process Mode | Artifact Pattern |
|----------|--------------|------------------|
| hyperlight | multi-process | `hyperlight.*multi-process` |
| hyperlight | single-process | `hyperlight.*single-process` |
| microvm | single-process | `microvm.*single-process` |
| microvm | multi-process | `microvm.*multi-process` |
| microvm | standalone | `microvm.*standalone` |

---

## Building

### Using nanvix-zutil (Recommended)

```bash
# Install nanvix-zutil
pip install nanvix-zutil

# Setup, build, and test in one go
./z setup && ./z build && ./z test
```

The `./z` entry point automatically delegates to `z.sh` on Linux/macOS or `z.ps1` on
Windows, which in turn invokes the `nanvix-zutil` CLI. Build logic is defined in
`.nanvix/z.py`.

### Using Docker

The Makefile supports automatic Docker fallback when the native toolchain is not available:

```bash
# Pull the Nanvix toolchain Docker image
docker pull nanvix/toolchain:latest-minimal

# Build (Docker is used automatically if native toolchain is not found)
make -f Makefile.nanvix CONFIG_NANVIX=y NANVIX_HOME=/path/to/nanvix/sysroot-debug
```

> **Note:** The sysroot (`NANVIX_HOME`) must contain `lib/libposix.a`, `lib/libz.a`,
> `lib/libsqlite3.a`, `lib/libssl.a`, `lib/libcrypto.a`, `lib/libbz2.a`, `lib/libffi.a`,
> and `lib/user.ld` from a Nanvix build.

**Docker Fallback Behavior:**
- If `NANVIX_TOOLCHAIN` points to a valid toolchain, it uses the native compiler
- If the native toolchain is not found, it automatically uses Docker if available
- Use `CONFIG_NANVIX_DOCKER=y` to force Docker usage even when native toolchain exists
- Use `NANVIX_DOCKER_IMAGE` to specify a custom Docker image (default: `nanvix/toolchain:latest-minimal`)

### Building on Windows

On Windows, cross-compilation is performed entirely inside Docker:

```powershell
# Prerequisites: Python 3, Make, and Docker Desktop must be installed and running.
# Avoid GnuWin32 Make 3.81; prefer ezwinports Make 4.4.1 (winget install ezwinports.make).
docker pull nanvix/toolchain:latest-minimal

.\z.ps1 setup
.\z.ps1 build
.\z.ps1 test
.\z.ps1 release
```

Set `NANVIX_DOCKER_IMAGE` to override the default Docker image.

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
# Using nanvix-zutil
./z test

# Or directly via Make
make -f Makefile.nanvix CONFIG_NANVIX=y NANVIX_HOME=/path/to/nanvix test
```

> **Note:** The `.nanvix/_test_staging/sysroot` directory is ephemeral — it is
> created by `make test` and removed automatically at the end of a successful
> run. To use the interactive or individual-module commands below, run
> `make test` first (or interrupt it after the staging step completes).

### Running Interactively

To start an interactive CPython session on Nanvix (standalone/microvm):

```bash
cd .nanvix/_test_staging/sysroot && \
  ./bin/nanvixd.elf -bin-dir ./bin -ramfs ../../cpython-rootfs.img \
    -- ./bin/python3.12 \
    "-i;PYTHONHOME=/ PYTHONDONTWRITEBYTECODE=1 _PYTHON_SYSCONFIGDATA_NAME=_sysconfigdata__nanvix_"
```

This launches `nanvixd` in standalone interactive mode with the CPython ramfs image
and drops you into the Python REPL:

```
Python 3.12.3 (...) [GCC 12.4.0] on nanvix
Type "help", "copyright", "credits" or "license" for more information.
>>> print(2 + 2)
4
```

> **Note:** The `-i` flag is part of the semicolon-delimited argument string passed to
> the guest binary. Environment variables (`PYTHONHOME`, `_PYTHON_SYSCONFIGDATA_NAME`)
> follow after the semicolon.

To run a one-shot script instead of the REPL:

```bash
cd .nanvix/_test_staging/sysroot && \
  ./bin/nanvixd.elf -bin-dir ./bin -ramfs ../../cpython-rootfs.img \
    -- ./bin/python3.12 \
    "-B ./test_hello.py;PYTHONHOME=/ PYTHONDONTWRITEBYTECODE=1 _PYTHON_SYSCONFIGDATA_NAME=_sysconfigdata__nanvix_"
```

### Running Individual Modules

To run a single test module inside the Nanvix VM:

```bash
cd .nanvix/_test_staging/sysroot && \
  ./bin/nanvixd.elf -bin-dir ./bin -ramfs ../../cpython-rootfs.img \
    -- ./bin/python3.12 \
    "-B -m test --verbose test_int;PYTHONHOME=/ PYTHONDONTWRITEBYTECODE=1 _PYTHON_SYSCONFIGDATA_NAME=_sysconfigdata__nanvix_"
```

### Test Suite Status

The `./z test` target runs **64 CPython stdlib test modules** on Nanvix
(i686, microvm, 256 MB RAM). In standalone mode, tests are split into
batches of 4 modules per VM invocation to stay within per-process memory
limits. Multi-process and single-process modes run all modules in a
single invocation.

All three deployment modes (**multi-process**, **single-process**, and
**standalone**) run tests via CPython's regrtest runner (`python -m test`).
In standalone mode, a `/tmp` directory is created on the ramfs so
`tempfile.gettempdir()` works, and modules are batched by
`run-tests.py`. Per-mode exclusions (e.g.
`test_filter_dealloc` on standalone to avoid OOM) are passed to regrtest
via `--ignore`.

| Metric | Value |
|--------|-------|
| **Modules enabled** | 64 |
| **Total tests run** | ~3,700 |
| **Tests passed** | ~90% |
| **Tests skipped** | ~10% (via `@skipIf(is_nanvix)`) |
| **Tests failed** | 0 |
| **Batches** | 16 |
| **Skip decorators added** | 73 |

#### Enabled Modules

| Group | Modules |
|-------|---------|
| Built-in Types | test_int, test_range, test_slice, test_memoryview, test_bytes, test_tuple |
| Operators & Expressions | test_builtin, test_operator, test_binop, test_unary, test_compare, test_richcmp, test_augassign, test_contains |
| Grammar, Syntax & Compiler | test_grammar, test_syntax, test_compile, test_compiler_assemble, test_compiler_codegen, test_ast, test_symtable, test_opcache, test_peepholer, test_dis, test_code, test_keyword, test_tokenize, test_perf_profiler |
| Function Calls & Control Flow | test_call, test_extcall, test_positional_only_arg, test_scope, test_global, test_dynamic, test_with |
| Data Types & Type System | test_types, test_typechecks, test_isinstance, test_hash, test_index, test_super, test_property |
| Math & Numerics | test_float, test_complex, test_bool, test_struct, test_math, test_cmath, test_decimal, test_fractions, test_statistics, test_random, test_numeric_tower |
| Exceptions & Tracebacks | test_exception_group, test_exceptions, test_raise, test_traceback |
| Stdlib & Containers | test_frame, test_contextlib, test_contextlib_async, test_pprint, test_reprlib, test_list, test_dict |

#### Excluded Modules

| Module | Reason |
|--------|--------|
| test_exception_hierarchy | Crashes at import — `errno.ESHUTDOWN` missing on Nanvix |
| test_inspect | VM hangs — asyncio event loop setup before skip; module too large for VM |

#### Skip Categories

| Category | Count | % of Skips |
|----------|-------|------------|
| Pickle corruption (32-bit) | 33 | 45% |
| Missing `_testcapi`/`_testinternalcapi` | 18 | 25% |
| No subprocess/fork | 6 | 8% |
| VM crash / deep recursion | 4 | 5% |
| Traceback formatting | 3 | 4% |
| Other (rounding, float precision, filesystem, 32-bit args) | 9 | 12% |

Each skip is annotated with a reason in the `@skipIf(is_nanvix, "...")` decorator
directly in the test source file.

---

## Changes Summary

The following changes were made to support Nanvix.

### Build System Changes

| Change | Description |
|--------|-------------|
| New Makefile | Added `Makefile.nanvix` for Nanvix cross-compilation |
| nanvix-zutil integration | `.nanvix/z.py` ZScript subclass for build orchestration |
| Package manifest | `.nanvix/nanvix.toml` declares version and dependencies |
| Thin wrappers | `z.sh`, `z.ps1`, `z` entry points delegate to nanvix-zutil |
| Cross-compilation | Uses `CONFIG_NANVIX=y` option to enable Nanvix build |
| Docker support | Automatic Docker fallback when native toolchain not available |
| Configure wrapper | Wraps standard `./configure` with Nanvix cross-compilation settings |
| Linker flags | Added Nanvix-specific flags (`-T user.ld -static`) |
| Shared libraries | Disabled (not supported on Nanvix) |
| Test target | Modified to run via `nanvixd.elf` |
| Package targets | `package` and `verify-package` for release tarball creation |

### Configuration Options

| Option | Description |
|--------|-------------|
| `--disable-shared` | Build static libraries only |
| `--disable-test-modules` | Don't build test modules |
| `--with-ensurepip=no` | Don't include pip |
| `--disable-ipv6` | Disable IPv6 support |
| `ac_cv_pthread_is_default=yes` | Force pthread detection |

### Nanvix-Specific Files

| File | Purpose |
|------|---------|
| `Makefile.nanvix` | Top-level Makefile (includes composable `.mk` files) |
| `.nanvix/mk/*.mk` | Composable Make includes (common, test modes, packaging) |
| `.nanvix/run-tests.py` | Host-side parallel batch runner (splits modules across VM invocations) |
| `.nanvix/run-regrtest.py` | Guest-side regrtest wrapper (invoked inside nanvixd VM) |
| `NANVIX.md` | This documentation file |
| `.nanvix/z.py` | ZScript subclass (build orchestration logic) |
| `.nanvix/nanvix.toml` | Package manifest with dependency declarations |
| `z` | Cross-platform entry point (routes to z.sh or z.ps1) |
| `z.sh` | Thin bash wrapper for nanvix-zutil |
| `z.ps1` | Thin PowerShell wrapper for nanvix-zutil |
| `.github/workflows/nanvix-ci.yml` | CI workflow (thin caller to reusable workflow) |

---

## Known Limitations

| Limitation | Impact |
|------------|--------|
| **No shared libraries** | Only static library (`libpython3.12.a`) is built |
| **No pip** | Package installer not available (`--with-ensurepip=no`) |
| **No IPv6** | IPv6 networking disabled |
| **Static linking only** | All executables are statically linked |
| **No sockets** | `socketpair()` unavailable; asyncio event loop cannot start |
| **No subprocess/fork** | `os.fork()`, `subprocess.Popen()` not supported |
| **Pickle corruption** | `pickle` produces corrupt data on 32-bit Nanvix; likely C accelerator issue |
| **Missing C test extensions** | `_testcapi` and `_testinternalcapi` not built |
| **Round-half-up** | C library uses round-half-up instead of IEEE 754 round-half-to-even |
| **Standalone ramfs limits** | A single shared ramfs image (~103 MB) is built once; batches share it across parallel VM invocations; some tests excluded for OOM |

---

## CI/CD

The GitHub Actions workflow at `.github/workflows/nanvix-ci.yml` is a thin caller that
invokes the reusable Nanvix CI workflow at
`nanvix/workflows/.github/workflows/nanvix-ci.yml@v1.0.0`.

### Trigger Events

| Event | Description |
|-------|-------------|
| Push to `nanvix/**` | Any push to Nanvix branches |
| PR to `nanvix/**` | Pull requests targeting Nanvix branches |
| Daily schedule | Runs at midnight UTC |
| Manual dispatch | Can be triggered manually |
| Repository dispatch | Triggered by dependency release events |

### Build Matrix

The CI runs across platform/process-mode/memory configurations:

| Platform | Process Mode | Runner |
|----------|--------------|--------|
| microvm | multi-process | `ubuntu-latest` (container) |
| microvm | single-process | `ubuntu-latest` (container) |
| microvm | standalone | `ubuntu-latest` (container) |

All configurations run in parallel with `fail-fast: false`.

### Dependency Management

Dependencies are declared in `.nanvix/nanvix.toml` and resolved automatically by
nanvix-zutil during `./z setup`. For each workspace, nanvix-zutil generates a
lockfile (`.nanvix/nanvix.lock`) that pins the exact resolved versions to enable
reproducible rebuilds in that workspace; this lockfile is not version-controlled
by default.

---
