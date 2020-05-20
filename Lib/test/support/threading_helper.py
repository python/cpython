import contextlib
import functools
import _thread
import threading
import time

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
    return _thread._count(), threading._dangling.copy()

def threading_cleanup(*original_values):
    _MAX_COUNT = 100

    for count in range(_MAX_COUNT):
        values = _thread._count(), threading._dangling
        if values == original_values:
            break

        if not count:
            # Display a warning at the first iteration
            support.environment_altered = True
            dangling_threads = values[1]
            support.print_warning(f"threading_cleanup() failed to cleanup "
                                  f"{values[0] - original_values[0]} threads "
                                  f"(count: {values[0]}, "
                                  f"dangling: {len(dangling_threads)})")
            for thread in dangling_threads:
                support.print_warning(f"Dangling thread: {thread!r}")

            # Don't hold references to threads
            dangling_threads = None
        values = None

        time.sleep(0.01)
        gc_collect()


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
        deadline = start_time + timeout
        while True:
            count = _thread._count()
            if count <= old_count:
                break
            if time.monotonic() > deadline:
                dt = time.monotonic() - start_time
                msg = (f"wait_threads() failed to cleanup {count - old_count} "
                       f"threads after {dt:.1f} seconds "
                       f"(count: {count}, old count: {old_count})")
                raise AssertionError(msg)
            time.sleep(0.010)
            gc_collect()


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
    import faulthandler
    threads = list(threads)
    started = []
    try:
        try:
            for t in threads:
                t.start()
                started.append(t)
        except:
            if verbose:
                print("Can't start %d threads, only %d threads started" %
                      (len(threads), len(started)))
            raise
        yield
    finally:
        try:
            if unlock:
                unlock()
            endtime = starttime = time.monotonic()
            for timeout in range(1, 16):
                endtime += 60
                for t in started:
                    t.join(max(endtime - time.monotonic(), 0.01))
                started = [t for t in started if t.is_alive()]
                if not started:
                    break
                if verbose:
                    print('Unable to join %d threads during a period of '
                          '%d minutes' % (len(started), timeout))
        finally:
            started = [t for t in started if t.is_alive()]
            if started:
                faulthandler.dump_traceback(sys.stdout)
                raise AssertionError('Unable to join %d threads' % len(started))
