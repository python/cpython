Pending removal in Python 3.14
------------------------------

* The import system:

  * Setting :attr:`~module.__loader__` on a module while
    failing to set :attr:`__spec__.loader <importlib.machinery.ModuleSpec.loader>`
    is deprecated. In Python 3.14, :attr:`!__loader__` will cease to be set or
    taken into consideration by the import system or the standard library.

* :mod:`asyncio`:

  * The :meth:`~asyncio.get_event_loop` method of the
    default event loop policy now emits a :exc:`DeprecationWarning` if there
    is no current event loop set and it decides to create one.
    (Contributed by Serhiy Storchaka and Guido van Rossum in :gh:`100160`.)

* :mod:`pkgutil`: :func:`~pkgutil.find_loader` and :func:`~pkgutil.get_loader`
  now raise :exc:`DeprecationWarning`;
  use :func:`importlib.util.find_spec` instead.
  (Contributed by Nikita Sobolev in :gh:`97850`.)

* :mod:`sqlite3`:

  * date and datetime adapter, date and timestamp converter:
    see the :mod:`sqlite3` documentation for suggested replacement recipes.

* :class:`types.CodeType`: Accessing :attr:`~codeobject.co_lnotab` was
  deprecated in :pep:`626`
  since 3.10 and was planned to be removed in 3.12,
  but it only got a proper :exc:`DeprecationWarning` in 3.12.
  May be removed in 3.14.
  (Contributed by Nikita Sobolev in :gh:`101866`.)
