Pending Removal in Python 3.15
------------------------------

* :class:`http.server.CGIHTTPRequestHandler` will be removed along with its
  related ``--cgi`` flag to ``python -m http.server``.  It was obsolete and
  rarely used.  No direct replacement exists.  *Anything* is better than CGI
  to interface a web server with a request handler.

* :class:`locale`: :func:`locale.getdefaultlocale` was deprecated in Python 3.11
  and originally planned for removal in Python 3.13 (:gh:`90817`),
  but removal has been postponed to Python 3.15.
  Use :func:`locale.setlocale()`, :func:`locale.getencoding()` and
  :func:`locale.getlocale()` instead.
  (Contributed by Hugo van Kemenade in :gh:`111187`.)

* :mod:`pathlib`:
  :meth:`pathlib.PurePath.is_reserved` is deprecated and scheduled for
  removal in Python 3.15. From Python 3.13 onwards, use ``os.path.isreserved``
  to detect reserved paths on Windows.

* :mod:`platform`:
  :func:`~platform.java_ver` is deprecated and will be removed in 3.15.
  It was largely untested, had a confusing API,
  and was only useful for Jython support.
  (Contributed by Nikita Sobolev in :gh:`116349`.)

* :mod:`threading`:
  Passing any arguments to :func:`threading.RLock` is now deprecated.
  C version allows any numbers of args and kwargs,
  but they are just ignored. Python version does not allow any arguments.
  All arguments will be removed from :func:`threading.RLock` in Python 3.15.
  (Contributed by Nikita Sobolev in :gh:`102029`.)

* :class:`typing.NamedTuple`:

  * The undocumented keyword argument syntax for creating :class:`!NamedTuple` classes
    (``NT = NamedTuple("NT", x=int)``) is deprecated, and will be disallowed in
    3.15. Use the class-based syntax or the functional syntax instead.

  * When using the functional syntax to create a :class:`!NamedTuple` class, failing to
    pass a value to the *fields* parameter (``NT = NamedTuple("NT")``) is
    deprecated. Passing ``None`` to the *fields* parameter
    (``NT = NamedTuple("NT", None)``) is also deprecated. Both will be
    disallowed in Python 3.15. To create a :class:`!NamedTuple` class with 0 fields, use
    ``class NT(NamedTuple): pass`` or ``NT = NamedTuple("NT", [])``.

* :class:`typing.TypedDict`: When using the functional syntax to create a
  :class:`!TypedDict` class, failing to pass a value to the *fields* parameter (``TD =
  TypedDict("TD")``) is deprecated. Passing ``None`` to the *fields* parameter
  (``TD = TypedDict("TD", None)``) is also deprecated. Both will be disallowed
  in Python 3.15. To create a :class:`!TypedDict` class with 0 fields, use ``class
  TD(TypedDict): pass`` or ``TD = TypedDict("TD", {})``.

* :mod:`wave`: Deprecate the ``getmark()``, ``setmark()`` and ``getmarkers()``
  methods of the :class:`wave.Wave_read` and :class:`wave.Wave_write` classes.
  They will be removed in Python 3.15.
  (Contributed by Victor Stinner in :gh:`105096`.)
