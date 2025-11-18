Pending removal in Python 3.18
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* The following private functions are deprecated
  and planned for removal in Python 3.18:

  * :c:func:`!_PyBytes_Join`: use :c:func:`PyBytes_Join`.
  * :c:func:`!_PyDict_GetItemStringWithError`: use :c:func:`PyDict_GetItemStringRef`.
  * :c:func:`!_PyDict_Pop()`: use :c:func:`PyDict_Pop`.
  * :c:func:`!_PyLong_Sign()`: use :c:func:`PyLong_GetSign`.
  * :c:func:`!_PyLong_FromDigits` and :c:func:`!_PyLong_New`:
    use :c:func:`PyLongWriter_Create`.
  * :c:func:`!_PyThreadState_UncheckedGet`: use :c:func:`PyThreadState_GetUnchecked`.
  * :c:func:`!_PyUnicode_AsString`: use :c:func:`PyUnicode_AsUTF8`.
  * :c:func:`!_PyUnicodeWriter_Init`:
    replace ``_PyUnicodeWriter_Init(&writer)`` with
    :c:func:`writer = PyUnicodeWriter_Create(0) <PyUnicodeWriter_Create>`.
  * :c:func:`!_PyUnicodeWriter_Finish`:
    replace ``_PyUnicodeWriter_Finish(&writer)`` with
    :c:func:`PyUnicodeWriter_Finish(writer) <PyUnicodeWriter_Finish>`.
  * :c:func:`!_PyUnicodeWriter_Dealloc`:
    replace ``_PyUnicodeWriter_Dealloc(&writer)`` with
    :c:func:`PyUnicodeWriter_Discard(writer) <PyUnicodeWriter_Discard>`.
  * :c:func:`!_PyUnicodeWriter_WriteChar`:
    replace ``_PyUnicodeWriter_WriteChar(&writer, ch)`` with
    :c:func:`PyUnicodeWriter_WriteChar(writer, ch) <PyUnicodeWriter_WriteChar>`.
  * :c:func:`!_PyUnicodeWriter_WriteStr`:
    replace ``_PyUnicodeWriter_WriteStr(&writer, str)`` with
    :c:func:`PyUnicodeWriter_WriteStr(writer, str) <PyUnicodeWriter_WriteStr>`.
  * :c:func:`!_PyUnicodeWriter_WriteSubstring`:
    replace ``_PyUnicodeWriter_WriteSubstring(&writer, str, start, end)`` with
    :c:func:`PyUnicodeWriter_WriteSubstring(writer, str, start, end) <PyUnicodeWriter_WriteSubstring>`.
  * :c:func:`!_PyUnicodeWriter_WriteASCIIString`:
    replace ``_PyUnicodeWriter_WriteASCIIString(&writer, str)`` with
    :c:func:`PyUnicodeWriter_WriteASCII(writer, str) <PyUnicodeWriter_WriteASCII>`.
  * :c:func:`!_PyUnicodeWriter_WriteLatin1String`:
    replace ``_PyUnicodeWriter_WriteLatin1String(&writer, str)`` with
    :c:func:`PyUnicodeWriter_WriteUTF8(writer, str) <PyUnicodeWriter_WriteUTF8>`.
  * :c:func:`!_PyUnicodeWriter_Prepare`: (no replacement).
  * :c:func:`!_PyUnicodeWriter_PrepareKind`: (no replacement).
  * :c:func:`!_Py_HashPointer`: use :c:func:`Py_HashPointer`.
  * :c:func:`!_Py_fopen_obj`: use :c:func:`Py_fopen`.

  The `pythoncapi-compat project
  <https://github.com/python/pythoncapi-compat/>`__ can be used to get
  these new public functions on Python 3.13 and older.
  (Contributed by Victor Stinner in :gh:`128863`.)
