Issue #26249: Memory functions of the :c:func:`PyMem_Malloc` domain
(:c:data:`PYMEM_DOMAIN_MEM`) now use the :ref:`pymalloc allocator <pymalloc>`
rather than system :c:func:`malloc`. Applications calling
:c:func:`PyMem_Malloc` without holding the GIL can now crash: use
``PYTHONMALLOC=debug`` environment variable to validate the usage of memory
allocators in your application.