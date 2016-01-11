__all__ = ['coroutine',
           'iscoroutinefunction', 'iscoroutine']

import functools
import inspect
import opcode
import os
import sys
import traceback
import types

from . import compat
from . import events
from . import futures
from .log import logger


# Opcode of "yield from" instruction
_YIELD_FROM = opcode.opmap['YIELD_FROM']

# If you set _DEBUG to true, @coroutine will wrap the resulting
# generator objects in a CoroWrapper instance (defined below).  That
# instance will log a message when the generator is never iterated
# over, which may happen when you forget to use "yield from" with a
# coroutine call.  Note that the value of the _DEBUG flag is taken
# when the decorator is used, so to be of any use it must be set
# before you define your coroutines.  A downside of using this feature
# is that tracebacks show entries for the CoroWrapper.__next__ method
# when _DEBUG is true.
_DEBUG = (not sys.flags.ignore_environment and
          bool(os.environ.get('PYTHONASYNCIODEBUG')))


try:
    _types_coroutine = types.coroutine
except AttributeError:
    _types_coroutine = None

try:
    _inspect_iscoroutinefunction = inspect.iscoroutinefunction
except AttributeError:
    _inspect_iscoroutinefunction = lambda func: False

try:
    from collections.abc import Coroutine as _CoroutineABC, \
                                Awaitable as _AwaitableABC
except ImportError:
    _CoroutineABC = _AwaitableABC = None


# Check for CPython issue #21209
def has_yield_from_bug():
    class MyGen:
        def __init__(self):
            self.send_args = None
        def __iter__(self):
            return self
        def __next__(self):
            return 42
        def send(self, *what):
            self.send_args = what
            return None
    def yield_from_gen(gen):
        yield from gen
    value = (1, 2, 3)
    gen = MyGen()
    coro = yield_from_gen(gen)
    next(coro)
    coro.send(value)
    return gen.send_args != (value,)
_YIELD_FROM_BUG = has_yield_from_bug()
del has_yield_from_bug


def debug_wrapper(gen):
    # This function is called from 'sys.set_coroutine_wrapper'.
    # We only wrap here coroutines defined via 'async def' syntax.
    # Generator-based coroutines are wrapped in @coroutine
    # decorator.
    return CoroWrapper(gen, None)


class CoroWrapper:
    # Wrapper for coroutine object in _DEBUG mode.

    def __init__(self, gen, func=None):
        assert inspect.isgenerator(gen) or inspect.iscoroutine(gen), gen
        self.gen = gen
        self.func = func  # Used to unwrap @coroutine decorator
        self._source_traceback = traceback.extract_stack(sys._getframe(1))
        self.__name__ = getattr(gen, '__name__', None)
        self.__qualname__ = getattr(gen, '__qualname__', None)

    def __repr__(self):
        coro_repr = _format_coroutine(self)
        if self._source_traceback:
            frame = self._source_traceback[-1]
            coro_repr += ', created at %s:%s' % (frame[0], frame[1])
        return '<%s %s>' % (self.__class__.__name__, coro_repr)

    def __iter__(self):
        return self

    def __next__(self):
        return self.gen.send(None)

    if _YIELD_FROM_BUG:
        # For for CPython issue #21209: using "yield from" and a custom
        # generator, generator.send(tuple) unpacks the tuple instead of passing
        # the tuple unchanged. Check if the caller is a generator using "yield
        # from" to decide if the parameter should be unpacked or not.
        def send(self, *value):
            frame = sys._getframe()
            caller = frame.f_back
            assert caller.f_lasti >= 0
            if caller.f_code.co_code[caller.f_lasti] != _YIELD_FROM:
                value = value[0]
            return self.gen.send(value)
    else:
        def send(self, value):
            return self.gen.send(value)

    def throw(self, exc):
        return self.gen.throw(exc)

    def close(self):
        return self.gen.close()

    @property
    def gi_frame(self):
        return self.gen.gi_frame

    @property
    def gi_running(self):
        return self.gen.gi_running

    @property
    def gi_code(self):
        return self.gen.gi_code

    if compat.PY35:

        def __await__(self):
            cr_await = getattr(self.gen, 'cr_await', None)
            if cr_await is not None:
                raise RuntimeError(
                    "Cannot await on coroutine {!r} while it's "
                    "awaiting for {!r}".format(self.gen, cr_await))
            return self

        @property
        def gi_yieldfrom(self):
            return self.gen.gi_yieldfrom

        @property
        def cr_await(self):
            return self.gen.cr_await

        @property
        def cr_running(self):
            return self.gen.cr_running

        @property
        def cr_code(self):
            return self.gen.cr_code

        @property
        def cr_frame(self):
            return self.gen.cr_frame

    def __del__(self):
        # Be careful accessing self.gen.frame -- self.gen might not exist.
        gen = getattr(self, 'gen', None)
        frame = getattr(gen, 'gi_frame', None)
        if frame is None:
            frame = getattr(gen, 'cr_frame', None)
        if frame is not None and frame.f_lasti == -1:
            msg = '%r was never yielded from' % self
            tb = getattr(self, '_source_traceback', ())
            if tb:
                tb = ''.join(traceback.format_list(tb))
                msg += ('\nCoroutine object created at '
                        '(most recent call last):\n')
                msg += tb.rstrip()
            logger.error(msg)


def coroutine(func):
    """Decorator to mark coroutines.

    If the coroutine is not yielded from before it is destroyed,
    an error message is logged.
    """
    if _inspect_iscoroutinefunction(func):
        # In Python 3.5 that's all we need to do for coroutines
        # defiend with "async def".
        # Wrapping in CoroWrapper will happen via
        # 'sys.set_coroutine_wrapper' function.
        return func

    if inspect.isgeneratorfunction(func):
        coro = func
    else:
        @functools.wraps(func)
        def coro(*args, **kw):
            res = func(*args, **kw)
            if isinstance(res, futures.Future) or inspect.isgenerator(res):
                res = yield from res
            elif _AwaitableABC is not None:
                # If 'func' returns an Awaitable (new in 3.5) we
                # want to run it.
                try:
                    await_meth = res.__await__
                except AttributeError:
                    pass
                else:
                    if isinstance(res, _AwaitableABC):
                        res = yield from await_meth()
            return res

    if not _DEBUG:
        if _types_coroutine is None:
            wrapper = coro
        else:
            wrapper = _types_coroutine(coro)
    else:
        @functools.wraps(func)
        def wrapper(*args, **kwds):
            w = CoroWrapper(coro(*args, **kwds), func=func)
            if w._source_traceback:
                del w._source_traceback[-1]
            # Python < 3.5 does not implement __qualname__
            # on generator objects, so we set it manually.
            # We use getattr as some callables (such as
            # functools.partial may lack __qualname__).
            w.__name__ = getattr(func, '__name__', None)
            w.__qualname__ = getattr(func, '__qualname__', None)
            return w

    wrapper._is_coroutine = True  # For iscoroutinefunction().
    return wrapper


def iscoroutinefunction(func):
    """Return True if func is a decorated coroutine function."""
    return (getattr(func, '_is_coroutine', False) or
            _inspect_iscoroutinefunction(func))


_COROUTINE_TYPES = (types.GeneratorType, CoroWrapper)
if _CoroutineABC is not None:
    _COROUTINE_TYPES += (_CoroutineABC,)


def iscoroutine(obj):
    """Return True if obj is a coroutine object."""
    return isinstance(obj, _COROUTINE_TYPES)


def _format_coroutine(coro):
    assert iscoroutine(coro)

    coro_name = None
    if isinstance(coro, CoroWrapper):
        func = coro.func
        coro_name = coro.__qualname__
        if coro_name is not None:
            coro_name = '{}()'.format(coro_name)
    else:
        func = coro

    if coro_name is None:
        coro_name = events._format_callback(func, ())

    try:
        coro_code = coro.gi_code
    except AttributeError:
        coro_code = coro.cr_code

    try:
        coro_frame = coro.gi_frame
    except AttributeError:
        coro_frame = coro.cr_frame

    filename = coro_code.co_filename
    lineno = 0
    if (isinstance(coro, CoroWrapper) and
            not inspect.isgeneratorfunction(coro.func) and
            coro.func is not None):
        source = events._get_function_source(coro.func)
        if source is not None:
            filename, lineno = source
        if coro_frame is None:
            coro_repr = ('%s done, defined at %s:%s'
                         % (coro_name, filename, lineno))
        else:
            coro_repr = ('%s running, defined at %s:%s'
                         % (coro_name, filename, lineno))
    elif coro_frame is not None:
        lineno = coro_frame.f_lineno
        coro_repr = ('%s running at %s:%s'
                     % (coro_name, filename, lineno))
    else:
        lineno = coro_code.co_firstlineno
        coro_repr = ('%s done, defined at %s:%s'
                     % (coro_name, filename, lineno))

    return coro_repr
