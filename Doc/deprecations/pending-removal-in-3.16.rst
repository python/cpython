Pending removal in Python 3.16
------------------------------

* The import system:

  * Setting :attr:`~module.__loader__` on a module while
    failing to set :attr:`__spec__.loader <importlib.machinery.ModuleSpec.loader>`
    is deprecated. In Python 3.16, :attr:`!__loader__` will cease to be set or
    taken into consideration by the import system or the standard library.

  * Setting :attr:`~module.__package__` on a module while
    failing to set :attr:`__spec__.parent <importlib.machinery.ModuleSpec.parent>`
    is deprecated. In Python 3.16, :attr:`!__package__` will cease to be
    taken into consideration by the import system or standard library. (:gh:`97879`)

* :mod:`asyncio`:

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
