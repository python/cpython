# Memory Arenas and Concurrent Bitmaps

- [`arena.c`](arena.c): Arena creation, reservation, traversal, and destruction.
- [`arena-alloc.c`](arena-alloc.c): Allocation, freeing, commitment, and purging of arena slices.
- [`arena-page.c`](arena-page.c): Allocation, abandonment, reclamation, and freeing of pages backed by arena slices.
- [`arena.h`](arena.h): Private arena declarations, traversal callbacks, and layout helpers.
- [`bitmap.c`](bitmap.c): Concurrent bitmap searches and range operations; these bitmaps are used to manage the slices in an arena.
- [`bitmap-chunk.c`](bitmap-chunk.c): Bitmaps consist of 512-bit chunks; this provides the low-level atomic bit-field and bitmap-chunk operations.
- [`bitmap.h`](bitmap.h): Bitmap types, constants, and operation declarations.
- [`bitmap-chunk.h`](bitmap-chunk.h): Private declarations and iteration helpers for bitmap fields and chunks.
