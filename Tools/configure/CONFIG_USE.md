# CPython CI Configure Usage Summary

This file documents how `./configure` is invoked across CPython's GitHub Actions
CI workflows, to ensure the Python-based `configure.py` supports all required
options.

## Testing Checklist

Each item tracks whether `configure.py` correctly handles that flag or scenario.

### Build variant flags
- [x] `--with-pydebug` — enable debug build (Py_DEBUG)
- [x] `--disable-gil` — free-threading build (no GIL)
- [x] `--enable-shared` — build shared library
- [x] `--without-pymalloc` — disable pymalloc allocator

### Optimization / safety flags
- [x] `--enable-safety` — enable safety checks
- [x] `--enable-slower-safety` — enable slower safety checks
- [x] `--enable-bolt` — enable BOLT binary optimization
- [x] `--config-cache` — read/write `config.cache` to speed up re-runs

### JIT / interpreter flags
- [x] `--enable-experimental-jit` — enable the JIT compiler
- [x] `--enable-experimental-jit=interpreter` — JIT interpreter-only mode
- [x] `--with-tail-call-interp` — enable tail-call interpreter

### Cross-compilation / universal binary flags
- [x] `--prefix=PATH` — installation prefix
- [ ] `--with-build-python=PATH` — build-system Python for cross-compilation (needs two-stage build)
- [x] `--enable-universalsdk` — build macOS universal binary (macOS only)
- [x] `--with-universal-archs=universal2` — macOS arm64+x86_64 universal binary (macOS only)

### SSL / crypto flags
- [x] `--with-openssl=PATH` — path to OpenSSL installation
- [x] `--with-builtin-hashlib-hashes=blake2` — build blake2 into hashlib
- [x] `--with-ssl-default-suites=openssl` — use OpenSSL default cipher suites

### Sanitizer flags
- [x] `--with-address-sanitizer` — enable AddressSanitizer
- [x] `--with-thread-sanitizer` — enable ThreadSanitizer
- [x] `--with-undefined-behavior-sanitizer` — enable UBSan

### Environment variables passed to configure
- [x] `CC=clang-N` — custom compiler (tail-call.yml, jit.yml)
- [x] `CFLAGS="..."` — extra compiler flags (e.g. `-fdiagnostics-format=json`)
- [x] `MACOSX_DEPLOYMENT_TARGET=10.15` — macOS min deployment target (macOS only)
- [x] `GDBM_CFLAGS`, `GDBM_LIBS` — GDBM library paths (macOS only)
- [x] `PROFILE_TASK=...` — PGO task override (reusable-ubuntu.yml)

### Invocation scenarios
- [x] In-tree build: `./configure [flags]`
- [x] Out-of-tree build: invoke as `../srcdir/configure` from a build directory
- [ ] WASI wrapper passthrough: `python3 Platforms/WASI configure-build-python -- --config-cache --with-pydebug` (needs WASI SDK)

---

## Workflow Files with Configure Invocations

### build.yml (Tests)

**Platforms:** ubuntu-24.04, ubuntu-24.04-arm, macos-14, macos-15-intel, android, ios, wasi

```sh
# check-generated-files
./configure --config-cache --with-pydebug --enable-shared

# build-ubuntu-ssltests-openssl
./configure CFLAGS="-fdiagnostics-format=json" \
  --config-cache --enable-slower-safety --with-pydebug \
  --with-openssl="$OPENSSL_DIR"

# build-ubuntu-ssltests-awslc
./configure CFLAGS="-fdiagnostics-format=json" \
  --config-cache --enable-slower-safety --with-pydebug \
  --with-openssl="$OPENSSL_DIR" \
  --with-builtin-hashlib-hashes=blake2 \
  --with-ssl-default-suites=openssl

# test-hypothesis (out-of-tree build from $BUILD_DIR)
../cpython-ro-srcdir/configure \
  --config-cache --with-pydebug --enable-slower-safety \
  --with-openssl="$OPENSSL_DIR"

# build-asan
./configure --config-cache --with-address-sanitizer --without-pymalloc

# cross-build-linux (build host python, then cross-compile)
./configure --prefix="$BUILD_DIR/host-python"
./configure --prefix="$BUILD_DIR/cross-python" \
  --with-build-python="$BUILD_DIR/host-python/bin/python3"
```

---

### jit.yml (JIT)

**Platforms:** ubuntu-24.04, ubuntu-24.04-arm, macos-14, macos-15-intel, windows-2022, windows-11-arm

```sh
# interpreter job
./configure --enable-experimental-jit=interpreter --with-pydebug

# jit - Linux (--with-pydebug conditional on debug matrix var)
./configure --enable-experimental-jit [--with-pydebug]

# jit - macOS
./configure --enable-experimental-jit \
  --enable-universalsdk --with-universal-archs=universal2 [--with-pydebug]

# jit-with-disabled-gil
./configure --enable-experimental-jit --with-pydebug --disable-gil

# tail-call-jit (with explicit compiler)
CC=clang-$LLVM ./configure --enable-experimental-jit \
  --with-tail-call-interp --with-pydebug
```

---

### reusable-docs.yml (Docs)

**Platforms:** ubuntu-24.04

```sh
./configure --with-pydebug
```

---

### reusable-macos.yml

**Platforms:** macos-14, macos-15-intel

Environment: `MACOSX_DEPLOYMENT_TARGET=10.15`, `GDBM_CFLAGS`, `GDBM_LIBS`

```sh
./configure \
  --config-cache \
  --with-pydebug \
  --enable-slower-safety \
  --enable-safety \
  [--disable-gil] \
  --prefix=/opt/python-dev \
  --with-openssl="$(brew --prefix openssl@3.0)"
```

---

### reusable-san.yml (Sanitizers)

**Platforms:** ubuntu-24.04

```sh
./configure \
  --config-cache \
  --with-pydebug \
  [--with-thread-sanitizer | --with-undefined-behavior-sanitizer] \
  [--disable-gil]
```

---

### reusable-ubuntu.yml

**Platforms:** ubuntu-24.04, ubuntu-24.04-arm (out-of-tree build)

Environment: `PROFILE_TASK='-m test --pgo --ignore test_unpickle_module_race'`

```sh
../cpython-ro-srcdir/configure \
  --config-cache \
  --with-pydebug \
  --enable-slower-safety \
  --enable-safety \
  --with-openssl="$OPENSSL_DIR" \
  [--disable-gil] \
  [--enable-bolt]
```

---

### reusable-wasi.yml

**Platforms:** ubuntu-24.04-arm

Uses a Python wrapper rather than calling `./configure` directly:

```sh
python3 Platforms/WASI configure-build-python -- --config-cache --with-pydebug
python3 Platforms/WASI configure-host -- --config-cache
```

---

### tail-call.yml (Tail-calling interpreter)

**Platforms:** ubuntu-24.04, ubuntu-24.04-arm, macos-14, macos-15-intel, windows-2022, windows-2025-vs2026, windows-11-arm

```sh
# Linux
CC=clang-20 ./configure --with-tail-call-interp --with-pydebug

# Linux free-threading
CC=clang-20 ./configure --with-tail-call-interp --disable-gil

# macOS
CC=clang-20 ./configure --with-tail-call-interp
```

---

## Notes for configure.py Implementation

1. **Out-of-tree builds**: `configure` is invoked from a separate build directory
   with the source path as `../cpython-ro-srcdir/configure`. The `srcdir` must
   be correctly detected as distinct from the current directory.

2. **Environment variable passthrough**: `CC`, `CFLAGS`, and library-path variables
   (`GDBM_CFLAGS`, `GDBM_LIBS`) must be accepted and propagated into the build.

3. **`--config-cache`**: Used extensively; must read/write `config.cache` to
   speed up repeated configure runs.

4. **Conditional flags**: `--disable-gil`, `--with-pydebug`, etc. are passed
   conditionally based on matrix variables — all must be accepted gracefully.

5. **WASI**: Uses a wrapper script, not `./configure` directly. The wrapper
   passes `--config-cache` and `--with-pydebug` through, so those flags must
   work when configure is invoked as a sub-step.

6. **Windows**: JIT and tail-call workflows include Windows runners, but
   configure on Windows uses PCbuild/MSBuild — no `./configure` call is made.
