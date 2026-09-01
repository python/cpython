# Python WASI (wasm32-wasi) build

**WASI support is [tier 2](https://peps.python.org/pep-0011/#tier-2).**

This directory contains configuration and helpers to facilitate cross
compilation of CPython to WebAssembly (WASM) using WASI. WASI builds
use WASM runtimes such as [wasmtime](https://wasmtime.dev/).

**NOTE**: If you are looking for general information about WebAssembly that is
not directly related to CPython, please see https://github.com/psf/webassembly.

## Build

See [the devguide on how to build and run for WASI](https://devguide.python.org/getting-started/setup-building/#wasi).

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
