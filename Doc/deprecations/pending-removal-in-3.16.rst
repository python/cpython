Pending removal in Python 3.16
------------------------------

* The import system:

  * Setting :attr:`~module.__loader__` on a module while
    failing to set :attr:`__spec__.loader <importlib.machinery.ModuleSpec.loader>`
    is deprecated. In Python 3.16, :attr:`!__loader__` will cease to be set or
    taken into consideration by the import system or the standard library.

* :mod:`array`:

  * The ``'u'`` format code (:c:type:`wchar_t`)
    has been deprecated in documentation since Python 3.3
    and at runtime since Python 3.13.
    Use the ``'w'`` format code (:c:type:`Py_UCS4`)
    for Unicode characters instead.

* :mod:`asyncio`:

  * :func:`!asyncio.iscoroutinefunction` is deprecated
    and will be removed in Python 3.16,
    use :func:`inspect.iscoroutinefunction` instead.
    (Contributed by Jiahao Li and Kumar Aditya in :gh:`122875`.)

* :mod:`builtins`:

  * Bitwise inversion on boolean types, ``~True`` or ``~False``
    has been deprecated since Python 3.12,
    as it produces surprising and unintuitive results (``-2`` and ``-1``).
    Use ``not x`` instead for the logical negation of a Boolean.
    In the rare case that you need the bitwise inversion of
    the underlying integer, convert to ``int`` explicitly (``~int(x)``).

* :mod:`shutil`:

  * The :class:`!ExecError` exception
    has been deprecated since Python 3.14.
    It has not been used by any function in :mod:`!shutil` since Python 3.4,
    and is now an alias of :exc:`RuntimeError`.

* :mod:`symtable`:

  * The :meth:`Class.get_methods <symtable.Class.get_methods>` method
    has been deprecated since Python 3.14.

* :mod:`sys`:

  * The :func:`~sys._enablelegacywindowsfsencoding` function
    has been deprecated since Python 3.13.
    Use the :envvar:`PYTHONLEGACYWINDOWSFSENCODING` environment variable instead.

* :mod:`tarfile`:

  * The undocumented and unused :attr:`!TarFile.tarfile` attribute
    has been deprecated since Python 3.13.
