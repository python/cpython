# Allocation and Freeing

- [`alloc.c`](alloc.c): Main allocation and reallocation functions, including the allocation fast paths.
- [`alloc-generic.c`](alloc-generic.c): Allocation slow path (`mi_malloc_generic`), page selection, deferred-free callbacks.
- [`alloc-aligned.c`](alloc-aligned.c): Aligned allocation and reallocation, including allocations with an alignment offset.
- [`alloc-posix.c`](alloc-posix.c): POSIX, Unix, and Microsoft-compatible allocation interfaces and convenience functions.
- [`alloc-override.c`](alloc-override.c): Standard allocator and C++ allocation overrides, included by `alloc.c`.
- [`free.c`](free.c): Local and cross-thread freeing, pointer validation, and usable-size queries, included by `alloc.c`.
- [`page.c`](page.c): Allocation-page initialization, free-list management, collection, and retirement.
- [`page.h`](page.h): Private declarations for page helpers used by generic allocation.
- [`page-queue.c`](page-queue.c): Size-class mapping and management of page queues.
- [`page-queue.h`](page-queue.h): Private page-queue declarations and inline helpers.
