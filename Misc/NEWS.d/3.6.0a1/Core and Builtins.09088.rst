Issue #26564: On error, the debug hooks on Python memory allocators now use
the :mod:`tracemalloc` module to get the traceback where a memory block was
allocated.