Pending removal in Python 3.19
------------------------------

* :mod:`ctypes`:

  * Implicitly switching to the MSVC-compatible struct layout by setting
    :attr:`~ctypes.Structure._pack_` but not :attr:`~ctypes.Structure._layout_`
    on non-Windows platforms.

* :mod:`hashlib`:

  - In hash function constructors such as :func:`~hashlib.new` or the
    direct hash-named constructors such as :func:`~hashlib.md5` and
    :func:`~hashlib.sha256`, their optional initial data parameter could
    also be passed a keyword argument named ``data=`` or ``string=`` in
    various :mod:`!hashlib` implementations.

    Support for the ``string`` keyword argument name is now deprecated
    and slated for removal in Python 3.19.

    Before Python 3.13, the ``string`` keyword parameter was not correctly
    supported depending on the backend implementation of hash functions.
    Prefer passing the initial data as a positional argument for maximum
    backwards compatibility.
