Pending Removal in Future Versions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The following APIs are deprecated and will be removed,
although there is currently no date scheduled for their removal.

* :c:macro:`Py_TPFLAGS_HAVE_FINALIZE`: unneeded since Python 3.8.
* :c:func:`PyErr_Fetch`: use :c:func:`PyErr_GetRaisedException` instead.
* :c:func:`PyErr_NormalizeException`: use :c:func:`PyErr_GetRaisedException` instead.
* :c:func:`PyErr_Restore`: use :c:func:`PyErr_SetRaisedException` instead.
* :c:func:`PyModule_GetFilename`: use :c:func:`PyModule_GetFilenameObject` instead.
* :c:func:`PyOS_AfterFork`: use :c:func:`PyOS_AfterFork_Child` instead.
* :c:func:`PySlice_GetIndicesEx`: use :c:func:`PySlice_Unpack` and :c:func:`PySlice_AdjustIndices` instead.
* :c:func:`!PyUnicode_AsDecodedObject`: use :c:func:`PyCodec_Decode` instead.
* :c:func:`!PyUnicode_AsDecodedUnicode`: use :c:func:`PyCodec_Decode` instead.
* :c:func:`!PyUnicode_AsEncodedObject`: use :c:func:`PyCodec_Encode` instead.
* :c:func:`!PyUnicode_AsEncodedUnicode`: use :c:func:`PyCodec_Encode` instead.
* :c:func:`PyUnicode_READY`: unneeded since Python 3.12
* :c:func:`!PyErr_Display`: use :c:func:`PyErr_DisplayException` instead.
* :c:func:`!_PyErr_ChainExceptions`: use ``_PyErr_ChainExceptions1`` instead.
* :c:member:`!PyBytesObject.ob_shash` member:
  call :c:func:`PyObject_Hash` instead.
* :c:member:`!PyDictObject.ma_version_tag` member.
* Thread Local Storage (TLS) API:

  * :c:func:`PyThread_create_key`: use :c:func:`PyThread_tss_alloc` instead.
  * :c:func:`PyThread_delete_key`: use :c:func:`PyThread_tss_free` instead.
  * :c:func:`PyThread_set_key_value`: use :c:func:`PyThread_tss_set` instead.
  * :c:func:`PyThread_get_key_value`: use :c:func:`PyThread_tss_get` instead.
  * :c:func:`PyThread_delete_key_value`: use :c:func:`PyThread_tss_delete` instead.
  * :c:func:`PyThread_ReInitTLS`: unneeded since Python 3.7.
