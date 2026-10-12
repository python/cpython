# Python WASI (wasm32-wasi) build

**WASI support is [tier 2](https://peps.python.org/pep-0011/#tier-2).**

This directory contains configuration and helpers to facilitate cross
compilation of CPython to WebAssembly (WASM) using WASI. WASI builds
use WASM runtimes such as [wasmtime](https://wasmtime.dev/).

**NOTE**: If you are looking for general information about WebAssembly that is
not directly related to CPython, please see https://github.com/psf/webassembly.


## Working on the WASI build

This directory provides a CLI for building and packaging WASI builds for
distribution.

Run all commands below from the root of the CPython source checkout.

To see all the available commands, run:

```shell
python3 Platforms/WASI --help
```

The `python3` interpreter used to run this CLI must be a Python version that
is receiving bugfixes. This requirement does not apply to the build Python,
which the CLI builds from the source checkout.


### Prerequisites

There are some tools that must be available to successfully build.

1. C compiler
2. `make`
3. WASI SDK
4. Wasmtime (or some other WASI runtime configured via `--host-runner`)

The default runner requires Wasmtime be on `PATH`.

The WASI SDK must be the same version as specified in config.toml. The search
for the WASI SDK is done via:

1. `--wasi-sdk` CLI option
2. `WASI_SDK_PATH` environment variable
3. `/opt` where the WASI SDK has been unpacked from its tarball

Note that all prerequisites are included and configured appropriately in the
[WASI dev container image](https://github.com/python/cpython-devcontainers/pkgs/container/wasicontainer).
You can download it via:

```shell
podman pull ghcr.io/python/wasicontainer:latest
```

The `latest` image contains the WASI SDK versions required by all supported
CPython branches.

### Development loop

The common way to get started is to first do a full build:

```shell
python3 Platforms/WASI build --quiet --logdir cross-build/logs -- --with-pydebug --config-cache
```

In the end, you will end up with a "build Python" which is a local build of
Python used for cross-builds. You will also have the WASI build.

Once you have the build you can run the test you want.

Bash:
```bash
"$(python3 Platforms/WASI path)/python.sh" -m test test_os
```

Fish:
```fish
set -l wasi_dir (python3 Platforms/WASI path); "$wasi_dir/python.sh" -m test test_os
```

If you are working on C code and need a rebuild:

```shell
python3 Platforms/WASI make-host
```


### Building

In general,
[the devguide covers how to build and run for WASI](https://devguide.python.org/getting-started/setup-building/#wasi),
but we will cover some of the details here.

The simplest way to get a pydebug WASI build is:

```shell
python3 Platforms/WASI build -- --with-pydebug
```

This builds the build Python and the WASI build with `--with-pydebug` passed to
`configure` (as is anything that comes after `--`). The builds are placed in
the `cross-build/` directory of the source checkout, each in a subdirectory
matching the compiler triple for the build.

You can do the two builds separately if you want:

```shell
python3 Platforms/WASI build-python -- --with-pydebug
python3 Platforms/WASI build-host
```

This can be broken down even more to the separate `configure` and `make` steps:

```shell
python3 Platforms/WASI configure-build-python -- --with-pydebug
python3 Platforms/WASI make-build-python
python3 Platforms/WASI configure-host
python3 Platforms/WASI make-host
```

Note that `configure-host` figures out to do a pydebug build by looking at the
build Python.

There is a `--quiet` flag to redirect output from the underlying commands to a
directory. The `--logdir` flag controls where the log files go (which defaults
to `/tmp`).

```shell
python3 Platforms/WASI build --quiet --logdir cross-build/logs
```


### Packaging

The `package` command is used to gather all the files necessary to make a
release and place them in an archive:

```shell
python3 Platforms/WASI package
```

The files are gathered into a versioned directory inside `dist/` in the source
checkout. An archive containing that directory is placed alongside it in
`dist/`. The gathered directory includes a `bin/python3.wasmtime` file to ease
launching the interpreter.

If you just need to gather the files for a release, you can use the `gather`
command:

```shell
python3 Platforms/WASI gather
```

Both `package` and `gather` require a completed build and delete the entire
existing `dist/` directory before gathering files.

There is no command just to archive the gathered files.


### Paths

The `path` command prints the location of the build Python, WASI build, or
gathered distribution files:

```shell
python3 Platforms/WASI path build-python
python3 Platforms/WASI path wasi
python3 Platforms/WASI path dist
```

With no location argument, `path` defaults to `wasi`. The `dist` location
returns the versioned distribution directory inside `dist/`, not the top-level
`dist/` directory. The `build-python` and `wasi` locations can be queried before
building; the `dist` location requires WASI build metadata to determine the
versioned directory name.


### Cleanup

The `clean` command deletes the entire `cross-build/` and `dist/` directories,
including all build files, gathered distribution files, and archives:

```shell
python3 Platforms/WASI clean
```


### Testing

To find out where the WASI build directory is, you can run:

```shell
python3 Platforms/WASI path
```

From there, you can run the test suite.

For Bash-like shells:

```bash
make buildbottest -C "$(python3 Platforms/WASI path)"
```

For fish:

```fish
make buildbottest -C (python3 Platforms/WASI path)
```

There is a `python.sh` file in the WASI build directory, so you can also run
tests that way.

Bash:

```bash
"$(python3 Platforms/WASI path)/python.sh" -m test
```

Fish:

```fish
set -l wasi_dir (python3 Platforms/WASI path); "$wasi_dir/python.sh" -m test
```

If you want to test the files meant for distribution, the directory containing
the files can be found via `python3 Platforms/WASI path dist` and there is a
`bin/python3.wasmtime` shell script.

Bash:

```bash
"$(python3 Platforms/WASI path dist)/bin/python3.wasmtime" -m test
```

Fish:

```fish
set -l dist_dir (python3 Platforms/WASI path dist); "$dist_dir/bin/python3.wasmtime" -m test
```


## Detecting WASI builds

### Python code

```python
import os, sys

if sys.platform == "wasi":
    # Python on WASI
    ...

if os.name == "posix":
    # WASM platforms identify as POSIX-like.
    # Windows does not provide os.uname().
    machine = os.uname().machine
    if machine.startswith("wasm"):
        # WebAssembly (wasm32, wasm64 potentially in the future)
```

```python
>>> import os, sys
>>> os.uname()
posix.uname_result(
    sysname='wasi',
    nodename='(none)',
    release='0.0.0',
    version='0.0.0',
    machine='wasm32'
)
>>> os.name
'posix'
>>> sys.platform
'wasi'
```


### C code

WASI SDK defines several built-in macros. You can dump a full list of built-ins
with ``/path/to/wasi-sdk/bin/clang -dM -E - < /dev/null``.

* WebAssembly ``__wasm__`` (also ``__wasm``)
* wasm32 ``__wasm32__`` (also ``__wasm32``)
* wasm64 ``__wasm64__``
* WASI ``__wasi__``
