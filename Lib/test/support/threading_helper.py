import _thread
from concurrent.futures import ThreadPoolExecutor, wait
import contextlib
import functools
import sys
from test.support.socket_helper import bind_port, HOST
import threading
from socket import socket
import time
import unittest

from test import support

_thread_pool = None


def setUpModule():
    _thread_pool = ThreadPoolExecutor()


def tearDownModule():
    _thread_pool = None


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
        support.gc_collect()


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
    import faulthandler
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


class Server:
    """A context manager for a blocking server in a thread pool.

    The server is designed:

    - for testing purposes so it serves a fixed count of clients, one by one
    - to be one-pass, short-lived, and terminated by in-protocol means so no
      stopper flag is used
    - to be used where asyncio has no application

    The server listens on an address returned from the ``with`` statement.
    """

    def __init__(self, client_func, *args, client_count=1, results=[],
                 client_fails=False, **kwargs):
        """Create and run the server.

        The method blocks until a server is ready to accept clients.

        When client_func raises an exception, all server-side sockets are
        closed.

        Args:
            client_func: a function called in a dedicated thread for each new
                connected client. The function receives all argument passed to
                the __init__ method excluding client_func and client_count.
            args: positional arguments passed to client_func.
            client_count: count of clients the server processes one by one
                before stopping.
            results: a reference to a list for collecting client_func
                return values. Populated after execution leaves a ``with``
                blocks associated with the Server context manager.
            client_fails: if True, a client is expected to cause
                connection-related exceptions by reasons asserted on its side.
                As a result, a server should swallow these exceptions and
                proceed to the next client instead. 
            kwargs: keyword arguments passed to client_func.

        Throws:
            When client_func throws, this method catches the exception, wraps
            it into RuntimeError("server-side error") and rethrows.
        """
        server_socket = socket()
        self._port = bind_port(server_socket)
        server_socket.listen()
        self._result = _thread_pool.submit(self._thread_func, server_socket,
                                           client_func, client_count,
                                           client_fails, args, kwargs)
        self._result_out = results

    def _thread_func(self, server_socket, client_func, client_count,
                     client_fails, args, kwargs):
        server_socket.settimeout(support.LOOPBACK_TIMEOUT)
        with server_socket:
            results = []
            for i in range(client_count):
                try:
                    client, peer_address = server_socket.accept()
                    with client:
                        results.append(client_func(client, peer_address,
                                                   *args, **kwargs))
                except (ConnectionAbortedError, ConnectionResetError):
                    if not client_fails:
                        raise
                # OSError is caused by read()/write() on a socket unexpectedly
                # closed by a client. However, important exceprions like
                # ssl.SSLError subclass OSError so we need a separate logic
                # to split them away.
                except OSError as e:
                    if not client_fails or type(e) is not OSError:
                        raise
            return results

    def __enter__(self):
        return HOST, self._port

    def __exit__(self, etype, evalue, traceback):
        wait([self._result])
        if etype is ConnectionAbortedError or etype is ConnectionResetError:
            if self._result.exception() is not None:
                generic = RuntimeError('server-side error')
                raise generic from self._result.exception()
            return False
        self._result_out = self._result.result()
        return False
