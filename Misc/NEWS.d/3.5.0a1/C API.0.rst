- Issue #14203: Remove obsolete support for view==NULL in PyBuffer_FillInfo(),
  bytearray_getbuffer(), bytesiobuf_getbuffer() and array_buffer_getbuf().
  All functions now raise BufferError in that case.

- Issue #22445: PyBuffer_IsContiguous() now implements precise contiguity
  tests, compatible with NumPy's NPY_RELAXED_STRIDES_CHECKING compilation
  flag.  Previously the function reported false negatives for corner cases.

- Issue #22079: PyType_Ready() now checks that statically allocated type has
  no dynamically allocated bases.

- Issue #22453: Removed non-documented macro PyObject_REPR().

- Issue #18395: Rename ``_Py_char2wchar()`` to :c:func:`Py_DecodeLocale`,
  rename ``_Py_wchar2char()`` to :c:func:`Py_EncodeLocale`, and document
  these functions.

- Issue #21233: Add new C functions: PyMem_RawCalloc(), PyMem_Calloc(),
  PyObject_Calloc(), _PyObject_GC_Calloc(). bytes(int) is now using
  ``calloc()`` instead of ``malloc()`` for large objects which is faster and
  use less memory.

- Issue #20942: PyImport_ImportFrozenModuleObject() no longer sets __file__ to
  match what importlib does; this affects _frozen_importlib as well as any
  module loaded using imp.init_frozen().

