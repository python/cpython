"""Support for tasks, coroutines and the scheduler."""

__all__ = ['coroutine', 'Task',
           'iscoroutinefunction', 'iscoroutine',
           'FIRST_COMPLETED', 'FIRST_EXCEPTION', 'ALL_COMPLETED',
           'wait', 'wait_for', 'as_completed', 'sleep', 'async',
           'gather', 'shield',
           ]

import concurrent.futures
import functools
import inspect
import linecache
import os
import sys
import traceback
import weakref

from . import events
from . import futures
from .log import logger

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


class CoroWrapper:
    # Wrapper for coroutine in _DEBUG mode.

    __slots__ = ['gen', 'func', '__name__', '__doc__', '__weakref__']

    def __init__(self, gen, func):
        assert inspect.isgenerator(gen), gen
        self.gen = gen
        self.func = func

    def __iter__(self):
        return self

    def __next__(self):
        return next(self.gen)

    def send(self, *value):
        # We use `*value` because of a bug in CPythons prior
        # to 3.4.1. See issue #21209 and test_yield_from_corowrapper
        # for details.  This workaround should be removed in 3.5.0.
        if len(value) == 1:
            value = value[0]
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
            func = self.func
            code = func.__code__
            filename = code.co_filename
            lineno = code.co_firstlineno
            logger.error(
                'Coroutine %r defined at %s:%s was never yielded from',
                func.__name__, filename, lineno)


def coroutine(func):
    """Decorator to mark coroutines.

    If the coroutine is not yielded from before it is destroyed,
    an error message is logged.
    """
    if inspect.isgeneratorfunction(func):
        coro = func
    else:
        @functools.wraps(func)
        def coro(*args, **kw):
            res = func(*args, **kw)
            if isinstance(res, futures.Future) or inspect.isgenerator(res):
                res = yield from res
            return res

    if not _DEBUG:
        wrapper = coro
    else:
        @functools.wraps(func)
        def wrapper(*args, **kwds):
            w = CoroWrapper(coro(*args, **kwds), func)
            w.__name__ = coro.__name__
            w.__doc__ = coro.__doc__
            return w

    wrapper._is_coroutine = True  # For iscoroutinefunction().
    return wrapper


def iscoroutinefunction(func):
    """Return True if func is a decorated coroutine function."""
    return getattr(func, '_is_coroutine', False)


def iscoroutine(obj):
    """Return True if obj is a coroutine object."""
    return isinstance(obj, CoroWrapper) or inspect.isgenerator(obj)


class Task(futures.Future):
    """A coroutine wrapped in a Future."""

    # An important invariant maintained while a Task not done:
    #
    # - Either _fut_waiter is None, and _step() is scheduled;
    # - or _fut_waiter is some Future, and _step() is *not* scheduled.
    #
    # The only transition from the latter to the former is through
    # _wakeup().  When _fut_waiter is not None, one of its callbacks
    # must be _wakeup().

    # Weak set containing all tasks alive.
    _all_tasks = weakref.WeakSet()

    # Dictionary containing tasks that are currently active in
    # all running event loops.  {EventLoop: Task}
    _current_tasks = {}

    @classmethod
    def current_task(cls, loop=None):
        """Return the currently running task in an event loop or None.

        By default the current task for the current event loop is returned.

        None is returned when called not in the context of a Task.
        """
        if loop is None:
            loop = events.get_event_loop()
        return cls._current_tasks.get(loop)

    @classmethod
    def all_tasks(cls, loop=None):
        """Return a set of all tasks for an event loop.

        By default all tasks for the current event loop are returned.
        """
        if loop is None:
            loop = events.get_event_loop()
        return {t for t in cls._all_tasks if t._loop is loop}

    def __init__(self, coro, *, loop=None):
        assert iscoroutine(coro), repr(coro)  # Not a coroutine function!
        super().__init__(loop=loop)
        self._coro = iter(coro)  # Use the iterator just in case.
        self._fut_waiter = None
        self._must_cancel = False
        self._loop.call_soon(self._step)
        self.__class__._all_tasks.add(self)

    def __repr__(self):
        res = super().__repr__()
        if (self._must_cancel and
            self._state == futures._PENDING and
            '<PENDING' in res):
            res = res.replace('<PENDING', '<CANCELLING', 1)
        i = res.find('<')
        if i < 0:
            i = len(res)
        res = res[:i] + '(<{}>)'.format(self._coro.__name__) + res[i:]
        return res

    def get_stack(self, *, limit=None):
        """Return the list of stack frames for this task's coroutine.

        If the coroutine is active, this returns the stack where it is
        suspended.  If the coroutine has completed successfully or was
        cancelled, this returns an empty list.  If the coroutine was
        terminated by an exception, this returns the list of traceback
        frames.

        The frames are always ordered from oldest to newest.

        The optional limit gives the maximum number of frames to
        return; by default all available frames are returned.  Its
        meaning differs depending on whether a stack or a traceback is
        returned: the newest frames of a stack are returned, but the
        oldest frames of a traceback are returned.  (This matches the
        behavior of the traceback module.)

        For reasons beyond our control, only one stack frame is
        returned for a suspended coroutine.
        """
        frames = []
        f = self._coro.gi_frame
        if f is not None:
            while f is not None:
                if limit is not None:
                    if limit <= 0:
                        break
                    limit -= 1
                frames.append(f)
                f = f.f_back
            frames.reverse()
        elif self._exception is not None:
            tb = self._exception.__traceback__
            while tb is not None:
                if limit is not None:
                    if limit <= 0:
                        break
                    limit -= 1
                frames.append(tb.tb_frame)
                tb = tb.tb_next
        return frames

    def print_stack(self, *, limit=None, file=None):
        """Print the stack or traceback for this task's coroutine.

        This produces output similar to that of the traceback module,
        for the frames retrieved by get_stack().  The limit argument
        is passed to get_stack().  The file argument is an I/O stream
        to which the output goes; by default it goes to sys.stderr.
        """
        extracted_list = []
        checked = set()
        for f in self.get_stack(limit=limit):
            lineno = f.f_lineno
            co = f.f_code
            filename = co.co_filename
            name = co.co_name
            if filename not in checked:
                checked.add(filename)
                linecache.checkcache(filename)
            line = linecache.getline(filename, lineno, f.f_globals)
            extracted_list.append((filename, lineno, name, line))
        exc = self._exception
        if not extracted_list:
            print('No stack for %r' % self, file=file)
        elif exc is not None:
            print('Traceback for %r (most recent call last):' % self,
                  file=file)
        else:
            print('Stack for %r (most recent call last):' % self,
                  file=file)
        traceback.print_list(extracted_list, file=file)
        if exc is not None:
            for line in traceback.format_exception_only(exc.__class__, exc):
                print(line, file=file, end='')

    def cancel(self):
        """Request that a task to cancel itself.

        This arranges for a CancellationError to be thrown into the
        wrapped coroutine on the next cycle through the event loop.
        The coroutine then has a chance to clean up or even deny
        the request using try/except/finally.

        Contrary to Future.cancel(), this does not guarantee that the
        task will be cancelled: the exception might be caught and
        acted upon, delaying cancellation of the task or preventing it
        completely.  The task may also return a value or raise a
        different exception.

        Immediately after this method is called, Task.cancelled() will
        not return True (unless the task was already cancelled).  A
        task will be marked as cancelled when the wrapped coroutine
        terminates with a CancelledError exception (even if cancel()
        was not called).
        """
        if self.done():
            return False
        if self._fut_waiter is not None:
            if self._fut_waiter.cancel():
                # Leave self._fut_waiter; it may be a Task that
                # catches and ignores the cancellation so we may have
                # to cancel it again later.
                return True
        # It must be the case that self._step is already scheduled.
        self._must_cancel = True
        return True

    def _step(self, value=None, exc=None):
        assert not self.done(), \
            '_step(): already done: {!r}, {!r}, {!r}'.format(self, value, exc)
        if self._must_cancel:
            if not isinstance(exc, futures.CancelledError):
                exc = futures.CancelledError()
            self._must_cancel = False
        coro = self._coro
        self._fut_waiter = None

        self.__class__._current_tasks[self._loop] = self
        # Call either coro.throw(exc) or coro.send(value).
        try:
            if exc is not None:
                result = coro.throw(exc)
            elif value is not None:
                result = coro.send(value)
            else:
                result = next(coro)
        except StopIteration as exc:
            self.set_result(exc.value)
        except futures.CancelledError as exc:
            super().cancel()  # I.e., Future.cancel(self).
        except Exception as exc:
            self.set_exception(exc)
        except BaseException as exc:
            self.set_exception(exc)
            raise
        else:
            if isinstance(result, futures.Future):
                # Yielded Future must come from Future.__iter__().
                if result._blocking:
                    result._blocking = False
                    result.add_done_callback(self._wakeup)
                    self._fut_waiter = result
                    if self._must_cancel:
                        if self._fut_waiter.cancel():
                            self._must_cancel = False
                else:
                    self._loop.call_soon(
                        self._step, None,
                        RuntimeError(
                            'yield was used instead of yield from '
                            'in task {!r} with {!r}'.format(self, result)))
            elif result is None:
                # Bare yield relinquishes control for one event loop iteration.
                self._loop.call_soon(self._step)
            elif inspect.isgenerator(result):
                # Yielding a generator is just wrong.
                self._loop.call_soon(
                    self._step, None,
                    RuntimeError(
                        'yield was used instead of yield from for '
                        'generator in task {!r} with {}'.format(
                            self, result)))
            else:
                # Yielding something else is an error.
                self._loop.call_soon(
                    self._step, None,
                    RuntimeError(
                        'Task got bad yield: {!r}'.format(result)))
        finally:
            self.__class__._current_tasks.pop(self._loop)
            self = None  # Needed to break cycles when an exception occurs.

    def _wakeup(self, future):
        try:
            value = future.result()
        except Exception as exc:
            # This may also be a cancellation.
            self._step(None, exc)
        else:
            self._step(value, None)
        self = None  # Needed to break cycles when an exception occurs.


# wait() and as_completed() similar to those in PEP 3148.

FIRST_COMPLETED = concurrent.futures.FIRST_COMPLETED
FIRST_EXCEPTION = concurrent.futures.FIRST_EXCEPTION
ALL_COMPLETED = concurrent.futures.ALL_COMPLETED


@coroutine
def wait(fs, *, loop=None, timeout=None, return_when=ALL_COMPLETED):
    """Wait for the Futures and coroutines given by fs to complete.

    Coroutines will be wrapped in Tasks.

    Returns two sets of Future: (done, pending).

    Usage:

        done, pending = yield from asyncio.wait(fs)

    Note: This does not raise TimeoutError! Futures that aren't done
    when the timeout occurs are returned in the second set.
    """
    if isinstance(fs, futures.Future) or iscoroutine(fs):
        raise TypeError("expect a list of futures, not %s" % type(fs).__name__)
    if not fs:
        raise ValueError('Set of coroutines/Futures is empty.')

    if loop is None:
        loop = events.get_event_loop()

    fs = {async(f, loop=loop) for f in set(fs)}

    if return_when not in (FIRST_COMPLETED, FIRST_EXCEPTION, ALL_COMPLETED):
        raise ValueError('Invalid return_when value: {}'.format(return_when))
    return (yield from _wait(fs, timeout, return_when, loop))


def _release_waiter(waiter, value=True, *args):
    if not waiter.done():
        waiter.set_result(value)


@coroutine
def wait_for(fut, timeout, *, loop=None):
    """Wait for the single Future or coroutine to complete, with timeout.

    Coroutine will be wrapped in Task.

    Returns result of the Future or coroutine.  When a timeout occurs,
    it cancels the task and raises TimeoutError.  To avoid the task
    cancellation, wrap it in shield().

    Usage:

        result = yield from asyncio.wait_for(fut, 10.0)

    """
    if loop is None:
        loop = events.get_event_loop()

    if timeout is None:
        return (yield from fut)

    waiter = futures.Future(loop=loop)
    timeout_handle = loop.call_later(timeout, _release_waiter, waiter, False)
    cb = functools.partial(_release_waiter, waiter, True)

    fut = async(fut, loop=loop)
    fut.add_done_callback(cb)

    try:
        if (yield from waiter):
            return fut.result()
        else:
            fut.remove_done_callback(cb)
            fut.cancel()
            raise futures.TimeoutError()
    finally:
        timeout_handle.cancel()


@coroutine
def _wait(fs, timeout, return_when, loop):
    """Internal helper for wait() and _wait_for().

    The fs argument must be a collection of Futures.
    """
    assert fs, 'Set of Futures is empty.'
    waiter = futures.Future(loop=loop)
    timeout_handle = None
    if timeout is not None:
        timeout_handle = loop.call_later(timeout, _release_waiter, waiter)
    counter = len(fs)

    def _on_completion(f):
        nonlocal counter
        counter -= 1
        if (counter <= 0 or
            return_when == FIRST_COMPLETED or
            return_when == FIRST_EXCEPTION and (not f.cancelled() and
                                                f.exception() is not None)):
            if timeout_handle is not None:
                timeout_handle.cancel()
            if not waiter.done():
                waiter.set_result(False)

    for f in fs:
        f.add_done_callback(_on_completion)

    try:
        yield from waiter
    finally:
        if timeout_handle is not None:
            timeout_handle.cancel()

    done, pending = set(), set()
    for f in fs:
        f.remove_done_callback(_on_completion)
        if f.done():
            done.add(f)
        else:
            pending.add(f)
    return done, pending


# This is *not* a @coroutine!  It is just an iterator (yielding Futures).
def as_completed(fs, *, loop=None, timeout=None):
    """Return an iterator whose values are coroutines.

    When waiting for the yielded coroutines you'll get the results (or
    exceptions!) of the original Futures (or coroutines), in the order
    in which and as soon as they complete.

    This differs from PEP 3148; the proper way to use this is:

        for f in as_completed(fs):
            result = yield from f  # The 'yield from' may raise.
            # Use result.

    If a timeout is specified, the 'yield from' will raise
    TimeoutError when the timeout occurs before all Futures are done.

    Note: The futures 'f' are not necessarily members of fs.
    """
    if isinstance(fs, futures.Future) or iscoroutine(fs):
        raise TypeError("expect a list of futures, not %s" % type(fs).__name__)
    loop = loop if loop is not None else events.get_event_loop()
    todo = {async(f, loop=loop) for f in set(fs)}
    from .queues import Queue  # Import here to avoid circular import problem.
    done = Queue(loop=loop)
    timeout_handle = None

    def _on_timeout():
        for f in todo:
            f.remove_done_callback(_on_completion)
            done.put_nowait(None)  # Queue a dummy value for _wait_for_one().
        todo.clear()  # Can't do todo.remove(f) in the loop.

    def _on_completion(f):
        if not todo:
            return  # _on_timeout() was here first.
        todo.remove(f)
        done.put_nowait(f)
        if not todo and timeout_handle is not None:
            timeout_handle.cancel()

    @coroutine
    def _wait_for_one():
        f = yield from done.get()
        if f is None:
            # Dummy value from _on_timeout().
            raise futures.TimeoutError
        return f.result()  # May raise f.exception().

    for f in todo:
        f.add_done_callback(_on_completion)
    if todo and timeout is not None:
        timeout_handle = loop.call_later(timeout, _on_timeout)
    for _ in range(len(todo)):
        yield _wait_for_one()


@coroutine
def sleep(delay, result=None, *, loop=None):
    """Coroutine that completes after a given time (in seconds)."""
    future = futures.Future(loop=loop)
    h = future._loop.call_later(delay, future.set_result, result)
    try:
        return (yield from future)
    finally:
        h.cancel()


def async(coro_or_future, *, loop=None):
    """Wrap a coroutine in a future.

    If the argument is a Future, it is returned directly.
    """
    if isinstance(coro_or_future, futures.Future):
        if loop is not None and loop is not coro_or_future._loop:
            raise ValueError('loop argument must agree with Future')
        return coro_or_future
    elif iscoroutine(coro_or_future):
        return Task(coro_or_future, loop=loop)
    else:
        raise TypeError('A Future or coroutine is required')


class _GatheringFuture(futures.Future):
    """Helper for gather().

    This overrides cancel() to cancel all the children and act more
    like Task.cancel(), which doesn't immediately mark itself as
    cancelled.
    """

    def __init__(self, children, *, loop=None):
        super().__init__(loop=loop)
        self._children = children

    def cancel(self):
        if self.done():
            return False
        for child in self._children:
            child.cancel()
        return True


def gather(*coros_or_futures, loop=None, return_exceptions=False):
    """Return a future aggregating results from the given coroutines
    or futures.

    All futures must share the same event loop.  If all the tasks are
    done successfully, the returned future's result is the list of
    results (in the order of the original sequence, not necessarily
    the order of results arrival).  If *return_exceptions* is True,
    exceptions in the tasks are treated the same as successful
    results, and gathered in the result list; otherwise, the first
    raised exception will be immediately propagated to the returned
    future.

    Cancellation: if the outer Future is cancelled, all children (that
    have not completed yet) are also cancelled.  If any child is
    cancelled, this is treated as if it raised CancelledError --
    the outer Future is *not* cancelled in this case.  (This is to
    prevent the cancellation of one child to cause other children to
    be cancelled.)
    """
    arg_to_fut = {arg: async(arg, loop=loop) for arg in set(coros_or_futures)}
    children = [arg_to_fut[arg] for arg in coros_or_futures]
    n = len(children)
    if n == 0:
        outer = futures.Future(loop=loop)
        outer.set_result([])
        return outer
    if loop is None:
        loop = children[0]._loop
    for fut in children:
        if fut._loop is not loop:
            raise ValueError("futures are tied to different event loops")
    outer = _GatheringFuture(children, loop=loop)
    nfinished = 0
    results = [None] * n

    def _done_callback(i, fut):
        nonlocal nfinished
        if outer._state != futures._PENDING:
            if fut._exception is not None:
                # Mark exception retrieved.
                fut.exception()
            return
        if fut._state == futures._CANCELLED:
            res = futures.CancelledError()
            if not return_exceptions:
                outer.set_exception(res)
                return
        elif fut._exception is not None:
            res = fut.exception()  # Mark exception retrieved.
            if not return_exceptions:
                outer.set_exception(res)
                return
        else:
            res = fut._result
        results[i] = res
        nfinished += 1
        if nfinished == n:
            outer.set_result(results)

    for i, fut in enumerate(children):
        fut.add_done_callback(functools.partial(_done_callback, i))
    return outer


def shield(arg, *, loop=None):
    """Wait for a future, shielding it from cancellation.

    The statement

        res = yield from shield(something())

    is exactly equivalent to the statement

        res = yield from something()

    *except* that if the coroutine containing it is cancelled, the
    task running in something() is not cancelled.  From the POV of
    something(), the cancellation did not happen.  But its caller is
    still cancelled, so the yield-from expression still raises
    CancelledError.  Note: If something() is cancelled by other means
    this will still cancel shield().

    If you want to completely ignore cancellation (not recommended)
    you can combine shield() with a try/except clause, as follows:

        try:
            res = yield from shield(something())
        except CancelledError:
            res = None
    """
    inner = async(arg, loop=loop)
    if inner.done():
        # Shortcut.
        return inner
    loop = inner._loop
    outer = futures.Future(loop=loop)

    def _done_callback(inner):
        if outer.cancelled():
            # Mark inner's result as retrieved.
            inner.cancelled() or inner.exception()
            return
        if inner.cancelled():
            outer.cancel()
        else:
            exc = inner.exception()
            if exc is not None:
                outer.set_exception(exc)
            else:
                outer.set_result(inner.result())

    inner.add_done_callback(_done_callback)
    return outer
