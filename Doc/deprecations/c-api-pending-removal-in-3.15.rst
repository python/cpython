Pending removal in Python 3.15
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* The :c:func:`!PyImport_ImportModuleNoBlock`:
  Use :c:func:`PyImport_ImportModule` instead.
* :c:func:`!PyWeakref_GetObject` and :c:func:`!PyWeakref_GET_OBJECT`:
  Use :c:func:`PyWeakref_GetRef` instead. The `pythoncapi-compat project
  <https://github.com/python/pythoncapi-compat/>`__ can be used to get
  :c:func:`PyWeakref_GetRef` on Python 3.12 and older.
* :c:func:`!PyUnicode_AsDecodedObject`:
  Use :c:func:`PyCodec_Decode` instead.
* :c:func:`!PyUnicode_AsDecodedUnicode`:
  Use :c:func:`PyCodec_Decode` instead; Note that some codecs (for example, "base64")
  may return a type other than :class:`str`, such as :class:`bytes`.
* :c:func:`!PyUnicode_AsEncodedObject`:
  Use :c:func:`PyCodec_Encode` instead.
* :c:func:`!PyUnicode_AsEncodedUnicode`:
  Use :c:func:`PyCodec_Encode` instead; Note that some codecs (for example, "base64")
  may return a type other than :class:`bytes`, such as :class:`str`.
* Python initialization functions, deprecated in Python 3.13:

  * :c:func:`!Py_GetPath`:
    Use :c:func:`PyConfig_Get("module_search_paths") <PyConfig_Get>`
    (:data:`sys.path`) instead.
  * :c:func:`!Py_GetPrefix`:
    Use :c:func:`PyConfig_Get("base_prefix") <PyConfig_Get>`
    (:data:`sys.base_prefix`) instead. Use :c:func:`PyConfig_Get("prefix")
    <PyConfig_Get>` (:data:`sys.prefix`) if :ref:`virtual environments
    <venv-def>` need to be handled.
  * :c:func:`!Py_GetExecPrefix`:
    Use :c:func:`PyConfig_Get("base_exec_prefix") <PyConfig_Get>`
    (:data:`sys.base_exec_prefix`) instead. Use
    :c:func:`PyConfig_Get("exec_prefix") <PyConfig_Get>`
    (:data:`sys.exec_prefix`) if :ref:`virtual environments <venv-def>` need to
    be handled.
  * :c:func:`!Py_GetProgramFullPath`:
    Use :c:func:`PyConfig_Get("executable") <PyConfig_Get>`
    (:data:`sys.executable`) instead.
  * :c:func:`!Py_GetProgramName`:
    Use :c:func:`PyConfig_Get("executable") <PyConfig_Get>`
    (:data:`sys.executable`) instead.
  * :c:func:`!Py_GetPythonHome`:
    Use :c:func:`PyConfig_Get("home") <PyConfig_Get>` or the
    :envvar:`PYTHONHOME` environment variable instead.

  The `pythoncapi-compat project
  <https://github.com/python/pythoncapi-compat/>`__ can be used to get
  :c:func:`PyConfig_Get` on Python 3.13 and older.
