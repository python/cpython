# OS and Portability Primitives

- [`include/mimalloc/prim.h`](/src/include/mimalloc/prim.h): primitive portability API definition.
- [`prim.c`](prim.c): This is the portability layer where all primitives needed from the OS are defined.
            Selects one of [`unix/prim.c`](unix/prim.c), [`windows/prim.c`](windows/prim.c), [`wasi/prim.c`](wasi/prim.c), or [`emscripten/prim.c`](emscripten/prim.c) depending on the host platform (and on macOS, [`osx/prim.c`](osx/prim.c) defers to [`unix/prim.c`](unix/prim.c)).
- [`prim-tls.c`](prim-tls.c): primitive TLS model (thread local, pthreads, etc.)
- [`os.c`](os.c): A general higher level API around [`prim.c`](prim.c) that supports OS allocation, (de)commit, huge page allocation, etc.

