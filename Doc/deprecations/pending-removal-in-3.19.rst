Pending removal in Python 3.19
------------------------------

* :mod:`ctypes`:

  * :func:`~ctypes.DllGetClassObject` and :func:`~ctypes.DllCanUnloadNow`
    are deprecated and will be removed in Python 3.19.
    Their use for implementing in-process COM servers is unclear;
    discussion about replacements and use cases is welcome in :gh:`127369`.
