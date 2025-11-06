# Python WebAssembly (WASM) build

**WASI support is [tier 2](https://peps.python.org/pep-0011/#tier-2).**
**Emscripten support is [tier 3](https://peps.python.org/pep-0011/#tier-3).**

This directory contains configuration and helpers to facilitate cross
compilation of CPython to WebAssembly (WASM). Python supports Emscripten
(*wasm32-emscripten*) and WASI (*wasm32-wasi*) targets. Emscripten builds
run in modern browsers and JavaScript runtimes like *Node.js*. WASI builds
use WASM runtimes such as *wasmtime*.

**NOTE**: If you are looking for general information about WebAssembly that is
not directly related to CPython, please see https://github.com/psf/webassembly.

## Emscripten (wasm32-emscripten)

### Build

See [the devguide instructions for building for Emscripten](https://devguide.python.org/getting-started/setup-building/#emscripten).

### Running from node

If you want to run the normal Python CLI, you can use `python.sh`. It takes the
same options as the normal Python CLI entrypoint, though the REPL does not
function and will crash.

`python.sh` invokes `node_entry.mjs` which imports the Emscripten module for the
Python process and starts it up with the appropriate settings. If you wish to
make a node application that "embeds" the interpreter instead of acting like the
CLI you will need to write your own alternative to `node_entry.mjs`.


### Running tests

After building, you can run the full test suite with:
```shell
./cross-build/wasm32-emscripten/build/python/python.sh -m test -uall
```
You can run the browser smoke test with:
```shell
./Tools/wasm/emscripten/browser_test/run_test.sh
```

### The Web Example

When building for Emscripten, a small web example will be built automatically
in the ``web_example`` directory. To run the web example, ``cd`` into the
``web_example`` directory, then run ``python server.py``. This will start a web
server; you can then visit ``http://localhost:8000/`` in a browser to see a
simple REPL example.

The web example relies on a bug fix in Emscripten version 3.1.73 so if you build
with earlier versions of Emscripten it may not work. The web example uses
``SharedArrayBuffer``. For security reasons browsers only provide
``SharedArrayBuffer`` in secure environments with cross-origin isolation. The
webserver must send cross-origin headers and correct MIME types for the
JavaScript and WebAssembly files. Otherwise the terminal will fail to load with
an error message like ``ReferenceError: SharedArrayBuffer is not defined``. See
more information here:
https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/SharedArrayBuffer#security_requirements

Note that ``SharedArrayBuffer`` is _not required_ to use Python itself, only the
web example. If cross-origin isolation is not appropriate for your use case you
may make your own application embedding `python.mjs` which does not use
``SharedArrayBuffer`` and serve it without the cross-origin isolation headers.

### Embedding Python in a custom JavaScript application

You can look at `python.worker.mjs` and `node_entry.mjs` for inspiration. At a
minimum you must import ``createEmscriptenModule`` and you need to call
``createEmscriptenModule`` with an appropriate settings object. This settings
object will need a prerun hook that installs the Python standard library into
the Emscripten file system.

#### NodeJs

In Node, you can use the NodeFS to mount the standard library in your native
file system into the Emscripten file system:
```js
import createEmscriptenModule from "./python.mjs";

await createEmscriptenModule({
    preRun(Module) {
        Module.FS.mount(
            Module.FS.filesystems.NODEFS,
            { root: "/path/to/python/stdlib" },
            "/lib/",
        );
    },
});
```

#### Browser

In the browser, the simplest approach is to put the standard library in a zip
file it and install it. With Python 3.14 this could look like:
```js
import createEmscriptenModule from "./python.mjs";

await createEmscriptenModule({
  async preRun(Module) {
    Module.FS.mkdirTree("/lib/python3.14/lib-dynload/");
    Module.addRunDependency("install-stdlib");
    const resp = await fetch("python3.14.zip");
    const stdlibBuffer = await resp.arrayBuffer();
    Module.FS.writeFile(`/lib/python314.zip`, new Uint8Array(stdlibBuffer), {
      canOwn: true,
    });
    Module.removeRunDependency("install-stdlib");
  },
});
```

### Limitations and issues

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
    release='4.0.2',
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
