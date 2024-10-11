Pending Removal in Python 3.15
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* The bundled copy of ``libmpdecimal``.
* The :c:func:`PyImport_ImportModuleNoBlock`:
  Use :c:func:`PyImport_ImportModule` instead.
* :c:func:`PyWeakref_GetObject` and :c:func:`PyWeakref_GET_OBJECT`:
  Use :c:func:`PyWeakref_GetRef` instead.
* :c:type:`Py_UNICODE` type and the :c:macro:`!Py_UNICODE_WIDE` macro:
  Use :c:type:`wchar_t` instead.
* Python initialization functions:

  * :c:func:`PySys_ResetWarnOptions`:
    Clear :data:`sys.warnoptions` and :data:`!warnings.filters` instead.
  * :c:func:`Py_GetExecPrefix`:
    Get :data:`sys.base_exec_prefix` and :data:`sys.exec_prefix` instead.
  * :c:func:`Py_GetPath`:
    Get :data:`sys.path` instead.
  * :c:func:`Py_GetPrefix`:
    Get :data:`sys.base_prefix` and :data:`sys.prefix` instead.
  * :c:func:`Py_GetProgramFullPath`:
    Get :data:`sys.executable` instead.
  * :c:func:`Py_GetProgramName`:
    Get :data:`sys.executable` instead.
  * :c:func:`Py_GetPythonHome`:
    Get :c:member:`PyConfig.home`
    or the :envvar:`PYTHONHOME` environment variable instead.
