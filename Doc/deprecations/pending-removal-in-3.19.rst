Pending removal in Python 3.19
------------------------------

* :mod:`ctypes`:

  * Implicitly switching to the MSVC-compatible struct layout by setting
    :attr:`~ctypes.Structure._pack_` but not :attr:`~ctypes.Structure._layout_`
    on non-Windows platforms.
