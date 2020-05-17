"""High-level support for working with threads in asyncio"""

import functools

from . import events


__all__ = "to_thread",


async def to_thread(func, /, *args, **kwargs):
    """Asynchronously run function *func* in a separate thread.

    Any *args and **kwargs supplied for this function are directly passed
    to *func*.

    Return an asyncio.Future which represents the eventual result of *func*.
    """
    loop = events.get_running_loop()
    func_call = functools.partial(func, *args, **kwargs)
    return await loop.run_in_executor(None, func_call)
