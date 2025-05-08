Pending removal in future versions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The following APIs are deprecated and will be removed,
although there is currently no date scheduled for their removal.

* :c:macro:`Py_TPFLAGS_HAVE_FINALIZE`:
  Unneeded since Python 3.8.
* :c:func:`PyErr_Fetch`:
  Use :c:func:`PyErr_GetRaisedException` instead.
* :c:func:`PyErr_NormalizeException`:
  Use :c:func:`PyErr_GetRaisedException` instead.
* :c:func:`PyErr_Restore`:
  Use :c:func:`PyErr_SetRaisedException` instead.
* :c:func:`PyModule_GetFilename`:
  Use :c:func:`PyModule_GetFilenameObject` instead.
* :c:func:`PyOS_AfterFork`:
  Use :c:func:`PyOS_AfterFork_Child` instead.
* :c:func:`PySlice_GetIndicesEx`:
  Use :c:func:`PySlice_Unpack` and :c:func:`PySlice_AdjustIndices` instead.
* :c:func:`PyUnicode_READY`:
  Unneeded since Python 3.12
* :c:func:`!PyErr_Display`:
  Use :c:func:`PyErr_DisplayException` instead.
* :c:func:`!_PyErr_ChainExceptions`:
  Use :c:func:`!_PyErr_ChainExceptions1` instead.
* :c:member:`!PyBytesObject.ob_shash` member:
  call :c:func:`PyObject_Hash` instead.
* Thread Local Storage (TLS) API:

  * :c:func:`PyThread_create_key`:
    Use :c:func:`PyThread_tss_alloc` instead.
  * :c:func:`PyThread_delete_key`:
    Use :c:func:`PyThread_tss_free` instead.
  * :c:func:`PyThread_set_key_value`:
    Use :c:func:`PyThread_tss_set` instead.
  * :c:func:`PyThread_get_key_value`:
    Use :c:func:`PyThread_tss_get` instead.
  * :c:func:`PyThread_delete_key_value`:
    Use :c:func:`PyThread_tss_delete` instead.
  * :c:func:`PyThread_ReInitTLS`:
    Unneeded since Python 3.7.
