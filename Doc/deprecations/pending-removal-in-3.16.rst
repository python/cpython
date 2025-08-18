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
    and will be removed in Python 3.16;
    use :func:`inspect.iscoroutinefunction` instead.
    (Contributed by Jiahao Li and Kumar Aditya in :gh:`122875`.)

  * :mod:`asyncio` policy system is deprecated and will be removed in Python 3.16.
    In particular, the following classes and functions are deprecated:

    * :class:`asyncio.AbstractEventLoopPolicy`
    * :class:`asyncio.DefaultEventLoopPolicy`
    * :class:`asyncio.WindowsSelectorEventLoopPolicy`
    * :class:`asyncio.WindowsProactorEventLoopPolicy`
    * :func:`asyncio.get_event_loop_policy`
    * :func:`asyncio.set_event_loop_policy`

    Users should use :func:`asyncio.run` or :class:`asyncio.Runner` with
    *loop_factory* to use the desired event loop implementation.

    For example, to use :class:`asyncio.SelectorEventLoop` on Windows::

      import asyncio

      async def main():
          ...

      asyncio.run(main(), loop_factory=asyncio.SelectorEventLoop)

    (Contributed by Kumar Aditya in :gh:`127949`.)

* :mod:`builtins`:

  * Bitwise inversion on boolean types, ``~True`` or ``~False``
    has been deprecated since Python 3.12,
    as it produces surprising and unintuitive results (``-2`` and ``-1``).
    Use ``not x`` instead for the logical negation of a Boolean.
    In the rare case that you need the bitwise inversion of
    the underlying integer, convert to ``int`` explicitly (``~int(x)``).

* :mod:`functools`:

  * Calling the Python implementation of :func:`functools.reduce` with *function*
    or *sequence* as keyword arguments has been deprecated since Python 3.14.

* :mod:`logging`:

  Support for custom logging handlers with the *strm* argument is deprecated
  and scheduled for removal in Python 3.16. Define handlers with the *stream*
  argument instead. (Contributed by Mariusz Felisiak in :gh:`115032`.)

* :mod:`mimetypes`:

  * Valid extensions start with a '.' or are empty for
    :meth:`mimetypes.MimeTypes.add_type`.
    Undotted extensions are deprecated and will
    raise a :exc:`ValueError` in Python 3.16.
    (Contributed by Hugo van Kemenade in :gh:`75223`.)

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

* :mod:`sysconfig`:

  * The :func:`!sysconfig.expand_makefile_vars` function
    has been deprecated since Python 3.14.
    Use the ``vars`` argument of :func:`sysconfig.get_paths` instead.

* :mod:`tarfile`:

  * The undocumented and unused :attr:`!TarFile.tarfile` attribute
    has been deprecated since Python 3.13.
