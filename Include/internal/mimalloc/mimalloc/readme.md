# Internal Headers

- [`types.h`](types.h): Defines the types of all internal mimalloc datastructures
and configuration options.
- [`decl.h`](decl.h): Compiler attributes, assertions, initialization macros, and platform-specific declarations.
- [`bits.h`](bits.h): Architecture dependent type definitions, and bit operations.
- [`atomic.h`](atomic.h): Portable atomic operations and locks.
- [`prim.h`](prim.h): Primitive and minimal portability API needed from a host system.
- [`track.h`](track.h): Memory-tracking integration for Valgrind, AddressSanitizer, and Windows event tracing.
- [`qsbr.h`](qsbr.h): Quiescent-state based reclamation primitives (for runtime systems like CPython).
- [`internal.h`](internal.h): Deprecated: umbrella header for internal allocator declarations and utilities.

# Source Headers

- [`alloc`](alloc): Internal allocation, freeing, and page-management interfaces.
- [`arena`](arena): Arena management, slice allocation, and arena-backed page interfaces.
- [`heap`](heap): Heap, thread-local heap, subprocess, initialization, and page-map interfaces.
- [`prim`](prim): Higher-level OS memory operations and primitive thread-local access.
- [`profile`](profile): Sampled allocation, guarded allocation, and profiler interfaces.
- [`util`](util): Runtime utilities, options, randomness, statistics, and dynamic thread-local interfaces.

