import _thread
import contextlib
import functools
import sys
import threading
import time
import unittest

from test import support


#=======================================================================
# Threading support to prevent reporting refleaks when running regrtest.py -R

# NOTE: we use thread._count() rather than threading.enumerate() (or the
# moral equivalent thereof) because a threading.Thread object is still alive
# until its __bootstrap() method has returned, even after it has been
# unregistered from the threading module.
# thread._count(), on the other hand, only gets decremented *after* the
# __bootstrap() method has returned, which gives us reliable reference counts
# at the end of a test run.


def threading_setup():
    return _thread._count(), len(threading._dangling)


def threading_cleanup(*original_values):
    orig_count, orig_ndangling = original_values

    timeout = 1.0
    for _ in support.sleeping_retry(timeout, error=False):
        # Copy the thread list to get a consistent output. threading._dangling
        # is a WeakSet, its value changes when it's read.
        dangling_threads = list(threading._dangling)
        count = _thread._count()

        if count <= orig_count:
            return

    # Timeout!
    support.environment_altered = True
    support.print_warning(
        f"threading_cleanup() failed to clean up threads "
        f"in {timeout:.1f} seconds\n"
        f"  before: thread count={orig_count}, dangling={orig_ndangling}\n"
        f"  after: thread count={count}, dangling={len(dangling_threads)}")
    for thread in dangling_threads:
        support.print_warning(f"Dangling thread: {thread!r}")

    # The warning happens when a test spawns threads and some of these threads
    # are still running after the test completes. To fix this warning, join
    # threads explicitly to wait until they complete.
    #
    # To make the warning more likely, reduce the timeout.


def reap_threads(func):
    """Use this function when threads are being used.  This will
    ensure that the threads are cleaned up even when the test fails.
    """
    @functools.wraps(func)
    def decorator(*args):
        key = threading_setup()
        try:
            return func(*args)
        finally:
            threading_cleanup(*key)
    return decorator


@contextlib.contextmanager
def wait_threads_exit(timeout=None):
    """
    bpo-31234: Context manager to wait until all threads created in the with
    statement exit.

    Use _thread.count() to check if threads exited. Indirectly, wait until
    threads exit the internal t_bootstrap() C function of the _thread module.

    threading_setup() and threading_cleanup() are designed to emit a warning
    if a test leaves running threads in the background. This context manager
    is designed to cleanup threads started by the _thread.start_new_thread()
    which doesn't allow to wait for thread exit, whereas thread.Thread has a
    join() method.
    """
    if timeout is None:
        timeout = support.SHORT_TIMEOUT
    old_count = _thread._count()
    try:
        yield
    finally:
        start_time = time.monotonic()
        for _ in support.sleeping_retry(timeout, error=False):
            support.gc_collect()
            count = _thread._count()
            if count <= old_count:
                break
        else:
            dt = time.monotonic() - start_time
            msg = (f"wait_threads() failed to cleanup {count - old_count} "
                   f"threads after {dt:.1f} seconds "
                   f"(count: {count}, old count: {old_count})")
            raise AssertionError(msg)


def join_thread(thread, timeout=None):
    """Join a thread. Raise an AssertionError if the thread is still alive
    after timeout seconds.
    """
    if timeout is None:
        timeout = support.SHORT_TIMEOUT
    thread.join(timeout)
    if thread.is_alive():
        msg = f"failed to join the thread in {timeout:.1f} seconds"
        raise AssertionError(msg)


@contextlib.contextmanager
def start_threads(threads, unlock=None):
    try:
        import faulthandler
    except ImportError:
        # It isn't supported on subinterpreters yet.
        faulthandler = None
    threads = list(threads)
    started = []
    try:
        try:
            for t in threads:
                t.start()
                started.append(t)
        except:
            if support.verbose:
                print("Can't start %d threads, only %d threads started" %
                      (len(threads), len(started)))
            raise
        yield
    finally:
        try:
            if unlock:
                unlock()
            endtime = time.monotonic()
            for timeout in range(1, 16):
                endtime += 60
                for t in started:
                    t.join(max(endtime - time.monotonic(), 0.01))
                started = [t for t in started if t.is_alive()]
                if not started:
                    break
                if support.verbose:
                    print('Unable to join %d threads during a period of '
                          '%d minutes' % (len(started), timeout))
        finally:
            started = [t for t in started if t.is_alive()]
            if started:
                if faulthandler is not None:
                    faulthandler.dump_traceback(sys.stdout)
                raise AssertionError('Unable to join %d threads' % len(started))


class catch_threading_exception:
    """
    Context manager catching threading.Thread exception using
    threading.excepthook.

    Attributes set when an exception is caught:

    * exc_type
    * exc_value
    * exc_traceback
    * thread

    See threading.excepthook() documentation for these attributes.

    These attributes are deleted at the context manager exit.

    Usage:

        with threading_helper.catch_threading_exception() as cm:
            # code spawning a thread which raises an exception
            ...

            # check the thread exception, use cm attributes:
            # exc_type, exc_value, exc_traceback, thread
            ...

        # exc_type, exc_value, exc_traceback, thread attributes of cm no longer
        # exists at this point
        # (to avoid reference cycles)
    """

    def __init__(self):
        self.exc_type = None
        self.exc_value = None
        self.exc_traceback = None
        self.thread = None
        self._old_hook = None

    def _hook(self, args):
        self.exc_type = args.exc_type
        self.exc_value = args.exc_value
        self.exc_traceback = args.exc_traceback
        self.thread = args.thread

    def __enter__(self):
        self._old_hook = threading.excepthook
        threading.excepthook = self._hook
        return self

    def __exit__(self, *exc_info):
        threading.excepthook = self._old_hook
        del self.exc_type
        del self.exc_value
        del self.exc_traceback
        del self.thread


def _can_start_thread() -> bool:
    """Detect whether Python can start new threads.

    Some WebAssembly platforms do not provide a working pthread
    implementation. Thread support is stubbed and any attempt
    to create a new thread fails.

    - wasm32-wasi does not have threading.
    - wasm32-emscripten can be compiled with or without pthread
      support (-s USE_PTHREADS / __EMSCRIPTEN_PTHREADS__).
    """
    if sys.platform == "emscripten":
        return sys._emscripten_info.pthreads
    elif sys.platform == "wasi":
        return False
    else:
        # assume all other platforms have working thread support.
        return True

can_start_thread = _can_start_thread()

def requires_working_threading(*, module=False):
    """Skip tests or modules that require working threading.

    Can be used as a function/class decorator or to skip an entire module.
    """
    msg = "requires threading support"
    if module:
        if not can_start_thread:
            raise unittest.SkipTest(msg)
    else:
        return unittest.skipUnless(can_start_thread, msg)


def run_concurrently(worker_func, nthreads, args=(), kwargs={}):
    """
    Run the worker function concurrently in multiple threads.
    """
    barrier = threading.Barrier(nthreads)

    def wrapper_func(*args, **kwargs):
        # Wait for all threads to reach this point before proceeding.
        barrier.wait()
        worker_func(*args, **kwargs)

    with catch_threading_exception() as cm:
        workers = [
            threading.Thread(target=wrapper_func, args=args, kwargs=kwargs)
            for _ in range(nthreads)
        ]
        with start_threads(workers):
            pass

        # If a worker thread raises an exception, re-raise it.
        if cm.exc_value is not None:
            raise cm.exc_value
