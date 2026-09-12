"""Builtins implemented in Python.

This module is frozen into the interpreter and imported during startup,
before the import system exists.  The names listed in ``__all__`` are
copied into the ``builtins`` module.
"""

__all__ = ['anext']

_NOT_GIVEN = object()


def anext(async_iterator, default=_NOT_GIVEN, /):
    """Return the next item from the async iterator.

    If default is given and the async iterator is exhausted,
    it is returned instead of raising StopAsyncIteration.
    """
    cls = type(async_iterator)
    try:
        # Looked up on the type, like the C slot am_anext.
        anext_method = cls.__anext__
    except AttributeError:
        raise TypeError(
            f"'{cls.__name__}' object is not an async iterator"
        ) from None
    awaitable = anext_method(async_iterator)
    if default is _NOT_GIVEN:
        return awaitable
    return _anext_with_default(awaitable, default)


async def _anext_with_default(awaitable, default):
    try:
        return await awaitable
    except StopAsyncIteration:
        return default


for _name in __all__:
    globals()[_name].__module__ = 'builtins'
del _name
