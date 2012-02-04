"""Thread module emulating a subset of Java's threading model."""

import sys as _sys
import _thread

from time import time as _time, sleep as _sleep
from traceback import format_exc as _format_exc
from _weakrefset import WeakSet

# Note regarding PEP 8 compliant names
#  This threading model was originally inspired by Java, and inherited
# the convention of camelCase function and method names from that
# language. Those originaly names are not in any imminent danger of
# being deprecated (even for Py3k),so this module provides them as an
# alias for the PEP 8 compliant names
# Note that using the new PEP 8 compliant names facilitates substitution
# with the multiprocessing module, which doesn't provide the old
# Java inspired names.

__all__ = ['active_count', 'Condition', 'current_thread', 'enumerate', 'Event',
           'Lock', 'RLock', 'Semaphore', 'BoundedSemaphore', 'Thread', 'Barrier',
           'Timer', 'setprofile', 'settrace', 'local', 'stack_size']

# Rename some stuff so "from threading import *" is safe
_start_new_thread = _thread.start_new_thread
_allocate_lock = _thread.allocate_lock
_get_ident = _thread.get_ident
ThreadError = _thread.error
try:
    _CRLock = _thread.RLock
except AttributeError:
    _CRLock = None
TIMEOUT_MAX = _thread.TIMEOUT_MAX
del _thread


# Debug support (adapted from ihooks.py).

_VERBOSE = False

if __debug__:

    class _Verbose(object):

        def __init__(self, verbose=None):
            if verbose is None:
                verbose = _VERBOSE
            self._verbose = verbose

        def _note(self, format, *args):
            if self._verbose:
                format = format % args
                # Issue #4188: calling current_thread() can incur an infinite
                # recursion if it has to create a DummyThread on the fly.
                ident = _get_ident()
                try:
                    name = _active[ident].name
                except KeyError:
                    name = "<OS thread %d>" % ident
                format = "%s: %s\n" % (name, format)
                _sys.stderr.write(format)

else:
    # Disable this when using "python -O"
    class _Verbose(object):
        def __init__(self, verbose=None):
            pass
        def _note(self, *args):
            pass

# Support for profile and trace hooks

_profile_hook = None
_trace_hook = None

def setprofile(func):
    global _profile_hook
    _profile_hook = func

def settrace(func):
    global _trace_hook
    _trace_hook = func

# Synchronization classes

Lock = _allocate_lock

def RLock(verbose=None, *args, **kwargs):
    if verbose is None:
        verbose = _VERBOSE
    if (__debug__ and verbose) or _CRLock is None:
        return _PyRLock(verbose, *args, **kwargs)
    return _CRLock(*args, **kwargs)

class _RLock(_Verbose):

    def __init__(self, verbose=None):
        _Verbose.__init__(self, verbose)
        self._block = _allocate_lock()
        self._owner = None
        self._count = 0

    def __repr__(self):
        owner = self._owner
        try:
            owner = _active[owner].name
        except KeyError:
            pass
        return "<%s owner=%r count=%d>" % (
                self.__class__.__name__, owner, self._count)

    def acquire(self, blocking=True, timeout=-1):
        me = _get_ident()
        if self._owner == me:
            self._count = self._count + 1
            if __debug__:
                self._note("%s.acquire(%s): recursive success", self, blocking)
            return 1
        rc = self._block.acquire(blocking, timeout)
        if rc:
            self._owner = me
            self._count = 1
            if __debug__:
                self._note("%s.acquire(%s): initial success", self, blocking)
        else:
            if __debug__:
                self._note("%s.acquire(%s): failure", self, blocking)
        return rc

    __enter__ = acquire

    def release(self):
        if self._owner != _get_ident():
            raise RuntimeError("cannot release un-acquired lock")
        self._count = count = self._count - 1
        if not count:
            self._owner = None
            self._block.release()
            if __debug__:
                self._note("%s.release(): final release", self)
        else:
            if __debug__:
                self._note("%s.release(): non-final release", self)

    def __exit__(self, t, v, tb):
        self.release()

    # Internal methods used by condition variables

    def _acquire_restore(self, state):
        self._block.acquire()
        self._count, self._owner = state
        if __debug__:
            self._note("%s._acquire_restore()", self)

    def _release_save(self):
        if __debug__:
            self._note("%s._release_save()", self)
        count = self._count
        self._count = 0
        owner = self._owner
        self._owner = None
        self._block.release()
        return (count, owner)

    def _is_owned(self):
        return self._owner == _get_ident()

_PyRLock = _RLock


def Condition(*args, **kwargs):
    return _Condition(*args, **kwargs)

class _Condition(_Verbose):

    def __init__(self, lock=None, verbose=None):
        _Verbose.__init__(self, verbose)
        if lock is None:
            lock = RLock()
        self._lock = lock
        # Export the lock's acquire() and release() methods
        self.acquire = lock.acquire
        self.release = lock.release
        # If the lock defines _release_save() and/or _acquire_restore(),
        # these override the default implementations (which just call
        # release() and acquire() on the lock).  Ditto for _is_owned().
        try:
            self._release_save = lock._release_save
        except AttributeError:
            pass
        try:
            self._acquire_restore = lock._acquire_restore
        except AttributeError:
            pass
        try:
            self._is_owned = lock._is_owned
        except AttributeError:
            pass
        self._waiters = []

    def __enter__(self):
        return self._lock.__enter__()

    def __exit__(self, *args):
        return self._lock.__exit__(*args)

    def __repr__(self):
        return "<Condition(%s, %d)>" % (self._lock, len(self._waiters))

    def _release_save(self):
        self._lock.release()           # No state to save

    def _acquire_restore(self, x):
        self._lock.acquire()           # Ignore saved state

    def _is_owned(self):
        # Return True if lock is owned by current_thread.
        # This method is called only if __lock doesn't have _is_owned().
        if self._lock.acquire(0):
            self._lock.release()
            return False
        else:
            return True

    def wait(self, timeout=None):
        if not self._is_owned():
            raise RuntimeError("cannot wait on un-acquired lock")
        waiter = _allocate_lock()
        waiter.acquire()
        self._waiters.append(waiter)
        saved_state = self._release_save()
        try:    # restore state no matter what (e.g., KeyboardInterrupt)
            if timeout is None:
                waiter.acquire()
                gotit = True
                if __debug__:
                    self._note("%s.wait(): got it", self)
            else:
                if timeout > 0:
                    gotit = waiter.acquire(True, timeout)
                else:
                    gotit = waiter.acquire(False)
                if not gotit:
                    if __debug__:
                        self._note("%s.wait(%s): timed out", self, timeout)
                    try:
                        self._waiters.remove(waiter)
                    except ValueError:
                        pass
                else:
                    if __debug__:
                        self._note("%s.wait(%s): got it", self, timeout)
            return gotit
        finally:
            self._acquire_restore(saved_state)

    def wait_for(self, predicate, timeout=None):
        endtime = None
        waittime = timeout
        result = predicate()
        while not result:
            if waittime is not None:
                if endtime is None:
                    endtime = _time() + waittime
                else:
                    waittime = endtime - _time()
                    if waittime <= 0:
                        if __debug__:
                            self._note("%s.wait_for(%r, %r): Timed out.",
                                       self, predicate, timeout)
                        break
            if __debug__:
                self._note("%s.wait_for(%r, %r): Waiting with timeout=%s.",
                           self, predicate, timeout, waittime)
            self.wait(waittime)
            result = predicate()
        else:
            if __debug__:
                self._note("%s.wait_for(%r, %r): Success.",
                           self, predicate, timeout)
        return result

    def notify(self, n=1):
        if not self._is_owned():
            raise RuntimeError("cannot notify on un-acquired lock")
        __waiters = self._waiters
        waiters = __waiters[:n]
        if not waiters:
            if __debug__:
                self._note("%s.notify(): no waiters", self)
            return
        self._note("%s.notify(): notifying %d waiter%s", self, n,
                   n!=1 and "s" or "")
        for waiter in waiters:
            waiter.release()
            try:
                __waiters.remove(waiter)
            except ValueError:
                pass

    def notify_all(self):
        self.notify(len(self._waiters))

    notifyAll = notify_all


def Semaphore(*args, **kwargs):
    return _Semaphore(*args, **kwargs)

class _Semaphore(_Verbose):

    # After Tim Peters' semaphore class, but not quite the same (no maximum)

    def __init__(self, value=1, verbose=None):
        if value < 0:
            raise ValueError("semaphore initial value must be >= 0")
        _Verbose.__init__(self, verbose)
        self._cond = Condition(Lock())
        self._value = value

    def acquire(self, blocking=True, timeout=None):
        if not blocking and timeout is not None:
            raise ValueError("can't specify timeout for non-blocking acquire")
        rc = False
        endtime = None
        self._cond.acquire()
        while self._value == 0:
            if not blocking:
                break
            if __debug__:
                self._note("%s.acquire(%s): blocked waiting, value=%s",
                           self, blocking, self._value)
            if timeout is not None:
                if endtime is None:
                    endtime = _time() + timeout
                else:
                    timeout = endtime - _time()
                    if timeout <= 0:
                        break
            self._cond.wait(timeout)
        else:
            self._value = self._value - 1
            if __debug__:
                self._note("%s.acquire: success, value=%s",
                           self, self._value)
            rc = True
        self._cond.release()
        return rc

    __enter__ = acquire

    def release(self):
        self._cond.acquire()
        self._value = self._value + 1
        if __debug__:
            self._note("%s.release: success, value=%s",
                       self, self._value)
        self._cond.notify()
        self._cond.release()

    def __exit__(self, t, v, tb):
        self.release()


def BoundedSemaphore(*args, **kwargs):
    return _BoundedSemaphore(*args, **kwargs)

class _BoundedSemaphore(_Semaphore):
    """Semaphore that checks that # releases is <= # acquires"""
    def __init__(self, value=1, verbose=None):
        _Semaphore.__init__(self, value, verbose)
        self._initial_value = value

    def release(self):
        if self._value >= self._initial_value:
            raise ValueError("Semaphore released too many times")
        return _Semaphore.release(self)


def Event(*args, **kwargs):
    return _Event(*args, **kwargs)

class _Event(_Verbose):

    # After Tim Peters' event class (without is_posted())

    def __init__(self, verbose=None):
        _Verbose.__init__(self, verbose)
        self._cond = Condition(Lock())
        self._flag = False

    def _reset_internal_locks(self):
        # private!  called by Thread._reset_internal_locks by _after_fork()
        self._cond.__init__()

    def is_set(self):
        return self._flag

    isSet = is_set

    def set(self):
        self._cond.acquire()
        try:
            self._flag = True
            self._cond.notify_all()
        finally:
            self._cond.release()

    def clear(self):
        self._cond.acquire()
        try:
            self._flag = False
        finally:
            self._cond.release()

    def wait(self, timeout=None):
        self._cond.acquire()
        try:
            signaled = self._flag
            if not signaled:
                signaled = self._cond.wait(timeout)
            return signaled
        finally:
            self._cond.release()


# A barrier class.  Inspired in part by the pthread_barrier_* api and
# the CyclicBarrier class from Java.  See
# http://sourceware.org/pthreads-win32/manual/pthread_barrier_init.html and
# http://java.sun.com/j2se/1.5.0/docs/api/java/util/concurrent/
#        CyclicBarrier.html
# for information.
# We maintain two main states, 'filling' and 'draining' enabling the barrier
# to be cyclic.  Threads are not allowed into it until it has fully drained
# since the previous cycle.  In addition, a 'resetting' state exists which is
# similar to 'draining' except that threads leave with a BrokenBarrierError,
# and a 'broken' state in which all threads get the exception.
class Barrier(_Verbose):
    """
    Barrier.  Useful for synchronizing a fixed number of threads
    at known synchronization points.  Threads block on 'wait()' and are
    simultaneously once they have all made that call.
    """
    def __init__(self, parties, action=None, timeout=None, verbose=None):
        """
        Create a barrier, initialised to 'parties' threads.
        'action' is a callable which, when supplied, will be called
        by one of the threads after they have all entered the
        barrier and just prior to releasing them all.
        If a 'timeout' is provided, it is uses as the default for
        all subsequent 'wait()' calls.
        """
        _Verbose.__init__(self, verbose)
        self._cond = Condition(Lock())
        self._action = action
        self._timeout = timeout
        self._parties = parties
        self._state = 0 #0 filling, 1, draining, -1 resetting, -2 broken
        self._count = 0

    def wait(self, timeout=None):
        """
        Wait for the barrier.  When the specified number of threads have
        started waiting, they are all simultaneously awoken. If an 'action'
        was provided for the barrier, one of the threads will have executed
        that callback prior to returning.
        Returns an individual index number from 0 to 'parties-1'.
        """
        if timeout is None:
            timeout = self._timeout
        with self._cond:
            self._enter() # Block while the barrier drains.
            index = self._count
            self._count += 1
            try:
                if index + 1 == self._parties:
                    # We release the barrier
                    self._release()
                else:
                    # We wait until someone releases us
                    self._wait(timeout)
                return index
            finally:
                self._count -= 1
                # Wake up any threads waiting for barrier to drain.
                self._exit()

    # Block until the barrier is ready for us, or raise an exception
    # if it is broken.
    def _enter(self):
        while self._state in (-1, 1):
            # It is draining or resetting, wait until done
            self._cond.wait()
        #see if the barrier is in a broken state
        if self._state < 0:
            raise BrokenBarrierError
        assert self._state == 0

    # Optionally run the 'action' and release the threads waiting
    # in the barrier.
    def _release(self):
        try:
            if self._action:
                self._action()
            # enter draining state
            self._state = 1
            self._cond.notify_all()
        except:
            #an exception during the _action handler.  Break and reraise
            self._break()
            raise

    # Wait in the barrier until we are relased.  Raise an exception
    # if the barrier is reset or broken.
    def _wait(self, timeout):
        if not self._cond.wait_for(lambda : self._state != 0, timeout):
            #timed out.  Break the barrier
            self._break()
            raise BrokenBarrierError
        if self._state < 0:
            raise BrokenBarrierError
        assert self._state == 1

    # If we are the last thread to exit the barrier, signal any threads
    # waiting for the barrier to drain.
    def _exit(self):
        if self._count == 0:
            if self._state in (-1, 1):
                #resetting or draining
                self._state = 0
                self._cond.notify_all()

    def reset(self):
        """
        Reset the barrier to the initial state.
        Any threads currently waiting will get the BrokenBarrier exception
        raised.
        """
        with self._cond:
            if self._count > 0:
                if self._state == 0:
                    #reset the barrier, waking up threads
                    self._state = -1
                elif self._state == -2:
                    #was broken, set it to reset state
                    #which clears when the last thread exits
                    self._state = -1
            else:
                self._state = 0
            self._cond.notify_all()

    def abort(self):
        """
        Place the barrier into a 'broken' state.
        Useful in case of error.  Any currently waiting threads and
        threads attempting to 'wait()' will have BrokenBarrierError
        raised.
        """
        with self._cond:
            self._break()

    def _break(self):
        # An internal error was detected.  The barrier is set to
        # a broken state all parties awakened.
        self._state = -2
        self._cond.notify_all()

    @property
    def parties(self):
        """
        Return the number of threads required to trip the barrier.
        """
        return self._parties

    @property
    def n_waiting(self):
        """
        Return the number of threads that are currently waiting at the barrier.
        """
        # We don't need synchronization here since this is an ephemeral result
        # anyway.  It returns the correct value in the steady state.
        if self._state == 0:
            return self._count
        return 0

    @property
    def broken(self):
        """
        Return True if the barrier is in a broken state
        """
        return self._state == -2

#exception raised by the Barrier class
class BrokenBarrierError(RuntimeError): pass


# Helper to generate new thread names
_counter = 0
def _newname(template="Thread-%d"):
    global _counter
    _counter = _counter + 1
    return template % _counter

# Active thread administration
_active_limbo_lock = _allocate_lock()
_active = {}    # maps thread id to Thread object
_limbo = {}

# For debug and leak testing
_dangling = WeakSet()

# Main class for threads

class Thread(_Verbose):

    __initialized = False
    # Need to store a reference to sys.exc_info for printing
    # out exceptions when a thread tries to use a global var. during interp.
    # shutdown and thus raises an exception about trying to perform some
    # operation on/with a NoneType
    __exc_info = _sys.exc_info
    # Keep sys.exc_clear too to clear the exception just before
    # allowing .join() to return.
    #XXX __exc_clear = _sys.exc_clear

    def __init__(self, group=None, target=None, name=None,
                 args=(), kwargs=None, verbose=None):
        assert group is None, "group argument must be None for now"
        _Verbose.__init__(self, verbose)
        if kwargs is None:
            kwargs = {}
        self._target = target
        self._name = str(name or _newname())
        self._args = args
        self._kwargs = kwargs
        self._daemonic = self._set_daemon()
        self._ident = None
        self._started = Event()
        self._stopped = False
        self._block = Condition(Lock())
        self._initialized = True
        # sys.stderr is not stored in the class like
        # sys.exc_info since it can be changed between instances
        self._stderr = _sys.stderr
        _dangling.add(self)

    def _reset_internal_locks(self):
        # private!  Called by _after_fork() to reset our internal locks as
        # they may be in an invalid state leading to a deadlock or crash.
        if hasattr(self, '_block'):  # DummyThread deletes _block
            self._block.__init__()
        self._started._reset_internal_locks()

    def _set_daemon(self):
        # Overridden in _MainThread and _DummyThread
        return current_thread().daemon

    def __repr__(self):
        assert self._initialized, "Thread.__init__() was not called"
        status = "initial"
        if self._started.is_set():
            status = "started"
        if self._stopped:
            status = "stopped"
        if self._daemonic:
            status += " daemon"
        if self._ident is not None:
            status += " %s" % self._ident
        return "<%s(%s, %s)>" % (self.__class__.__name__, self._name, status)

    def start(self):
        if not self._initialized:
            raise RuntimeError("thread.__init__() not called")

        if self._started.is_set():
            raise RuntimeError("threads can only be started once")
        if __debug__:
            self._note("%s.start(): starting thread", self)
        with _active_limbo_lock:
            _limbo[self] = self
        try:
            _start_new_thread(self._bootstrap, ())
        except Exception:
            with _active_limbo_lock:
                del _limbo[self]
            raise
        self._started.wait()

    def run(self):
        try:
            if self._target:
                self._target(*self._args, **self._kwargs)
        finally:
            # Avoid a refcycle if the thread is running a function with
            # an argument that has a member that points to the thread.
            del self._target, self._args, self._kwargs

    def _bootstrap(self):
        # Wrapper around the real bootstrap code that ignores
        # exceptions during interpreter cleanup.  Those typically
        # happen when a daemon thread wakes up at an unfortunate
        # moment, finds the world around it destroyed, and raises some
        # random exception *** while trying to report the exception in
        # _bootstrap_inner() below ***.  Those random exceptions
        # don't help anybody, and they confuse users, so we suppress
        # them.  We suppress them only when it appears that the world
        # indeed has already been destroyed, so that exceptions in
        # _bootstrap_inner() during normal business hours are properly
        # reported.  Also, we only suppress them for daemonic threads;
        # if a non-daemonic encounters this, something else is wrong.
        try:
            self._bootstrap_inner()
        except:
            if self._daemonic and _sys is None:
                return
            raise

    def _set_ident(self):
        self._ident = _get_ident()

    def _bootstrap_inner(self):
        try:
            self._set_ident()
            self._started.set()
            with _active_limbo_lock:
                _active[self._ident] = self
                del _limbo[self]
            if __debug__:
                self._note("%s._bootstrap(): thread started", self)

            if _trace_hook:
                self._note("%s._bootstrap(): registering trace hook", self)
                _sys.settrace(_trace_hook)
            if _profile_hook:
                self._note("%s._bootstrap(): registering profile hook", self)
                _sys.setprofile(_profile_hook)

            try:
                self.run()
            except SystemExit:
                if __debug__:
                    self._note("%s._bootstrap(): raised SystemExit", self)
            except:
                if __debug__:
                    self._note("%s._bootstrap(): unhandled exception", self)
                # If sys.stderr is no more (most likely from interpreter
                # shutdown) use self._stderr.  Otherwise still use sys (as in
                # _sys) in case sys.stderr was redefined since the creation of
                # self.
                if _sys:
                    _sys.stderr.write("Exception in thread %s:\n%s\n" %
                                      (self.name, _format_exc()))
                else:
                    # Do the best job possible w/o a huge amt. of code to
                    # approximate a traceback (code ideas from
                    # Lib/traceback.py)
                    exc_type, exc_value, exc_tb = self._exc_info()
                    try:
                        print((
                            "Exception in thread " + self.name +
                            " (most likely raised during interpreter shutdown):"), file=self._stderr)
                        print((
                            "Traceback (most recent call last):"), file=self._stderr)
                        while exc_tb:
                            print((
                                '  File "%s", line %s, in %s' %
                                (exc_tb.tb_frame.f_code.co_filename,
                                    exc_tb.tb_lineno,
                                    exc_tb.tb_frame.f_code.co_name)), file=self._stderr)
                            exc_tb = exc_tb.tb_next
                        print(("%s: %s" % (exc_type, exc_value)), file=self._stderr)
                    # Make sure that exc_tb gets deleted since it is a memory
                    # hog; deleting everything else is just for thoroughness
                    finally:
                        del exc_type, exc_value, exc_tb
            else:
                if __debug__:
                    self._note("%s._bootstrap(): normal return", self)
            finally:
                # Prevent a race in
                # test_threading.test_no_refcycle_through_target when
                # the exception keeps the target alive past when we
                # assert that it's dead.
                #XXX self.__exc_clear()
                pass
        finally:
            with _active_limbo_lock:
                self._stop()
                try:
                    # We don't call self._delete() because it also
                    # grabs _active_limbo_lock.
                    del _active[_get_ident()]
                except:
                    pass

    def _stop(self):
        self._block.acquire()
        self._stopped = True
        self._block.notify_all()
        self._block.release()

    def _delete(self):
        "Remove current thread from the dict of currently running threads."

        # Notes about running with _dummy_thread:
        #
        # Must take care to not raise an exception if _dummy_thread is being
        # used (and thus this module is being used as an instance of
        # dummy_threading).  _dummy_thread.get_ident() always returns -1 since
        # there is only one thread if _dummy_thread is being used.  Thus
        # len(_active) is always <= 1 here, and any Thread instance created
        # overwrites the (if any) thread currently registered in _active.
        #
        # An instance of _MainThread is always created by 'threading'.  This
        # gets overwritten the instant an instance of Thread is created; both
        # threads return -1 from _dummy_thread.get_ident() and thus have the
        # same key in the dict.  So when the _MainThread instance created by
        # 'threading' tries to clean itself up when atexit calls this method
        # it gets a KeyError if another Thread instance was created.
        #
        # This all means that KeyError from trying to delete something from
        # _active if dummy_threading is being used is a red herring.  But
        # since it isn't if dummy_threading is *not* being used then don't
        # hide the exception.

        try:
            with _active_limbo_lock:
                del _active[_get_ident()]
                # There must not be any python code between the previous line
                # and after the lock is released.  Otherwise a tracing function
                # could try to acquire the lock again in the same thread, (in
                # current_thread()), and would block.
        except KeyError:
            if 'dummy_threading' not in _sys.modules:
                raise

    def join(self, timeout=None):
        if not self._initialized:
            raise RuntimeError("Thread.__init__() not called")
        if not self._started.is_set():
            raise RuntimeError("cannot join thread before it is started")
        if self is current_thread():
            raise RuntimeError("cannot join current thread")

        if __debug__:
            if not self._stopped:
                self._note("%s.join(): waiting until thread stops", self)

        self._block.acquire()
        try:
            if timeout is None:
                while not self._stopped:
                    self._block.wait()
                if __debug__:
                    self._note("%s.join(): thread stopped", self)
            else:
                deadline = _time() + timeout
                while not self._stopped:
                    delay = deadline - _time()
                    if delay <= 0:
                        if __debug__:
                            self._note("%s.join(): timed out", self)
                        break
                    self._block.wait(delay)
                else:
                    if __debug__:
                        self._note("%s.join(): thread stopped", self)
        finally:
            self._block.release()

    @property
    def name(self):
        assert self._initialized, "Thread.__init__() not called"
        return self._name

    @name.setter
    def name(self, name):
        assert self._initialized, "Thread.__init__() not called"
        self._name = str(name)

    @property
    def ident(self):
        assert self._initialized, "Thread.__init__() not called"
        return self._ident

    def is_alive(self):
        assert self._initialized, "Thread.__init__() not called"
        return self._started.is_set() and not self._stopped

    isAlive = is_alive

    @property
    def daemon(self):
        assert self._initialized, "Thread.__init__() not called"
        return self._daemonic

    @daemon.setter
    def daemon(self, daemonic):
        if not self._initialized:
            raise RuntimeError("Thread.__init__() not called")
        if self._started.is_set():
            raise RuntimeError("cannot set daemon status of active thread");
        self._daemonic = daemonic

    def isDaemon(self):
        return self.daemon

    def setDaemon(self, daemonic):
        self.daemon = daemonic

    def getName(self):
        return self.name

    def setName(self, name):
        self.name = name

# The timer class was contributed by Itamar Shtull-Trauring

def Timer(*args, **kwargs):
    return _Timer(*args, **kwargs)

class _Timer(Thread):
    """Call a function after a specified number of seconds:

    t = Timer(30.0, f, args=[], kwargs={})
    t.start()
    t.cancel() # stop the timer's action if it's still waiting
    """

    def __init__(self, interval, function, args=[], kwargs={}):
        Thread.__init__(self)
        self.interval = interval
        self.function = function
        self.args = args
        self.kwargs = kwargs
        self.finished = Event()

    def cancel(self):
        """Stop the timer if it hasn't finished yet"""
        self.finished.set()

    def run(self):
        self.finished.wait(self.interval)
        if not self.finished.is_set():
            self.function(*self.args, **self.kwargs)
        self.finished.set()

# Special thread class to represent the main thread
# This is garbage collected through an exit handler

class _MainThread(Thread):

    def __init__(self):
        Thread.__init__(self, name="MainThread")
        self._started.set()
        self._set_ident()
        with _active_limbo_lock:
            _active[self._ident] = self

    def _set_daemon(self):
        return False

    def _exitfunc(self):
        self._stop()
        t = _pickSomeNonDaemonThread()
        if t:
            if __debug__:
                self._note("%s: waiting for other threads", self)
        while t:
            t.join()
            t = _pickSomeNonDaemonThread()
        if __debug__:
            self._note("%s: exiting", self)
        self._delete()

def _pickSomeNonDaemonThread():
    for t in enumerate():
        if not t.daemon and t.is_alive():
            return t
    return None


# Dummy thread class to represent threads not started here.
# These aren't garbage collected when they die, nor can they be waited for.
# If they invoke anything in threading.py that calls current_thread(), they
# leave an entry in the _active dict forever after.
# Their purpose is to return *something* from current_thread().
# They are marked as daemon threads so we won't wait for them
# when we exit (conform previous semantics).

class _DummyThread(Thread):

    def __init__(self):
        Thread.__init__(self, name=_newname("Dummy-%d"))

        # Thread._block consumes an OS-level locking primitive, which
        # can never be used by a _DummyThread.  Since a _DummyThread
        # instance is immortal, that's bad, so release this resource.
        del self._block

        self._started.set()
        self._set_ident()
        with _active_limbo_lock:
            _active[self._ident] = self

    def _set_daemon(self):
        return True

    def join(self, timeout=None):
        assert False, "cannot join a dummy thread"


# Global API functions

def current_thread():
    try:
        return _active[_get_ident()]
    except KeyError:
        ##print "current_thread(): no current thread for", _get_ident()
        return _DummyThread()

currentThread = current_thread

def active_count():
    with _active_limbo_lock:
        return len(_active) + len(_limbo)

activeCount = active_count

def _enumerate():
    # Same as enumerate(), but without the lock. Internal use only.
    return list(_active.values()) + list(_limbo.values())

def enumerate():
    with _active_limbo_lock:
        return list(_active.values()) + list(_limbo.values())

from _thread import stack_size

# Create the main thread object,
# and make it available for the interpreter
# (Py_Main) as threading._shutdown.

_shutdown = _MainThread()._exitfunc

# get thread-local implementation, either from the thread
# module, or from the python fallback

try:
    from _thread import _local as local
except ImportError:
    from _threading_local import local


def _after_fork():
    # This function is called by Python/ceval.c:PyEval_ReInitThreads which
    # is called from PyOS_AfterFork.  Here we cleanup threading module state
    # that should not exist after a fork.

    # Reset _active_limbo_lock, in case we forked while the lock was held
    # by another (non-forked) thread.  http://bugs.python.org/issue874900
    global _active_limbo_lock
    _active_limbo_lock = _allocate_lock()

    # fork() only copied the current thread; clear references to others.
    new_active = {}
    current = current_thread()
    with _active_limbo_lock:
        for thread in _active.values():
            # Any lock/condition variable may be currently locked or in an
            # invalid state, so we reinitialize them.
            thread._reset_internal_locks()
            if thread is current:
                # There is only one active thread. We reset the ident to
                # its new value since it can have changed.
                ident = _get_ident()
                thread._ident = ident
                new_active[ident] = thread
            else:
                # All the others are already stopped.
                thread._stop()

        _limbo.clear()
        _active.clear()
        _active.update(new_active)
        assert len(_active) == 1
