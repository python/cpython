# Python WebAssembly (WASM) build

**WASI support is [tier 2](https://peps.python.org/pep-0011/#tier-2).**
**Emscripten is NOT officially supported as of Python 3.13.**

This directory contains configuration and helpers to facilitate cross
compilation of CPython to WebAssembly (WASM). Python supports Emscripten
(*wasm32-emscripten*) and WASI (*wasm32-wasi*) targets. Emscripten builds
run in modern browsers and JavaScript runtimes like *Node.js*. WASI builds
use WASM runtimes such as *wasmtime*.

Users and developers are encouraged to use the script
`Tools/wasm/wasm_build.py`. The tool automates the build process and provides
assistance with installation of SDKs, running tests, etc.

**NOTE**: If you are looking for information that is not directly related to
building CPython for WebAssembly (or the resulting build), please see
https://github.com/psf/webassembly for more information.

## wasm32-emscripten

### Build

For now the build system has two target flavors. The ``Emscripten/browser``
target (``--with-emscripten-target=browser``) is optimized for browsers.
It comes with a reduced and preloaded stdlib without tests and threading
support. The ``Emscripten/node`` target has threading enabled and can
access the file system directly.

Cross compiling to the wasm32-emscripten platform needs the
[Emscripten](https://emscripten.org/) SDK and a build Python interpreter.
Emscripten 3.1.19 or newer are recommended. All commands below are relative
to a repository checkout.

#### Toolchain

##### Container image

Christian Heimes maintains a container image with Emscripten SDK, Python
build dependencies, WASI-SDK, wasmtime, and several additional tools.

From within your local CPython repo clone, run one of the following commands:

```
# Fedora, RHEL, CentOS
podman run --rm -ti -v $(pwd):/python-wasm/cpython:Z -w /python-wasm/cpython quay.io/tiran/cpythonbuild:emsdk3

# other
docker run --rm -ti -v $(pwd):/python-wasm/cpython -w /python-wasm/cpython quay.io/tiran/cpythonbuild:emsdk3
```

##### Manually

###### Install [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html)

**NOTE**: Follow the on-screen instructions how to add the SDK to ``PATH``.

```shell
git clone https://github.com/emscripten-core/emsdk.git /opt/emsdk
/opt/emsdk/emsdk install latest
/opt/emsdk/emsdk activate latest
```

###### Optionally: enable ccache for EMSDK

The ``EM_COMPILER_WRAPPER`` must be set after the EMSDK environment is
sourced. Otherwise the source script removes the environment variable.

```
. /opt/emsdk/emsdk_env.sh
EM_COMPILER_WRAPPER=ccache
```

###### Optionally: pre-build and cache static libraries

Emscripten SDK provides static builds of core libraries without PIC
(position-independent code). Python builds with ``dlopen`` support require
PIC. To populate the build cache, run:

```shell
. /opt/emsdk/emsdk_env.sh
embuilder build zlib bzip2 MINIMAL_PIC
embuilder --pic build zlib bzip2 MINIMAL_PIC
```


### Compile and build Python interpreter

From within the container, run the following command:

```shell
./Tools/wasm/wasm_build.py build
```

The command is roughly equivalent to:

```shell
mkdir -p builddir/build
pushd builddir/build
../../configure -C
make -j$(nproc)
popd
```

#### Cross-compile to wasm32-emscripten for browser

```shell
./Tools/wasm/wasm_build.py emscripten-browser
```

The command is roughly equivalent to:

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
Python comes with a minimal web server script that sets necessary HTTP
headers like COOP, COEP, and mimetypes. Run the script outside the container
and from the root of the CPython checkout.

```shell
./Tools/wasm/wasm_webserver.py
```

and open http://localhost:8000/builddir/emscripten-browser/python.html . This
directory structure enables the *C/C++ DevTools Support (DWARF)* to load C
and header files with debug builds.


#### Cross compile to wasm32-emscripten for node

```shell
./Tools/wasm/wasm_build.py emscripten-node-dl
```

The command is roughly equivalent to:

```shell
mkdir -p builddir/emscripten-node-dl
pushd builddir/emscripten-node-dl

CONFIG_SITE=../../Tools/wasm/config.site-wasm32-emscripten \
  emconfigure ../../configure -C \
    --host=wasm32-unknown-emscripten \
    --build=$(../../config.guess) \
    --with-emscripten-target=node \
    --enable-wasm-dynamic-linking \
    --with-build-python=$(pwd)/../build/python

emmake make -j$(nproc)
popd
```

```shell
node --experimental-wasm-threads --experimental-wasm-bulk-memory --experimental-wasm-bigint builddir/emscripten-node-dl/python.js
```

(``--experimental-wasm-bigint`` is not needed with recent NodeJS versions)

### Limitations and issues

Emscripten before 3.1.8 has known bugs that can cause memory corruption and
resource leaks. 3.1.8 contains several fixes for bugs in date and time
functions.

#### Network stack

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

#### processes, signals

- Processes are not supported. System calls like fork, popen, and subprocess
  fail with ``ENOSYS`` or ``ENOSUP``.
- Signal support is limited. ``signal.alarm``, ``itimer``, ``sigaction``
  are not available or do not work correctly. ``SIGTERM`` exits the runtime.
- Keyboard interrupt (CTRL+C) handling is not implemented yet.
- Resource-related functions like ``os.nice`` and most functions of the
  ``resource`` module are not available.

#### threading

- Threading is disabled by default. The ``configure`` option
  ``--enable-wasm-pthreads`` adds compiler flag ``-pthread`` and
  linker flags ``-sUSE_PTHREADS -sPROXY_TO_PTHREAD``.
- pthread support requires WASM threads and SharedArrayBuffer (bulk memory).
  The Node.JS runtime keeps a pool of web workers around. Each web worker
  uses several file descriptors (eventfd, epoll, pipe).
- It's not advised to enable threading when building for browsers or with
  dynamic linking support; there are performance and stability issues.

#### file system

- Most user, group, and permission related function and modules are not
  supported or don't work as expected, e.g.``pwd`` module, ``grp`` module,
  ``os.setgroups``, ``os.chown``, and so on. ``lchown`` and ``lchmod`` are
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

#### Misc

- Heap memory and stack size are limited. Recursion or extensive memory
  consumption can crash Python.
- Most stdlib modules with a dependency on external libraries are missing,
  e.g. ``ctypes``, ``readline``, ``ssl``, and more.
- Shared extension modules are not implemented yet. All extension modules
  are statically linked into the main binary. The experimental configure
  option ``--enable-wasm-dynamic-linking`` enables dynamic extensions
  supports. It's currently known to crash in combination with threading.
- glibc extensions for date and time formatting are not available.
- ``locales`` module is affected by musl libc issues,
  [gh-90548](https://github.com/python/cpython/issues/90548).
- Python's object allocator ``obmalloc`` is disabled by default.
- ``ensurepip`` is not available.
- Some ``ctypes`` features like ``c_longlong`` and ``c_longdouble`` may need
   NodeJS option ``--experimental-wasm-bigint``.

#### In the browser

- The interactive shell does not handle copy 'n paste and unicode support
  well.
- The bundled stdlib is limited. Network-related modules,
  multiprocessing, dbm, tests and similar modules
  are not shipped. All other modules are bundled as pre-compiled
  ``pyc`` files.
- In-memory file system (MEMFS) is not persistent and limited.
- Test modules are disabled by default. Use ``--enable-test-modules`` build
  test modules like ``_testcapi``.

### wasm32-emscripten in node

Node builds use ``NODERAWFS``.

- Node RawFS allows direct access to the host file system without need to
  perform ``FS.mount()`` call.

### wasm64-emscripten

- wasm64 requires recent NodeJS and ``--experimental-wasm-memory64``.
- ``EM_JS`` functions must return ``BigInt()``.
- ``Py_BuildValue()`` format strings must match size of types. Confusing 32
  and 64 bits types leads to memory corruption, see
  [gh-95876](https://github.com/python/cpython/issues/95876) and
  [gh-95878](https://github.com/python/cpython/issues/95878).

### Hosting Python WASM builds

The simple REPL terminal uses SharedArrayBuffer. For security reasons
browsers only provide the feature in secure environments with cross-origin
isolation. The webserver must send cross-origin headers and correct MIME types
for the JavaScript and WASM files. Otherwise the terminal will fail to load
with an error message like ``Browsers disable shared array buffer``.

#### Apache HTTP .htaccess

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

## WASI (wasm32-wasi)

See [the devguide on how to build and run for WASI](https://devguide.python.org/getting-started/setup-building/#wasi).

## Detecting WebAssembly builds

### Python code

```python
import os, sys

if sys.platform == "emscripten":
    # Python on Emscripten
    ...
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
    sysname='Emscripten',
    nodename='emscripten',
    release='3.1.19',
    version='#1',
    machine='wasm32'
)
>>> os.name
'posix'
>>> sys.platform
'emscripten'
>>> sys._emscripten_info
sys._emscripten_info(
    emscripten_version=(3, 1, 10),
    runtime='Mozilla/5.0 (X11; Linux x86_64; rv:104.0) Gecko/20100101 Firefox/104.0',
    pthreads=False,
    shared_memory=False
)
```

```python
>>> sys._emscripten_info
sys._emscripten_info(
    emscripten_version=(3, 1, 19),
    runtime='Node.js v14.18.2',
    pthreads=True,
    shared_memory=True
)
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

Emscripten SDK and WASI SDK define several built-in macros. You can dump a
full list of built-ins with ``emcc -dM -E - < /dev/null`` and
``/path/to/wasi-sdk/bin/clang -dM -E - < /dev/null``.

* WebAssembly ``__wasm__`` (also ``__wasm``)
* wasm32 ``__wasm32__`` (also ``__wasm32``)
* wasm64 ``__wasm64__``
* Emscripten ``__EMSCRIPTEN__`` (also ``EMSCRIPTEN``)
* Emscripten version ``__EMSCRIPTEN_major__``, ``__EMSCRIPTEN_minor__``, ``__EMSCRIPTEN_tiny__``
* WASI ``__wasi__``
