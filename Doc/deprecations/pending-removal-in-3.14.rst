Pending removal in Python 3.14
------------------------------

* :mod:`argparse`: The *type*, *choices*, and *metavar* parameters
  of :class:`!argparse.BooleanOptionalAction` are deprecated
  and will be removed in 3.14.
  (Contributed by Nikita Sobolev in :gh:`92248`.)

* :mod:`ast`: The following features have been deprecated in documentation
  since Python 3.8, now cause a :exc:`DeprecationWarning` to be emitted at
  runtime when they are accessed or used, and will be removed in Python 3.14:

  * :class:`!ast.Num`
  * :class:`!ast.Str`
  * :class:`!ast.Bytes`
  * :class:`!ast.NameConstant`
  * :class:`!ast.Ellipsis`

  Use :class:`ast.Constant` instead.
  (Contributed by Serhiy Storchaka in :gh:`90953`.)

* :mod:`asyncio`:

  * The child watcher classes :class:`!asyncio.MultiLoopChildWatcher`,
    :class:`!asyncio.FastChildWatcher`, :class:`!asyncio.AbstractChildWatcher`
    and :class:`!asyncio.SafeChildWatcher` are deprecated and
    will be removed in Python 3.14.
    (Contributed by Kumar Aditya in :gh:`94597`.)

  * :func:`!asyncio.set_child_watcher`, :func:`!asyncio.get_child_watcher`,
    :meth:`!asyncio.AbstractEventLoopPolicy.set_child_watcher` and
    :meth:`!asyncio.AbstractEventLoopPolicy.get_child_watcher` are deprecated
    and will be removed in Python 3.14.
    (Contributed by Kumar Aditya in :gh:`94597`.)

  * The :meth:`~asyncio.get_event_loop` method of the
    default event loop policy now emits a :exc:`DeprecationWarning` if there
    is no current event loop set and it decides to create one.
    (Contributed by Serhiy Storchaka and Guido van Rossum in :gh:`100160`.)

* :mod:`collections.abc`: Deprecated :class:`!collections.abc.ByteString`.
  Prefer :class:`!Sequence` or :class:`~collections.abc.Buffer`.
  For use in typing, prefer a union, like ``bytes | bytearray``,
  or :class:`collections.abc.Buffer`.
  (Contributed by Shantanu Jain in :gh:`91896`.)

* :mod:`email`: Deprecated the *isdst* parameter in :func:`email.utils.localtime`.
  (Contributed by Alan Williams in :gh:`72346`.)

* :mod:`importlib.abc` deprecated classes:

  * :class:`!importlib.abc.ResourceReader`
  * :class:`!importlib.abc.Traversable`
  * :class:`!importlib.abc.TraversableResources`

  Use :mod:`importlib.resources.abc` classes instead:

  * :class:`importlib.resources.abc.Traversable`
  * :class:`importlib.resources.abc.TraversableResources`

  (Contributed by Jason R. Coombs and Hugo van Kemenade in :gh:`93963`.)

* :mod:`itertools` had undocumented, inefficient, historically buggy,
  and inconsistent support for copy, deepcopy, and pickle operations.
  This will be removed in 3.14 for a significant reduction in code
  volume and maintenance burden.
  (Contributed by Raymond Hettinger in :gh:`101588`.)

* :mod:`multiprocessing`: The default start method will change to a safer one on
  Linux, BSDs, and other non-macOS POSIX platforms where ``'fork'`` is currently
  the default (:gh:`84559`). Adding a runtime warning about this was deemed too
  disruptive as the majority of code is not expected to care. Use the
  :func:`~multiprocessing.get_context` or
  :func:`~multiprocessing.set_start_method` APIs to explicitly specify when
  your code *requires* ``'fork'``.  See :ref:`multiprocessing-start-methods`.

* :mod:`pathlib`: :meth:`~pathlib.PurePath.is_relative_to` and
  :meth:`~pathlib.PurePath.relative_to`: passing additional arguments is
  deprecated.

* :mod:`pkgutil`: :func:`!pkgutil.find_loader` and :func:`!pkgutil.get_loader`
  now raise :exc:`DeprecationWarning`;
  use :func:`importlib.util.find_spec` instead.
  (Contributed by Nikita Sobolev in :gh:`97850`.)

* :mod:`pty`:

  * ``master_open()``: use :func:`pty.openpty`.
  * ``slave_open()``: use :func:`pty.openpty`.

* :mod:`sqlite3`:

  * :data:`!version` and :data:`!version_info`.

  * :meth:`~sqlite3.Cursor.execute` and :meth:`~sqlite3.Cursor.executemany`
    if :ref:`named placeholders <sqlite3-placeholders>` are used and
    *parameters* is a sequence instead of a :class:`dict`.

* :mod:`typing`: :class:`!typing.ByteString`, deprecated since Python 3.9,
  now causes a :exc:`DeprecationWarning` to be emitted when it is used.

* :mod:`urllib`:
  :class:`!urllib.parse.Quoter` is deprecated: it was not intended to be a
  public API.
  (Contributed by Gregory P. Smith in :gh:`88168`.)
