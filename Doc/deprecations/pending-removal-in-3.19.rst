Pending removal in Python 3.19
------------------------------

* :mod:`ctypes`:

  * Implicitly switching to the MSVC-compatible struct layout by setting
    :attr:`Structure._pack_` but not :attr:`Structure._layout_`
    on non-Windows platforms.
