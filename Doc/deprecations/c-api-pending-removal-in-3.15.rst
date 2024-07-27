Pending Removal in Python 3.15
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* :c:func:`PyImport_ImportModuleNoBlock`: use :c:func:`PyImport_ImportModule`
* :c:type:`!Py_UNICODE_WIDE` type: use :c:type:`wchar_t`
* :c:type:`Py_UNICODE` type: use :c:type:`wchar_t`
* Python initialization functions:

  * :c:func:`PySys_ResetWarnOptions`: clear :data:`sys.warnoptions` and
    :data:`!warnings.filters`
  * :c:func:`Py_GetExecPrefix`: get :data:`sys.exec_prefix`
  * :c:func:`Py_GetPath`: get :data:`sys.path`
  * :c:func:`Py_GetPrefix`: get :data:`sys.prefix`
  * :c:func:`Py_GetProgramFullPath`: get :data:`sys.executable`
  * :c:func:`Py_GetProgramName`: get :data:`sys.executable`
  * :c:func:`Py_GetPythonHome`: get :c:member:`PyConfig.home` or
    the :envvar:`PYTHONHOME` environment variable
