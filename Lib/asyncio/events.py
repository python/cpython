"""Event loop and event loop policy."""

__all__ = ['AbstractEventLoopPolicy',
           'AbstractEventLoop', 'AbstractServer',
           'Handle', 'TimerHandle',
           'get_event_loop_policy', 'set_event_loop_policy',
           'get_event_loop', 'set_event_loop', 'new_event_loop',
           'get_child_watcher', 'set_child_watcher',
           ]

import subprocess
import threading
import socket


class Handle:
    """Object returned by callback registration methods."""

    __slots__ = ['_callback', '_args', '_cancelled', '_loop']

    def __init__(self, callback, args, loop):
        assert not isinstance(callback, Handle), 'A Handle is not a callback'
        self._loop = loop
        self._callback = callback
        self._args = args
        self._cancelled = False

    def __repr__(self):
        res = 'Handle({}, {})'.format(self._callback, self._args)
        if self._cancelled:
            res += '<cancelled>'
        return res

    def cancel(self):
        self._cancelled = True

    def _run(self):
        try:
            self._callback(*self._args)
        except Exception as exc:
            msg = 'Exception in callback {}{!r}'.format(self._callback,
                                                        self._args)
            self._loop.call_exception_handler({
                'message': msg,
                'exception': exc,
                'handle': self,
            })
        self = None  # Needed to break cycles when an exception occurs.


class TimerHandle(Handle):
    """Object returned by timed callback registration methods."""

    __slots__ = ['_when']

    def __init__(self, when, callback, args, loop):
        assert when is not None
        super().__init__(callback, args, loop)

        self._when = when

    def __repr__(self):
        res = 'TimerHandle({}, {}, {})'.format(self._when,
                                               self._callback,
                                               self._args)
        if self._cancelled:
            res += '<cancelled>'

        return res

    def __hash__(self):
        return hash(self._when)

    def __lt__(self, other):
        return self._when < other._when

    def __le__(self, other):
        if self._when < other._when:
            return True
        return self.__eq__(other)

    def __gt__(self, other):
        return self._when > other._when

    def __ge__(self, other):
        if self._when > other._when:
            return True
        return self.__eq__(other)

    def __eq__(self, other):
        if isinstance(other, TimerHandle):
            return (self._when == other._when and
                    self._callback == other._callback and
                    self._args == other._args and
                    self._cancelled == other._cancelled)
        return NotImplemented

    def __ne__(self, other):
        equal = self.__eq__(other)
        return NotImplemented if equal is NotImplemented else not equal


class AbstractServer:
    """Abstract server returned by create_server()."""

    def close(self):
        """Stop serving.  This leaves existing connections open."""
        return NotImplemented

    def wait_closed(self):
        """Coroutine to wait until service is closed."""
        return NotImplemented


class AbstractEventLoop:
    """Abstract event loop."""

    # Running and stopping the event loop.

    def run_forever(self):
        """Run the event loop until stop() is called."""
        raise NotImplementedError

    def run_until_complete(self, future):
        """Run the event loop until a Future is done.

        Return the Future's result, or raise its exception.
        """
        raise NotImplementedError

    def stop(self):
        """Stop the event loop as soon as reasonable.

        Exactly how soon that is may depend on the implementation, but
        no more I/O callbacks should be scheduled.
        """
        raise NotImplementedError

    def is_running(self):
        """Return whether the event loop is currently running."""
        raise NotImplementedError

    def close(self):
        """Close the loop.

        The loop should not be running.

        This is idempotent and irreversible.

        No other methods should be called after this one.
        """
        raise NotImplementedError

    # Methods scheduling callbacks.  All these return Handles.

    def call_soon(self, callback, *args):
        return self.call_later(0, callback, *args)

    def call_later(self, delay, callback, *args):
        raise NotImplementedError

    def call_at(self, when, callback, *args):
        raise NotImplementedError

    def time(self):
        raise NotImplementedError

    # Methods for interacting with threads.

    def call_soon_threadsafe(self, callback, *args):
        raise NotImplementedError

    def run_in_executor(self, executor, callback, *args):
        raise NotImplementedError

    def set_default_executor(self, executor):
        raise NotImplementedError

    # Network I/O methods returning Futures.

    def getaddrinfo(self, host, port, *, family=0, type=0, proto=0, flags=0):
        raise NotImplementedError

    def getnameinfo(self, sockaddr, flags=0):
        raise NotImplementedError

    def create_connection(self, protocol_factory, host=None, port=None, *,
                          ssl=None, family=0, proto=0, flags=0, sock=None,
                          local_addr=None, server_hostname=None):
        raise NotImplementedError

    def create_server(self, protocol_factory, host=None, port=None, *,
                      family=socket.AF_UNSPEC, flags=socket.AI_PASSIVE,
                      sock=None, backlog=100, ssl=None, reuse_address=None):
        """A coroutine which creates a TCP server bound to host and port.

        The return value is a Server object which can be used to stop
        the service.

        If host is an empty string or None all interfaces are assumed
        and a list of multiple sockets will be returned (most likely
        one for IPv4 and another one for IPv6).

        family can be set to either AF_INET or AF_INET6 to force the
        socket to use IPv4 or IPv6. If not set it will be determined
        from host (defaults to AF_UNSPEC).

        flags is a bitmask for getaddrinfo().

        sock can optionally be specified in order to use a preexisting
        socket object.

        backlog is the maximum number of queued connections passed to
        listen() (defaults to 100).

        ssl can be set to an SSLContext to enable SSL over the
        accepted connections.

        reuse_address tells the kernel to reuse a local socket in
        TIME_WAIT state, without waiting for its natural timeout to
        expire. If not specified will automatically be set to True on
        UNIX.
        """
        raise NotImplementedError

    def create_unix_connection(self, protocol_factory, path, *,
                               ssl=None, sock=None,
                               server_hostname=None):
        raise NotImplementedError

    def create_unix_server(self, protocol_factory, path, *,
                           sock=None, backlog=100, ssl=None):
        """A coroutine which creates a UNIX Domain Socket server.

        The return value is a Server object, which can be used to stop
        the service.

        path is a str, representing a file systsem path to bind the
        server socket to.

        sock can optionally be specified in order to use a preexisting
        socket object.

        backlog is the maximum number of queued connections passed to
        listen() (defaults to 100).

        ssl can be set to an SSLContext to enable SSL over the
        accepted connections.
        """
        raise NotImplementedError

    def create_datagram_endpoint(self, protocol_factory,
                                 local_addr=None, remote_addr=None, *,
                                 family=0, proto=0, flags=0):
        raise NotImplementedError

    # Pipes and subprocesses.

    def connect_read_pipe(self, protocol_factory, pipe):
        """Register read pipe in event loop.

        protocol_factory should instantiate object with Protocol interface.
        pipe is file-like object already switched to nonblocking.
        Return pair (transport, protocol), where transport support
        ReadTransport interface."""
        # The reason to accept file-like object instead of just file descriptor
        # is: we need to own pipe and close it at transport finishing
        # Can got complicated errors if pass f.fileno(),
        # close fd in pipe transport then close f and vise versa.
        raise NotImplementedError

    def connect_write_pipe(self, protocol_factory, pipe):
        """Register write pipe in event loop.

        protocol_factory should instantiate object with BaseProtocol interface.
        Pipe is file-like object already switched to nonblocking.
        Return pair (transport, protocol), where transport support
        WriteTransport interface."""
        # The reason to accept file-like object instead of just file descriptor
        # is: we need to own pipe and close it at transport finishing
        # Can got complicated errors if pass f.fileno(),
        # close fd in pipe transport then close f and vise versa.
        raise NotImplementedError

    def subprocess_shell(self, protocol_factory, cmd, *, stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                         **kwargs):
        raise NotImplementedError

    def subprocess_exec(self, protocol_factory, *args, stdin=subprocess.PIPE,
                        stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                        **kwargs):
        raise NotImplementedError

    # Ready-based callback registration methods.
    # The add_*() methods return None.
    # The remove_*() methods return True if something was removed,
    # False if there was nothing to delete.

    def add_reader(self, fd, callback, *args):
        raise NotImplementedError

    def remove_reader(self, fd):
        raise NotImplementedError

    def add_writer(self, fd, callback, *args):
        raise NotImplementedError

    def remove_writer(self, fd):
        raise NotImplementedError

    # Completion based I/O methods returning Futures.

    def sock_recv(self, sock, nbytes):
        raise NotImplementedError

    def sock_sendall(self, sock, data):
        raise NotImplementedError

    def sock_connect(self, sock, address):
        raise NotImplementedError

    def sock_accept(self, sock):
        raise NotImplementedError

    # Signal handling.

    def add_signal_handler(self, sig, callback, *args):
        raise NotImplementedError

    def remove_signal_handler(self, sig):
        raise NotImplementedError

    # Error handlers.

    def set_exception_handler(self, handler):
        raise NotImplementedError

    def default_exception_handler(self, context):
        raise NotImplementedError

    def call_exception_handler(self, context):
        raise NotImplementedError

    # Debug flag management.

    def get_debug(self):
        raise NotImplementedError

    def set_debug(self, enabled):
        raise NotImplementedError


class AbstractEventLoopPolicy:
    """Abstract policy for accessing the event loop."""

    def get_event_loop(self):
        """XXX"""
        raise NotImplementedError

    def set_event_loop(self, loop):
        """XXX"""
        raise NotImplementedError

    def new_event_loop(self):
        """XXX"""
        raise NotImplementedError

    # Child processes handling (Unix only).

    def get_child_watcher(self):
        """XXX"""
        raise NotImplementedError

    def set_child_watcher(self, watcher):
        """XXX"""
        raise NotImplementedError


class BaseDefaultEventLoopPolicy(AbstractEventLoopPolicy):
    """Default policy implementation for accessing the event loop.

    In this policy, each thread has its own event loop.  However, we
    only automatically create an event loop by default for the main
    thread; other threads by default have no event loop.

    Other policies may have different rules (e.g. a single global
    event loop, or automatically creating an event loop per thread, or
    using some other notion of context to which an event loop is
    associated).
    """

    _loop_factory = None

    class _Local(threading.local):
        _loop = None
        _set_called = False

    def __init__(self):
        self._local = self._Local()

    def get_event_loop(self):
        """Get the event loop.

        This may be None or an instance of EventLoop.
        """
        if (self._local._loop is None and
            not self._local._set_called and
            isinstance(threading.current_thread(), threading._MainThread)):
            self.set_event_loop(self.new_event_loop())
        assert self._local._loop is not None, \
               ('There is no current event loop in thread %r.' %
                threading.current_thread().name)
        return self._local._loop

    def set_event_loop(self, loop):
        """Set the event loop."""
        self._local._set_called = True
        assert loop is None or isinstance(loop, AbstractEventLoop)
        self._local._loop = loop

    def new_event_loop(self):
        """Create a new event loop.

        You must call set_event_loop() to make this the current event
        loop.
        """
        return self._loop_factory()


# Event loop policy.  The policy itself is always global, even if the
# policy's rules say that there is an event loop per thread (or other
# notion of context).  The default policy is installed by the first
# call to get_event_loop_policy().
_event_loop_policy = None

# Lock for protecting the on-the-fly creation of the event loop policy.
_lock = threading.Lock()


def _init_event_loop_policy():
    global _event_loop_policy
    with _lock:
        if _event_loop_policy is None:  # pragma: no branch
            from . import DefaultEventLoopPolicy
            _event_loop_policy = DefaultEventLoopPolicy()


def get_event_loop_policy():
    """XXX"""
    if _event_loop_policy is None:
        _init_event_loop_policy()
    return _event_loop_policy


def set_event_loop_policy(policy):
    """XXX"""
    global _event_loop_policy
    assert policy is None or isinstance(policy, AbstractEventLoopPolicy)
    _event_loop_policy = policy


def get_event_loop():
    """XXX"""
    return get_event_loop_policy().get_event_loop()


def set_event_loop(loop):
    """XXX"""
    get_event_loop_policy().set_event_loop(loop)


def new_event_loop():
    """XXX"""
    return get_event_loop_policy().new_event_loop()


def get_child_watcher():
    """XXX"""
    return get_event_loop_policy().get_child_watcher()


def set_child_watcher(watcher):
    """XXX"""
    return get_event_loop_policy().set_child_watcher(watcher)
