# Sources

[`prim`](/include/mimalloc/prim.h) &rarr; [`os`](/include/mimalloc/prim/os.h) &rarr; [`arena`](/include/mimalloc/arena/arena-page.h`) &rarr; [`page`](/include/mimalloc/alloc/page.h) &rarr; [`alloc`](alloc/alloc.c)

* [`alloc`](alloc): "front-end": main allocation and free functions.

* [`arena`](arena): "back-end": management of large memory arenas allocated from the OS.

* [`heap`](heap): initialization, first-class heaps, thread local _theaps_, and subprocesses.

* [`prim`](prim): primitive functions needed from the host, and OS level allocations.

* [`profile`](profile): sampling for guarded allocations, profiled allocations, and the default profiler.

* [`util`](util): utility functions, libc, options, random numbers, statistics, and dynamic thread locals.

