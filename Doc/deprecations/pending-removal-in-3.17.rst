Pending removal in Python 3.17
------------------------------

* :mod:`typing`:

  - Before Python 3.14, old-style unions were implemented using the private class
    ``typing._UnionGenericAlias``. This class is no longer needed for the implementation,
    but it has been retained for backward compatibility, with removal scheduled for Python
    3.17. Users should use documented introspection helpers like :func:`typing.get_origin`
    and :func:`typing.get_args` instead of relying on private implementation details.

* The ``__version__`` attribute has been deprecated in these standard library
  modules and will be removed in Python 3.17.
  Use :py:data:`sys.version_info` instead.

  - :mod:`argparse`
  - :mod:`csv`
  - :mod:`!ctypes.macholib`
  - :mod:`ipaddress`
  - :mod:`json`
  - :mod:`logging` (``__date__`` also deprecated)
  - :mod:`optparse`
  - :mod:`pickle`
  - :mod:`platform`
  - :mod:`re`
  - :mod:`socketserver`
  - :mod:`tabnanny`
  - :mod:`tkinter.font`
  - :mod:`tkinter.ttk`

  (Contributed by Hugo van Kemenade in :gh:`76007`.)
