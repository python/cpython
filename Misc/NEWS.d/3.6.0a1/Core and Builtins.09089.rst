Issue #26563: Debug hooks on Python memory allocators now raise a fatal
error if functions of the :c:func:`PyMem_Malloc` family are called without
holding the GIL.