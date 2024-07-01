Pending Removal in Future Versions
----------------------------------

The following APIs were deprecated in earlier Python versions and will be
removed, although there is currently no date scheduled for their removal.

* :c:macro:`Py_TPFLAGS_HAVE_FINALIZE`: no needed since Python 3.8.
* :c:func:`PyErr_Fetch`: use :c:func:`PyErr_GetRaisedException`.
* :c:func:`PyErr_NormalizeException`: use :c:func:`PyErr_GetRaisedException`.
* :c:func:`PyErr_Restore`: use :c:func:`PyErr_SetRaisedException`.
* :c:func:`PyModule_GetFilename`: use :c:func:`PyModule_GetFilenameObject`.
* :c:func:`PyOS_AfterFork`: use :c:func:`PyOS_AfterFork_Child()`.
* :c:func:`PySlice_GetIndicesEx`.
* :c:func:`!PyUnicode_AsDecodedObject`.
* :c:func:`!PyUnicode_AsDecodedUnicode`.
* :c:func:`!PyUnicode_AsEncodedObject`.
* :c:func:`!PyUnicode_AsEncodedUnicode`.
* :c:func:`PyUnicode_READY`: not needed since Python 3.12.
* :c:func:`!_PyErr_ChainExceptions`.
* :c:member:`!PyBytesObject.ob_shash` member:
  call :c:func:`PyObject_Hash` instead.
* :c:member:`!PyDictObject.ma_version_tag` member.
* TLS API:

  * :c:func:`PyThread_create_key`: use :c:func:`PyThread_tss_alloc`.
  * :c:func:`PyThread_delete_key`: use :c:func:`PyThread_tss_free`.
  * :c:func:`PyThread_set_key_value`: use :c:func:`PyThread_tss_set`.
  * :c:func:`PyThread_get_key_value`: use :c:func:`PyThread_tss_get`.
  * :c:func:`PyThread_delete_key_value`: use :c:func:`PyThread_tss_delete`.
  * :c:func:`PyThread_ReInitTLS`: no longer needed.

* Remove undocumented ``PY_TIMEOUT_MAX`` constant from the limited C API.
  (Contributed by Victor Stinner in :gh:`110014`.)
