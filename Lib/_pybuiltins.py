"""Builtins implemented in Python.

This module is frozen into the interpreter and imported during startup,
before the import system exists.  The names listed in ``__all__`` are
copied into the ``builtins`` module.
"""

__all__ = []


# Used by the C implementation of anext() when a default is given.
async def _anext_with_default(awaitable, default):
    try:
        return await awaitable
    except StopAsyncIteration:
        return default
