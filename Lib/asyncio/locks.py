"""Synchronization primitives."""

__all__ = ['Lock', 'Event', 'Condition', 'Semaphore']

import collections

from . import events
from . import futures
from . import tasks


class Lock:
    """Primitive lock objects.

    A primitive lock is a synchronization primitive that is not owned
    by a particular coroutine when locked.  A primitive lock is in one
    of two states, 'locked' or 'unlocked'.

    It is created in the unlocked state.  It has two basic methods,
    acquire() and release().  When the state is unlocked, acquire()
    changes the state to locked and returns immediately.  When the
    state is locked, acquire() blocks until a call to release() in
    another coroutine changes it to unlocked, then the acquire() call
    resets it to locked and returns.  The release() method should only
    be called in the locked state; it changes the state to unlocked
    and returns immediately.  If an attempt is made to release an
    unlocked lock, a RuntimeError will be raised.

    When more than one coroutine is blocked in acquire() waiting for
    the state to turn to unlocked, only one coroutine proceeds when a
    release() call resets the state to unlocked; first coroutine which
    is blocked in acquire() is being processed.

    acquire() is a coroutine and should be called with 'yield from'.

    Locks also support the context manager protocol.  '(yield from lock)'
    should be used as context manager expression.

    Usage:

        lock = Lock()
        ...
        yield from lock
        try:
            ...
        finally:
            lock.release()

    Context manager usage:

        lock = Lock()
        ...
        with (yield from lock):
             ...

    Lock objects can be tested for locking state:

        if not lock.locked():
           yield from lock
        else:
           # lock is acquired
           ...

    """

    def __init__(self, *, loop=None):
        self._waiters = collections.deque()
        self._locked = False
        if loop is not None:
            self._loop = loop
        else:
            self._loop = events.get_event_loop()

    def __repr__(self):
        res = super().__repr__()
        extra = 'locked' if self._locked else 'unlocked'
        if self._waiters:
            extra = '{},waiters:{}'.format(extra, len(self._waiters))
        return '<{} [{}]>'.format(res[1:-1], extra)

    def locked(self):
        """Return true if lock is acquired."""
        return self._locked

    @tasks.coroutine
    def acquire(self):
        """Acquire a lock.

        This method blocks until the lock is unlocked, then sets it to
        locked and returns True.
        """
        if not self._waiters and not self._locked:
            self._locked = True
            return True

        fut = futures.Future(loop=self._loop)
        self._waiters.append(fut)
        try:
            yield from fut
            self._locked = True
            return True
        finally:
            self._waiters.remove(fut)

    def release(self):
        """Release a lock.

        When the lock is locked, reset it to unlocked, and return.
        If any other coroutines are blocked waiting for the lock to become
        unlocked, allow exactly one of them to proceed.

        When invoked on an unlocked lock, a RuntimeError is raised.

        There is no return value.
        """
        if self._locked:
            self._locked = False
            # Wake up the first waiter who isn't cancelled.
            for fut in self._waiters:
                if not fut.done():
                    fut.set_result(True)
                    break
        else:
            raise RuntimeError('Lock is not acquired.')

    def __enter__(self):
        if not self._locked:
            raise RuntimeError(
                '"yield from" should be used as context manager expression')
        return True

    def __exit__(self, *args):
        self.release()

    def __iter__(self):
        yield from self.acquire()
        return self


class Event:
    """An Event implementation, our equivalent to threading.Event.

    Class implementing event objects. An event manages a flag that can be set
    to true with the set() method and reset to false with the clear() method.
    The wait() method blocks until the flag is true. The flag is initially
    false.
    """

    def __init__(self, *, loop=None):
        self._waiters = collections.deque()
        self._value = False
        if loop is not None:
            self._loop = loop
        else:
            self._loop = events.get_event_loop()

    def __repr__(self):
        res = super().__repr__()
        extra = 'set' if self._value else 'unset'
        if self._waiters:
            extra = '{},waiters:{}'.format(extra, len(self._waiters))
        return '<{} [{}]>'.format(res[1:-1], extra)

    def is_set(self):
        """Return true if and only if the internal flag is true."""
        return self._value

    def set(self):
        """Set the internal flag to true. All coroutines waiting for it to
        become true are awakened. Coroutine that call wait() once the flag is
        true will not block at all.
        """
        if not self._value:
            self._value = True

            for fut in self._waiters:
                if not fut.done():
                    fut.set_result(True)

    def clear(self):
        """Reset the internal flag to false. Subsequently, coroutines calling
        wait() will block until set() is called to set the internal flag
        to true again."""
        self._value = False

    @tasks.coroutine
    def wait(self):
        """Block until the internal flag is true.

        If the internal flag is true on entry, return True
        immediately.  Otherwise, block until another coroutine calls
        set() to set the flag to true, then return True.
        """
        if self._value:
            return True

        fut = futures.Future(loop=self._loop)
        self._waiters.append(fut)
        try:
            yield from fut
            return True
        finally:
            self._waiters.remove(fut)


class Condition:
    """A Condition implementation, our equivalent to threading.Condition.

    This class implements condition variable objects. A condition variable
    allows one or more coroutines to wait until they are notified by another
    coroutine.

    A new Lock object is created and used as the underlying lock.
    """

    def __init__(self, *, loop=None):
        if loop is not None:
            self._loop = loop
        else:
            self._loop = events.get_event_loop()

        # Lock as an attribute as in threading.Condition.
        lock = Lock(loop=self._loop)
        self._lock = lock
        # Export the lock's locked(), acquire() and release() methods.
        self.locked = lock.locked
        self.acquire = lock.acquire
        self.release = lock.release

        self._waiters = collections.deque()

    def __repr__(self):
        res = super().__repr__()
        extra = 'locked' if self.locked() else 'unlocked'
        if self._waiters:
            extra = '{},waiters:{}'.format(extra, len(self._waiters))
        return '<{} [{}]>'.format(res[1:-1], extra)

    @tasks.coroutine
    def wait(self):
        """Wait until notified.

        If the calling coroutine has not acquired the lock when this
        method is called, a RuntimeError is raised.

        This method releases the underlying lock, and then blocks
        until it is awakened by a notify() or notify_all() call for
        the same condition variable in another coroutine.  Once
        awakened, it re-acquires the lock and returns True.
        """
        if not self.locked():
            raise RuntimeError('cannot wait on un-acquired lock')

        keep_lock = True
        self.release()
        try:
            fut = futures.Future(loop=self._loop)
            self._waiters.append(fut)
            try:
                yield from fut
                return True
            finally:
                self._waiters.remove(fut)

        except GeneratorExit:
            keep_lock = False  # Prevent yield in finally clause.
            raise
        finally:
            if keep_lock:
                yield from self.acquire()

    @tasks.coroutine
    def wait_for(self, predicate):
        """Wait until a predicate becomes true.

        The predicate should be a callable which result will be
        interpreted as a boolean value.  The final predicate value is
        the return value.
        """
        result = predicate()
        while not result:
            yield from self.wait()
            result = predicate()
        return result

    def notify(self, n=1):
        """By default, wake up one coroutine waiting on this condition, if any.
        If the calling coroutine has not acquired the lock when this method
        is called, a RuntimeError is raised.

        This method wakes up at most n of the coroutines waiting for the
        condition variable; it is a no-op if no coroutines are waiting.

        Note: an awakened coroutine does not actually return from its
        wait() call until it can reacquire the lock. Since notify() does
        not release the lock, its caller should.
        """
        if not self.locked():
            raise RuntimeError('cannot notify on un-acquired lock')

        idx = 0
        for fut in self._waiters:
            if idx >= n:
                break

            if not fut.done():
                idx += 1
                fut.set_result(False)

    def notify_all(self):
        """Wake up all threads waiting on this condition. This method acts
        like notify(), but wakes up all waiting threads instead of one. If the
        calling thread has not acquired the lock when this method is called,
        a RuntimeError is raised.
        """
        self.notify(len(self._waiters))

    def __enter__(self):
        return self._lock.__enter__()

    def __exit__(self, *args):
        return self._lock.__exit__(*args)

    def __iter__(self):
        yield from self.acquire()
        return self


class Semaphore:
    """A Semaphore implementation.

    A semaphore manages an internal counter which is decremented by each
    acquire() call and incremented by each release() call. The counter
    can never go below zero; when acquire() finds that it is zero, it blocks,
    waiting until some other thread calls release().

    Semaphores also support the context manager protocol.

    The first optional argument gives the initial value for the internal
    counter; it defaults to 1. If the value given is less than 0,
    ValueError is raised.

    The second optional argument determines if the semaphore can be released
    more than initial internal counter value; it defaults to False. If the
    value given is True and number of release() is more than number of
    successful acquire() calls ValueError is raised.
    """

    def __init__(self, value=1, bound=False, *, loop=None):
        if value < 0:
            raise ValueError("Semaphore initial value must be > 0")
        self._value = value
        self._bound = bound
        self._bound_value = value
        self._waiters = collections.deque()
        self._locked = False
        if loop is not None:
            self._loop = loop
        else:
            self._loop = events.get_event_loop()

    def __repr__(self):
        res = super().__repr__()
        extra = 'locked' if self._locked else 'unlocked,value:{}'.format(
            self._value)
        if self._waiters:
            extra = '{},waiters:{}'.format(extra, len(self._waiters))
        return '<{} [{}]>'.format(res[1:-1], extra)

    def locked(self):
        """Returns True if semaphore can not be acquired immediately."""
        return self._locked

    @tasks.coroutine
    def acquire(self):
        """Acquire a semaphore.

        If the internal counter is larger than zero on entry,
        decrement it by one and return True immediately.  If it is
        zero on entry, block, waiting until some other coroutine has
        called release() to make it larger than 0, and then return
        True.
        """
        if not self._waiters and self._value > 0:
            self._value -= 1
            if self._value == 0:
                self._locked = True
            return True

        fut = futures.Future(loop=self._loop)
        self._waiters.append(fut)
        try:
            yield from fut
            self._value -= 1
            if self._value == 0:
                self._locked = True
            return True
        finally:
            self._waiters.remove(fut)

    def release(self):
        """Release a semaphore, incrementing the internal counter by one.
        When it was zero on entry and another coroutine is waiting for it to
        become larger than zero again, wake up that coroutine.

        If Semaphore is created with "bound" parameter equals true, then
        release() method checks to make sure its current value doesn't exceed
        its initial value. If it does, ValueError is raised.
        """
        if self._bound and self._value >= self._bound_value:
            raise ValueError('Semaphore released too many times')

        self._value += 1
        self._locked = False

        for waiter in self._waiters:
            if not waiter.done():
                waiter.set_result(True)
                break

    def __enter__(self):
        # TODO: This is questionable.  How do we know the user actually
        # wrote "with (yield from sema)" instead of "with sema"?
        return True

    def __exit__(self, *args):
        self.release()

    def __iter__(self):
        yield from self.acquire()
        return self
