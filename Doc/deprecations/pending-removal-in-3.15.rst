Pending removal in Python 3.15
------------------------------

* The import system:

  * Setting :attr:`~module.__cached__` on a module while
    failing to set :attr:`__spec__.cached <importlib.machinery.ModuleSpec.cached>`
    is deprecated. In Python 3.15, :attr:`!__cached__` will cease to be set or
    take into consideration by the import system or standard library. (:gh:`97879`)

  * Setting :attr:`~module.__package__` on a module while
    failing to set :attr:`__spec__.parent <importlib.machinery.ModuleSpec.parent>`
    is deprecated. In Python 3.15, :attr:`!__package__` will cease to be set or
    take into consideration by the import system or standard library. (:gh:`97879`)

* :mod:`ctypes`:

  * The undocumented :func:`!ctypes.SetPointerType` function
    has been deprecated since Python 3.13.

* :mod:`http.server`:

  * The obsolete and rarely used :class:`!CGIHTTPRequestHandler`
    has been deprecated since Python 3.13.
    No direct replacement exists.
    *Anything* is better than CGI to interface
    a web server with a request handler.

  * The :option:`!--cgi` flag to the :program:`python -m http.server`
    command-line interface has been deprecated since Python 3.13.

* :mod:`importlib`:

  * ``load_module()`` method: use ``exec_module()`` instead.

* :class:`locale`:

  * The :func:`~locale.getdefaultlocale` function
    has been deprecated since Python 3.11.
    Its removal was originally planned for Python 3.13 (:gh:`90817`),
    but has been postponed to Python 3.15.
    Use :func:`~locale.getlocale`, :func:`~locale.setlocale`,
    and :func:`~locale.getencoding` instead.
    (Contributed by Hugo van Kemenade in :gh:`111187`.)

* :mod:`pathlib`:

  * :meth:`.PurePath.is_reserved`
    has been deprecated since Python 3.13.
    Use :func:`os.path.isreserved` to detect reserved paths on Windows.

* :mod:`platform`:

  * :func:`!platform.java_ver` has been deprecated since Python 3.13.
    This function is only useful for Jython support, has a confusing API,
    and is largely untested.

* :mod:`sysconfig`:

  * The *check_home* argument of :func:`sysconfig.is_python_build` has been
    deprecated since Python 3.12.

* :mod:`threading`:

  * :func:`~threading.RLock` will take no arguments in Python 3.15.
    Passing any arguments has been deprecated since Python 3.14,
    as the  Python version does not permit any arguments,
    but the C version allows any number of positional or keyword arguments,
    ignoring every argument.

* :mod:`types`:

  * :class:`types.CodeType`: Accessing :attr:`~codeobject.co_lnotab` was
    deprecated in :pep:`626`
    since 3.10 and was planned to be removed in 3.12,
    but it only got a proper :exc:`DeprecationWarning` in 3.12.
    May be removed in 3.15.
    (Contributed by Nikita Sobolev in :gh:`101866`.)

* :mod:`typing`:

  * The undocumented keyword argument syntax for creating
    :class:`~typing.NamedTuple` classes
    (for example, ``Point = NamedTuple("Point", x=int, y=int)``)
    has been deprecated since Python 3.13.
    Use the class-based syntax or the functional syntax instead.

  * When using the functional syntax of :class:`~typing.TypedDict`\s, failing
    to pass a value to the *fields* parameter (``TD = TypedDict("TD")``) or
    passing ``None`` (``TD = TypedDict("TD", None)``) has been deprecated
    since Python 3.13.
    Use ``class TD(TypedDict): pass`` or ``TD = TypedDict("TD", {})``
    to create a TypedDict with zero field.

  * The :func:`typing.no_type_check_decorator` decorator function
    has been deprecated since Python 3.13.
    After eight years in the :mod:`typing` module,
    it has yet to be supported by any major type checker.

* :mod:`wave`:

  * The ``getmark()``, ``setmark()`` and ``getmarkers()`` methods of
    the :class:`~wave.Wave_read` and :class:`~wave.Wave_write` classes
    have been deprecated since Python 3.13.

* :mod:`zipimport`:

  * :meth:`~zipimport.zipimporter.load_module` has been deprecated since
    Python 3.10. Use :meth:`~zipimport.zipimporter.exec_module` instead.
    (Contributed by Jiahao Li in :gh:`125746`.)
