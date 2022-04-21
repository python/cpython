# Python WebAssembly (WASM) build

**WARNING: WASM support is highly experimental! Lots of features are not working yet.**

This directory contains configuration and helpers to facilitate cross
compilation of CPython to WebAssembly (WASM). For now we support
*wasm32-emscripten* builds for modern browser and for *Node.js*. WASI
(*wasm32-wasi*) is work-in-progress

## wasm32-emscripten build

For now the build system has two target flavors. The ``Emscripten/browser``
target (``--with-emscripten-target=browser``) is optimized for browsers.
It comes with a reduced and preloaded stdlib without tests and threading
support. The ``Emscripten/node`` target has threading enabled and can
access the file system directly.

Cross compiling to the wasm32-emscripten platform needs the
[Emscripten](https://emscripten.org/) SDK and a build Python interpreter.
Emscripten 3.1.8 or newer are recommended. All commands below are relative
to a repository checkout.

Christian Heimes maintains a container image with Emscripten SDK, Python
build dependencies, WASI-SDK, wasmtime, and several additional tools.

```
# Fedora, RHEL, CentOS
podman run --rm -ti -v $(pwd):/python-wasm/cpython:Z quay.io/tiran/cpythonbuild:emsdk3

# other
docker run --rm -ti -v $(pwd):/python-wasm/cpython quay.io/tiran/cpythonbuild:emsdk3
```

### Compile a build Python interpreter

```shell
mkdir -p builddir/build
pushd builddir/build
../../configure -C
make -j$(nproc)
popd
```

### Fetch and build additional emscripten ports

```shell
embuilder build zlib bzip2
```

### Cross compile to wasm32-emscripten for browser

```shell
mkdir -p builddir/emscripten-browser
pushd builddir/emscripten-browser

CONFIG_SITE=../../Tools/wasm/config.site-wasm32-emscripten \
  emconfigure ../../configure -C \
    --host=wasm32-unknown-emscripten \
    --build=$(../../config.guess) \
    --with-emscripten-target=browser \
    --with-build-python=$(pwd)/../build/python

emmake make -j$(nproc)
popd
```

Serve `python.html` with a local webserver and open the file in a browser.

```shell
emrun builddir/emscripten-browser/python.html
```

or

```shell
./Tools/wasm/wasm_webserver.py
```

and open http://localhost:8000/builddir/emscripten-browser/python.html . This
directory structure enables the *C/C++ DevTools Support (DWARF)* to load C
and header files with debug builds.

### Cross compile to wasm32-emscripten for node

```shell
mkdir -p builddir/emscripten-node
pushd builddir/emscripten-node

CONFIG_SITE=../../Tools/wasm/config.site-wasm32-emscripten \
  emconfigure ../../configure -C \
    --host=wasm32-unknown-emscripten \
    --build=$(../../config.guess) \
    --with-emscripten-target=node \
    --with-build-python=$(pwd)/../build/python

emmake make -j$(nproc)
popd
```

```shell
node --experimental-wasm-threads --experimental-wasm-bulk-memory builddir/emscripten-node/python.js
```

# wasm32-emscripten limitations and issues

Emscripten before 3.1.8 has known bugs that can cause memory corruption and
resource leaks. 3.1.8 contains several fixes for bugs in date and time
functions.

## Network stack

- Python's socket module does not work with Emscripten's emulated POSIX
  sockets yet. Network modules like ``asyncio``, ``urllib``, ``selectors``,
  etc. are not available.
- Only ``AF_INET`` and ``AF_INET6`` with ``SOCK_STREAM`` (TCP) or
  ``SOCK_DGRAM`` (UDP) are available. ``AF_UNIX`` is not supported.
- ``socketpair`` does not work.
- Blocking sockets are not available and non-blocking sockets don't work
  correctly, e.g. ``socket.accept`` crashes the runtime. ``gethostbyname``
  does not resolve to a real IP address. IPv6 is not available.
- The ``select`` module is limited. ``select.select()`` crashes the runtime
  due to lack of exectfd support.

## processes, threads, signals

- Processes are not supported. System calls like fork, popen, and subprocess
  fail with ``ENOSYS`` or ``ENOSUP``.
- Signal support is limited. ``signal.alarm``, ``itimer``, ``sigaction``
  are not available or do not work correctly. ``SIGTERM`` exits the runtime.
- Keyboard interrupt (CTRL+C) handling is not implemented yet.
- Browser builds cannot start new threads. Node's web workers consume
  extra file descriptors.
- Resource-related functions like ``os.nice`` and most functions of the
  ``resource`` module are not available.

## file system

- Most user, group, and permission related function and modules are not
  supported or don't work as expected, e.g.``pwd`` module, ``grp`` module,
  ``os.setgroups``, ``os.chown``, and so on. ``lchown`` and `lchmod`` are
  not available.
- ``umask`` is a no-op.
- hard links (``os.link``) are not supported.
- Offset and iovec I/O functions (e.g. ``os.pread``, ``os.preadv``) are not
  available.
- ``os.mknod`` and ``os.mkfifo``
  [don't work](https://github.com/emscripten-core/emscripten/issues/16158)
  and are disabled.
- Large file support crashes the runtime and is disabled.
- ``mmap`` module is unstable. flush (``msync``) can crash the runtime.

## Misc

- Heap memory and stack size are limited. Recursion or extensive memory
  consumption can crash Python.
- Most stdlib modules with a dependency on external libraries are missing,
  e.g. ``ctypes``, ``readline``, ``sqlite3``, ``ssl``, and more.
- Shared extension modules are not implemented yet. All extension modules
  are statically linked into the main binary. The experimental configure
  option ``--enable-wasm-dynamic-linking`` enables dynamic extensions
  supports. It's currently known to crash in combination with threading.
- glibc extensions for date and time formatting are not available.
- ``locales`` module is affected by musl libc issues,
  [bpo-46390](https://bugs.python.org/issue46390).
- Python's object allocator ``obmalloc`` is disabled by default.
- ``ensurepip`` is not available.

## wasm32-emscripten in browsers

- The interactive shell does not handle copy 'n paste and unicode support
  well.
- The bundled stdlib is limited. Network-related modules,
  distutils, multiprocessing, dbm, tests and similar modules
  are not shipped. All other modules are bundled as pre-compiled
  ``pyc`` files.
- Threading is disabled.
- In-memory file system (MEMFS) is not persistent and limited.
- Test modules are disabled by default. Use ``--enable-test-modules`` build
  test modules like ``_testcapi``.

## wasm32-emscripten in node

Node builds use ``NODERAWFS``, ``USE_PTHREADS`` and ``PROXY_TO_PTHREAD``
linker options.

- Node RawFS allows direct access to the host file system.
- pthread support requires WASM threads and SharedArrayBuffer (bulk memory).
  The runtime keeps a pool of web workers around. Each web worker uses
  several file descriptors (eventfd, epoll, pipe).

# Hosting Python WASM builds

The simple REPL terminal uses SharedArrayBuffer. For security reasons
browsers only provide the feature in secure environents with cross-origin
isolation. The webserver must send cross-origin headers and correct MIME types
for the JavaScript and WASM files. Otherwise the terminal will fail to load
with an error message like ``Browsers disable shared array buffer``.

## Apache HTTP .htaccess

Place a ``.htaccess`` file in the same directory as ``python.wasm``.

```
# .htaccess
Header set Cross-Origin-Opener-Policy same-origin
Header set Cross-Origin-Embedder-Policy require-corp

AddType application/javascript js
AddType application/wasm wasm

<IfModule mod_deflate.c>
    AddOutputFilterByType DEFLATE text/html application/javascript application/wasm
</IfModule>
```

# WASI (wasm32-wasi)

WASI builds require [WASI SDK](https://github.com/WebAssembly/wasi-sdk) and
currently [wasix](https://github.com/singlestore-labs/wasix) for POSIX
compatibility stubs.

# Detect WebAssembly builds

## Python code

```python
import os, sys

if sys.platform == "emscripten":
    # Python on Emscripten
if sys.platform == "wasi":
    # Python on WASI

if os.name == "posix":
    # WASM platforms identify as POSIX-like.
    # Windows does not provide os.uname().
    machine = os.uname().machine
    if machine.startswith("wasm"):
        # WebAssembly (wasm32, wasm64 in the future)
```

```python
>>> import os, sys
>>> os.uname()
posix.uname_result(sysname='Emscripten', nodename='emscripten', release='1.0', version='#1', machine='wasm32')
>>> os.name
'posix'
>>> sys.platform
'emscripten'
>>> sys._emscripten_info
sys._emscripten_info(
    emscripten_version=(3, 1, 8),
    runtime='Mozilla/5.0 (X11; Fedora; Linux x86_64; rv:99.0) Gecko/20100101 Firefox/99.0',
    pthreads=False,
    shared_memory=False
)
>>> sys._emscripten_info
sys._emscripten_info(emscripten_version=(3, 1, 8), runtime='Node.js v14.18.2', pthreads=True, shared_memory=True)
```

```python
>>> import os, sys
>>> os.uname()
posix.uname_result(sysname='wasi', nodename='(none)', release='0.0.0', version='0.0.0', machine='wasm32')
>>> os.name
'posix'
>>> sys.platform
'wasi'
```

## C code

Emscripten SDK and WASI SDK define several built-in macros. You can dump a
full list of built-ins with ``emcc -dM -E - < /dev/null`` and
``/path/to/wasi-sdk/bin/clang -dM -E - < /dev/null``.

```C
#ifdef __EMSCRIPTEN__
    // Python on Emscripten
#endif
```

* WebAssembly ``__wasm__`` (also ``__wasm``)
* wasm32 ``__wasm32__`` (also ``__wasm32``)
* wasm64 ``__wasm64__``
* Emscripten ``__EMSCRIPTEN__`` (also ``EMSCRIPTEN``)
* Emscripten version ``__EMSCRIPTEN_major__``, ``__EMSCRIPTEN_minor__``, ``__EMSCRIPTEN_tiny__``
* WASI ``__wasi__``

Feature detection flags:

* ``__EMSCRIPTEN_PTHREADS__``
* ``__EMSCRIPTEN_SHARED_MEMORY__``
* ``__wasm_simd128__``
* ``__wasm_sign_ext__``
* ``__wasm_bulk_memory__``
* ``__wasm_atomics__``
* ``__wasm_mutable_globals__``
