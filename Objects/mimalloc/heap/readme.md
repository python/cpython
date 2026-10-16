# Heaps and Initialization

- [`heap.c`](heap.c): First-class heap creation, destruction, ownership, and management.
- [`theap.c`](theap.c): Thread-local heap instances and page collection.
- [`init.c`](init.c): Process and thread initialization, shutdown, and initial allocator state.
- [`subproc.c`](subproc.c): Subprocess allocation domains, metadata allocation, and subprocess lifecycle management.
- [`page-map.c`](page-map.c): Address-to-page mapping to safely map any pointer
to a valid mimalloc page (or `NULL`).

