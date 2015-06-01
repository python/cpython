__all__ = ['coroutine',
           'iscoroutinefunction', 'iscoroutine']

import functools
import inspect
import opcode
import os
import sys
import traceback
import types

from . import events
from . import futures
from .log import logger


_PY35 = sys.version_info >= (3, 5)


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
_DEBUG = (not sys.flags.ignore_environment
          and bool(os.environ.get('PYTHONASYNCIODEBUG')))


try:
    types.coroutine
except AttributeError:
    native_coroutine_support = False
else:
    native_coroutine_support = True

try:
    _iscoroutinefunction = inspect.iscoroutinefunction
except AttributeError:
    _iscoroutinefunction = lambda func: False

try:
    inspect.CO_COROUTINE
except AttributeError:
    _is_native_coro_code = lambda code: False
else:
    _is_native_coro_code = lambda code: (code.co_flags &
                                         inspect.CO_COROUTINE)

try:
    from collections.abc import Coroutine as CoroutineABC, \
                                Awaitable as AwaitableABC
except ImportError:
    CoroutineABC = AwaitableABC = None


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
    if _is_native_coro_code(gen.gi_code):
        return CoroWrapper(gen, None)
    else:
        return gen


class CoroWrapper:
    # Wrapper for coroutine object in _DEBUG mode.

    def __init__(self, gen, func=None):
        assert inspect.isgenerator(gen) or inspect.iscoroutine(gen), gen
        self.gen = gen
        self.func = func # Used to unwrap @coroutine decorator
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

    if _PY35:
        __await__ = __iter__ # make compatible with 'await' expression

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

    def __del__(self):
        # Be careful accessing self.gen.frame -- self.gen might not exist.
        gen = getattr(self, 'gen', None)
        frame = getattr(gen, 'gi_frame', None)
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
    is_coroutine = _iscoroutinefunction(func)
    if is_coroutine and _is_native_coro_code(func.__code__):
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
            elif AwaitableABC is not None:
                # If 'func' returns an Awaitable (new in 3.5) we
                # want to run it.
                try:
                    await_meth = res.__await__
                except AttributeError:
                    pass
                else:
                    if isinstance(res, AwaitableABC):
                        res = yield from await_meth()
            return res

    if not _DEBUG:
        if native_coroutine_support:
            wrapper = types.coroutine(coro)
        else:
            wrapper = coro
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
            _iscoroutinefunction(func))


_COROUTINE_TYPES = (types.GeneratorType, CoroWrapper)
if CoroutineABC is not None:
    _COROUTINE_TYPES += (CoroutineABC,)


def iscoroutine(obj):
    """Return True if obj is a coroutine object."""
    return isinstance(obj, _COROUTINE_TYPES)


def _format_coroutine(coro):
    assert iscoroutine(coro)

    if isinstance(coro, CoroWrapper):
        func = coro.func
    else:
        func = coro
    coro_name = events._format_callback(func, ())

    filename = coro.gi_code.co_filename
    if (isinstance(coro, CoroWrapper)
    and not inspect.isgeneratorfunction(coro.func)):
        filename, lineno = events._get_function_source(coro.func)
        if coro.gi_frame is None:
            coro_repr = ('%s done, defined at %s:%s'
                         % (coro_name, filename, lineno))
        else:
            coro_repr = ('%s running, defined at %s:%s'
                         % (coro_name, filename, lineno))
    elif coro.gi_frame is not None:
        lineno = coro.gi_frame.f_lineno
        coro_repr = ('%s running at %s:%s'
                     % (coro_name, filename, lineno))
    else:
        lineno = coro.gi_code.co_firstlineno
        coro_repr = ('%s done, defined at %s:%s'
                     % (coro_name, filename, lineno))

    return coro_repr
