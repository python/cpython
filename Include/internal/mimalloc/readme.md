# Mimalloc Headers

- [`mimalloc.h`](mimalloc.h): The main mimalloc API.
- [`mimalloc-override.h`](mimalloc-override.h): Macro-based redirection of standard allocation functions to mimalloc. One way to override all allocations functions is to include this file at the start of each source file (but of course, this only works
if you control all sources).
- [`mimalloc-new-delete.h`](mimalloc-new-delete.h): Global C++ `new` and `delete` overrides. Include this header in a single (!) source file in your project.
- [`mimalloc-stats.h`](mimalloc-stats.h): Statistics types and APIs for allocator statistics.
- [`mimalloc-profile.h`](mimalloc-profile.h): Allocation profiling callbacks, sampling controls, and the built-in pprof profiler API.
- [`mimalloc`](mimalloc): Internal allocator headers.
  See in particular [`types.h`](mimalloc/types.h) for the type definitions of all
  internal mimalloc data structures.

