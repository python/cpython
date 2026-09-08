import inspect
import types
import unittest
import contextlib

from test.support.import_helper import import_module
from test.support import gc_collect, requires_working_socket
asyncio = import_module("asyncio")


requires_working_socket(module=True)

_no_default = object()


class AwaitException(Exception):
    pass


@types.coroutine
def awaitable(*, throw=False):
    if throw:
        yield ('throw',)
    else:
        yield ('result',)


def run_until_complete(coro):
    exc = False
    while True:
        try:
            if exc:
                exc = False
                fut = coro.throw(AwaitException)
            else:
                fut = coro.send(None)
        except StopIteration as ex:
            return ex.args[0]

        if fut == ('throw',):
            exc = True


def to_list(gen):
    async def iterate():
        res = []
        async for i in gen:
            res.append(i)
        return res

    return run_until_complete(iterate())


def py_anext(iterator, default=_no_default):
    """Pure-Python implementation of anext() for testing purposes.

    Closely matches the builtin anext() C implementation.
    Can be used to compare the built-in implementation of the inner
    coroutines machinery to C-implementation of __anext__() and send()
    or throw() on the returned generator.
    """

    try:
        __anext__ = type(iterator).__anext__
    except AttributeError:
        raise TypeError(f'{iterator!r} is not an async iterator')

    if default is _no_default:
        return __anext__(iterator)

    async def anext_impl():
        try:
            # The C code is way more low-level than this, as it implements
            # all methods of the iterator protocol. In this implementation
            # we're relying on higher-level coroutine concepts, but that's
            # exactly what we want -- crosstest pure-Python high-level
            # implementation and low-level C anext() iterators.
            return await __anext__(iterator)
        except StopAsyncIteration:
            return default

    return anext_impl()
