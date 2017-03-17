Issue #26558: The debug hooks on Python memory allocator
:c:func:`PyObject_Malloc` now detect when functions are called without
holding the GIL.