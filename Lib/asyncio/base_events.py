"""Base implementation of event loop.

The event loop can be broken up into a multiplexer (the part
responsible for notifying us of IO events) and the event loop proper,
which wraps a multiplexer with functionality for scheduling callbacks,
immediately or at a given time in the future.

Whenever a public API takes a callback, subsequent positional
arguments will be passed to the callback if/when it is called.  This
avoids the proliferation of trivial lambdas implementing closures.
Keyword arguments for the callback are not supported; this is a
conscious design decision, leaving the door open for keyword arguments
to modify the meaning of the API call itself.
"""


import collections
import concurrent.futures
import heapq
import logging
import socket
import subprocess
import time
import os
import sys

from . import events
from . import futures
from . import tasks
from .log import logger


__all__ = ['BaseEventLoop', 'Server']


# Argument for default thread pool executor creation.
_MAX_WORKERS = 5


class _StopError(BaseException):
    """Raised to stop the event loop."""


def _check_resolved_address(sock, address):
    # Ensure that the address is already resolved to avoid the trap of hanging
    # the entire event loop when the address requires doing a DNS lookup.
    family = sock.family
    if family == socket.AF_INET:
        host, port = address
    elif family == socket.AF_INET6:
        host, port = address[:2]
    else:
        return

    type_mask = 0
    if hasattr(socket, 'SOCK_NONBLOCK'):
        type_mask |= socket.SOCK_NONBLOCK
    if hasattr(socket, 'SOCK_CLOEXEC'):
        type_mask |= socket.SOCK_CLOEXEC
    # Use getaddrinfo(AI_NUMERICHOST) to ensure that the address is
    # already resolved.
    try:
        socket.getaddrinfo(host, port,
                           family=family,
                           type=(sock.type & ~type_mask),
                           proto=sock.proto,
                           flags=socket.AI_NUMERICHOST)
    except socket.gaierror as err:
        raise ValueError("address must be resolved (IP address), got %r: %s"
                         % (address, err))

def _raise_stop_error(*args):
    raise _StopError


class Server(events.AbstractServer):

    def __init__(self, loop, sockets):
        self.loop = loop
        self.sockets = sockets
        self.active_count = 0
        self.waiters = []

    def attach(self, transport):
        assert self.sockets is not None
        self.active_count += 1

    def detach(self, transport):
        assert self.active_count > 0
        self.active_count -= 1
        if self.active_count == 0 and self.sockets is None:
            self._wakeup()

    def close(self):
        sockets = self.sockets
        if sockets is not None:
            self.sockets = None
            for sock in sockets:
                self.loop._stop_serving(sock)
            if self.active_count == 0:
                self._wakeup()

    def _wakeup(self):
        waiters = self.waiters
        self.waiters = None
        for waiter in waiters:
            if not waiter.done():
                waiter.set_result(waiter)

    @tasks.coroutine
    def wait_closed(self):
        if self.sockets is None or self.waiters is None:
            return
        waiter = futures.Future(loop=self.loop)
        self.waiters.append(waiter)
        yield from waiter


class BaseEventLoop(events.AbstractEventLoop):

    def __init__(self):
        self._ready = collections.deque()
        self._scheduled = []
        self._default_executor = None
        self._internal_fds = 0
        self._running = False
        self._clock_resolution = time.get_clock_info('monotonic').resolution
        self._exception_handler = None
        self._debug = False

    def _make_socket_transport(self, sock, protocol, waiter=None, *,
                               extra=None, server=None):
        """Create socket transport."""
        raise NotImplementedError

    def _make_ssl_transport(self, rawsock, protocol, sslcontext, waiter, *,
                            server_side=False, server_hostname=None,
                            extra=None, server=None):
        """Create SSL transport."""
        raise NotImplementedError

    def _make_datagram_transport(self, sock, protocol,
                                 address=None, extra=None):
        """Create datagram transport."""
        raise NotImplementedError

    def _make_read_pipe_transport(self, pipe, protocol, waiter=None,
                                  extra=None):
        """Create read pipe transport."""
        raise NotImplementedError

    def _make_write_pipe_transport(self, pipe, protocol, waiter=None,
                                   extra=None):
        """Create write pipe transport."""
        raise NotImplementedError

    @tasks.coroutine
    def _make_subprocess_transport(self, protocol, args, shell,
                                   stdin, stdout, stderr, bufsize,
                                   extra=None, **kwargs):
        """Create subprocess transport."""
        raise NotImplementedError

    def _read_from_self(self):
        """XXX"""
        raise NotImplementedError

    def _write_to_self(self):
        """XXX"""
        raise NotImplementedError

    def _process_events(self, event_list):
        """Process selector events."""
        raise NotImplementedError

    def run_forever(self):
        """Run until stop() is called."""
        if self._running:
            raise RuntimeError('Event loop is running.')
        self._running = True
        try:
            while True:
                try:
                    self._run_once()
                except _StopError:
                    break
        finally:
            self._running = False

    def run_until_complete(self, future):
        """Run until the Future is done.

        If the argument is a coroutine, it is wrapped in a Task.

        XXX TBD: It would be disastrous to call run_until_complete()
        with the same coroutine twice -- it would wrap it in two
        different Tasks and that can't be good.

        Return the Future's result, or raise its exception.
        """
        future = tasks.async(future, loop=self)
        future.add_done_callback(_raise_stop_error)
        self.run_forever()
        future.remove_done_callback(_raise_stop_error)
        if not future.done():
            raise RuntimeError('Event loop stopped before Future completed.')

        return future.result()

    def stop(self):
        """Stop running the event loop.

        Every callback scheduled before stop() is called will run.
        Callback scheduled after stop() is called won't.  However,
        those callbacks will run if run() is called again later.
        """
        self.call_soon(_raise_stop_error)

    def close(self):
        """Close the event loop.

        This clears the queues and shuts down the executor,
        but does not wait for the executor to finish.
        """
        self._ready.clear()
        self._scheduled.clear()
        executor = self._default_executor
        if executor is not None:
            self._default_executor = None
            executor.shutdown(wait=False)

    def is_running(self):
        """Returns running status of event loop."""
        return self._running

    def time(self):
        """Return the time according to the event loop's clock."""
        return time.monotonic()

    def call_later(self, delay, callback, *args):
        """Arrange for a callback to be called at a given time.

        Return a Handle: an opaque object with a cancel() method that
        can be used to cancel the call.

        The delay can be an int or float, expressed in seconds.  It is
        always a relative time.

        Each callback will be called exactly once.  If two callbacks
        are scheduled for exactly the same time, it undefined which
        will be called first.

        Any positional arguments after the callback will be passed to
        the callback when it is called.
        """
        return self.call_at(self.time() + delay, callback, *args)

    def call_at(self, when, callback, *args):
        """Like call_later(), but uses an absolute time."""
        if tasks.iscoroutinefunction(callback):
            raise TypeError("coroutines cannot be used with call_at()")
        timer = events.TimerHandle(when, callback, args, self)
        heapq.heappush(self._scheduled, timer)
        return timer

    def call_soon(self, callback, *args):
        """Arrange for a callback to be called as soon as possible.

        This operates as a FIFO queue, callbacks are called in the
        order in which they are registered.  Each callback will be
        called exactly once.

        Any positional arguments after the callback will be passed to
        the callback when it is called.
        """
        if tasks.iscoroutinefunction(callback):
            raise TypeError("coroutines cannot be used with call_soon()")
        handle = events.Handle(callback, args, self)
        self._ready.append(handle)
        return handle

    def call_soon_threadsafe(self, callback, *args):
        """XXX"""
        handle = self.call_soon(callback, *args)
        self._write_to_self()
        return handle

    def run_in_executor(self, executor, callback, *args):
        if tasks.iscoroutinefunction(callback):
            raise TypeError("coroutines cannot be used with run_in_executor()")
        if isinstance(callback, events.Handle):
            assert not args
            assert not isinstance(callback, events.TimerHandle)
            if callback._cancelled:
                f = futures.Future(loop=self)
                f.set_result(None)
                return f
            callback, args = callback._callback, callback._args
        if executor is None:
            executor = self._default_executor
            if executor is None:
                executor = concurrent.futures.ThreadPoolExecutor(_MAX_WORKERS)
                self._default_executor = executor
        return futures.wrap_future(executor.submit(callback, *args), loop=self)

    def set_default_executor(self, executor):
        self._default_executor = executor

    def getaddrinfo(self, host, port, *,
                    family=0, type=0, proto=0, flags=0):
        return self.run_in_executor(None, socket.getaddrinfo,
                                    host, port, family, type, proto, flags)

    def getnameinfo(self, sockaddr, flags=0):
        return self.run_in_executor(None, socket.getnameinfo, sockaddr, flags)

    @tasks.coroutine
    def create_connection(self, protocol_factory, host=None, port=None, *,
                          ssl=None, family=0, proto=0, flags=0, sock=None,
                          local_addr=None, server_hostname=None):
        """XXX"""
        if server_hostname is not None and not ssl:
            raise ValueError('server_hostname is only meaningful with ssl')

        if server_hostname is None and ssl:
            # Use host as default for server_hostname.  It is an error
            # if host is empty or not set, e.g. when an
            # already-connected socket was passed or when only a port
            # is given.  To avoid this error, you can pass
            # server_hostname='' -- this will bypass the hostname
            # check.  (This also means that if host is a numeric
            # IP/IPv6 address, we will attempt to verify that exact
            # address; this will probably fail, but it is possible to
            # create a certificate for a specific IP address, so we
            # don't judge it here.)
            if not host:
                raise ValueError('You must set server_hostname '
                                 'when using ssl without a host')
            server_hostname = host

        if host is not None or port is not None:
            if sock is not None:
                raise ValueError(
                    'host/port and sock can not be specified at the same time')

            f1 = self.getaddrinfo(
                host, port, family=family,
                type=socket.SOCK_STREAM, proto=proto, flags=flags)
            fs = [f1]
            if local_addr is not None:
                f2 = self.getaddrinfo(
                    *local_addr, family=family,
                    type=socket.SOCK_STREAM, proto=proto, flags=flags)
                fs.append(f2)
            else:
                f2 = None

            yield from tasks.wait(fs, loop=self)

            infos = f1.result()
            if not infos:
                raise OSError('getaddrinfo() returned empty list')
            if f2 is not None:
                laddr_infos = f2.result()
                if not laddr_infos:
                    raise OSError('getaddrinfo() returned empty list')

            exceptions = []
            for family, type, proto, cname, address in infos:
                try:
                    sock = socket.socket(family=family, type=type, proto=proto)
                    sock.setblocking(False)
                    if f2 is not None:
                        for _, _, _, _, laddr in laddr_infos:
                            try:
                                sock.bind(laddr)
                                break
                            except OSError as exc:
                                exc = OSError(
                                    exc.errno, 'error while '
                                    'attempting to bind on address '
                                    '{!r}: {}'.format(
                                        laddr, exc.strerror.lower()))
                                exceptions.append(exc)
                        else:
                            sock.close()
                            sock = None
                            continue
                    yield from self.sock_connect(sock, address)
                except OSError as exc:
                    if sock is not None:
                        sock.close()
                    exceptions.append(exc)
                else:
                    break
            else:
                if len(exceptions) == 1:
                    raise exceptions[0]
                else:
                    # If they all have the same str(), raise one.
                    model = str(exceptions[0])
                    if all(str(exc) == model for exc in exceptions):
                        raise exceptions[0]
                    # Raise a combined exception so the user can see all
                    # the various error messages.
                    raise OSError('Multiple exceptions: {}'.format(
                        ', '.join(str(exc) for exc in exceptions)))

        elif sock is None:
            raise ValueError(
                'host and port was not specified and no sock specified')

        sock.setblocking(False)

        transport, protocol = yield from self._create_connection_transport(
            sock, protocol_factory, ssl, server_hostname)
        return transport, protocol

    @tasks.coroutine
    def _create_connection_transport(self, sock, protocol_factory, ssl,
                                     server_hostname):
        protocol = protocol_factory()
        waiter = futures.Future(loop=self)
        if ssl:
            sslcontext = None if isinstance(ssl, bool) else ssl
            transport = self._make_ssl_transport(
                sock, protocol, sslcontext, waiter,
                server_side=False, server_hostname=server_hostname)
        else:
            transport = self._make_socket_transport(sock, protocol, waiter)

        yield from waiter
        return transport, protocol

    @tasks.coroutine
    def create_datagram_endpoint(self, protocol_factory,
                                 local_addr=None, remote_addr=None, *,
                                 family=0, proto=0, flags=0):
        """Create datagram connection."""
        if not (local_addr or remote_addr):
            if family == 0:
                raise ValueError('unexpected address family')
            addr_pairs_info = (((family, proto), (None, None)),)
        else:
            # join addresss by (family, protocol)
            addr_infos = collections.OrderedDict()
            for idx, addr in ((0, local_addr), (1, remote_addr)):
                if addr is not None:
                    assert isinstance(addr, tuple) and len(addr) == 2, (
                        '2-tuple is expected')

                    infos = yield from self.getaddrinfo(
                        *addr, family=family, type=socket.SOCK_DGRAM,
                        proto=proto, flags=flags)
                    if not infos:
                        raise OSError('getaddrinfo() returned empty list')

                    for fam, _, pro, _, address in infos:
                        key = (fam, pro)
                        if key not in addr_infos:
                            addr_infos[key] = [None, None]
                        addr_infos[key][idx] = address

            # each addr has to have info for each (family, proto) pair
            addr_pairs_info = [
                (key, addr_pair) for key, addr_pair in addr_infos.items()
                if not ((local_addr and addr_pair[0] is None) or
                        (remote_addr and addr_pair[1] is None))]

            if not addr_pairs_info:
                raise ValueError('can not get address information')

        exceptions = []

        for ((family, proto),
             (local_address, remote_address)) in addr_pairs_info:
            sock = None
            r_addr = None
            try:
                sock = socket.socket(
                    family=family, type=socket.SOCK_DGRAM, proto=proto)
                sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                sock.setblocking(False)

                if local_addr:
                    sock.bind(local_address)
                if remote_addr:
                    yield from self.sock_connect(sock, remote_address)
                    r_addr = remote_address
            except OSError as exc:
                if sock is not None:
                    sock.close()
                exceptions.append(exc)
            else:
                break
        else:
            raise exceptions[0]

        protocol = protocol_factory()
        transport = self._make_datagram_transport(sock, protocol, r_addr)
        return transport, protocol

    @tasks.coroutine
    def create_server(self, protocol_factory, host=None, port=None,
                      *,
                      family=socket.AF_UNSPEC,
                      flags=socket.AI_PASSIVE,
                      sock=None,
                      backlog=100,
                      ssl=None,
                      reuse_address=None):
        """XXX"""
        if isinstance(ssl, bool):
            raise TypeError('ssl argument must be an SSLContext or None')
        if host is not None or port is not None:
            if sock is not None:
                raise ValueError(
                    'host/port and sock can not be specified at the same time')

            AF_INET6 = getattr(socket, 'AF_INET6', 0)
            if reuse_address is None:
                reuse_address = os.name == 'posix' and sys.platform != 'cygwin'
            sockets = []
            if host == '':
                host = None

            infos = yield from self.getaddrinfo(
                host, port, family=family,
                type=socket.SOCK_STREAM, proto=0, flags=flags)
            if not infos:
                raise OSError('getaddrinfo() returned empty list')

            completed = False
            try:
                for res in infos:
                    af, socktype, proto, canonname, sa = res
                    try:
                        sock = socket.socket(af, socktype, proto)
                    except socket.error:
                        # Assume it's a bad family/type/protocol combination.
                        continue
                    sockets.append(sock)
                    if reuse_address:
                        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR,
                                        True)
                    # Disable IPv4/IPv6 dual stack support (enabled by
                    # default on Linux) which makes a single socket
                    # listen on both address families.
                    if af == AF_INET6 and hasattr(socket, 'IPPROTO_IPV6'):
                        sock.setsockopt(socket.IPPROTO_IPV6,
                                        socket.IPV6_V6ONLY,
                                        True)
                    try:
                        sock.bind(sa)
                    except OSError as err:
                        raise OSError(err.errno, 'error while attempting '
                                      'to bind on address %r: %s'
                                      % (sa, err.strerror.lower()))
                completed = True
            finally:
                if not completed:
                    for sock in sockets:
                        sock.close()
        else:
            if sock is None:
                raise ValueError(
                    'host and port was not specified and no sock specified')
            sockets = [sock]

        server = Server(self, sockets)
        for sock in sockets:
            sock.listen(backlog)
            sock.setblocking(False)
            self._start_serving(protocol_factory, sock, ssl, server)
        return server

    @tasks.coroutine
    def connect_read_pipe(self, protocol_factory, pipe):
        protocol = protocol_factory()
        waiter = futures.Future(loop=self)
        transport = self._make_read_pipe_transport(pipe, protocol, waiter)
        yield from waiter
        return transport, protocol

    @tasks.coroutine
    def connect_write_pipe(self, protocol_factory, pipe):
        protocol = protocol_factory()
        waiter = futures.Future(loop=self)
        transport = self._make_write_pipe_transport(pipe, protocol, waiter)
        yield from waiter
        return transport, protocol

    @tasks.coroutine
    def subprocess_shell(self, protocol_factory, cmd, *, stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                         universal_newlines=False, shell=True, bufsize=0,
                         **kwargs):
        if not isinstance(cmd, (bytes, str)):
            raise ValueError("cmd must be a string")
        if universal_newlines:
            raise ValueError("universal_newlines must be False")
        if not shell:
            raise ValueError("shell must be True")
        if bufsize != 0:
            raise ValueError("bufsize must be 0")
        protocol = protocol_factory()
        transport = yield from self._make_subprocess_transport(
            protocol, cmd, True, stdin, stdout, stderr, bufsize, **kwargs)
        return transport, protocol

    @tasks.coroutine
    def subprocess_exec(self, protocol_factory, program, *args,
                        stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                        stderr=subprocess.PIPE, universal_newlines=False,
                        shell=False, bufsize=0, **kwargs):
        if universal_newlines:
            raise ValueError("universal_newlines must be False")
        if shell:
            raise ValueError("shell must be False")
        if bufsize != 0:
            raise ValueError("bufsize must be 0")
        popen_args = (program,) + args
        for arg in popen_args:
            if not isinstance(arg, (str, bytes)):
                raise TypeError("program arguments must be "
                                "a bytes or text string, not %s"
                                % type(arg).__name__)
        protocol = protocol_factory()
        transport = yield from self._make_subprocess_transport(
            protocol, popen_args, False, stdin, stdout, stderr,
            bufsize, **kwargs)
        return transport, protocol

    def set_exception_handler(self, handler):
        """Set handler as the new event loop exception handler.

        If handler is None, the default exception handler will
        be set.

        If handler is a callable object, it should have a
        matching signature to '(loop, context)', where 'loop'
        will be a reference to the active event loop, 'context'
        will be a dict object (see `call_exception_handler()`
        documentation for details about context).
        """
        if handler is not None and not callable(handler):
            raise TypeError('A callable object or None is expected, '
                            'got {!r}'.format(handler))
        self._exception_handler = handler

    def default_exception_handler(self, context):
        """Default exception handler.

        This is called when an exception occurs and no exception
        handler is set, and can be called by a custom exception
        handler that wants to defer to the default behavior.

        context parameter has the same meaning as in
        `call_exception_handler()`.
        """
        message = context.get('message')
        if not message:
            message = 'Unhandled exception in event loop'

        exception = context.get('exception')
        if exception is not None:
            exc_info = (type(exception), exception, exception.__traceback__)
        else:
            exc_info = False

        log_lines = [message]
        for key in sorted(context):
            if key in {'message', 'exception'}:
                continue
            log_lines.append('{}: {!r}'.format(key, context[key]))

        logger.error('\n'.join(log_lines), exc_info=exc_info)

    def call_exception_handler(self, context):
        """Call the current event loop exception handler.

        context is a dict object containing the following keys
        (new keys maybe introduced later):
        - 'message': Error message;
        - 'exception' (optional): Exception object;
        - 'future' (optional): Future instance;
        - 'handle' (optional): Handle instance;
        - 'protocol' (optional): Protocol instance;
        - 'transport' (optional): Transport instance;
        - 'socket' (optional): Socket instance.

        Note: this method should not be overloaded in subclassed
        event loops.  For any custom exception handling, use
        `set_exception_handler()` method.
        """
        if self._exception_handler is None:
            try:
                self.default_exception_handler(context)
            except Exception:
                # Second protection layer for unexpected errors
                # in the default implementation, as well as for subclassed
                # event loops with overloaded "default_exception_handler".
                logger.error('Exception in default exception handler',
                             exc_info=True)
        else:
            try:
                self._exception_handler(self, context)
            except Exception as exc:
                # Exception in the user set custom exception handler.
                try:
                    # Let's try default handler.
                    self.default_exception_handler({
                        'message': 'Unhandled error in exception handler',
                        'exception': exc,
                        'context': context,
                    })
                except Exception:
                    # Guard 'default_exception_handler' in case it's
                    # overloaded.
                    logger.error('Exception in default exception handler '
                                 'while handling an unexpected error '
                                 'in custom exception handler',
                                 exc_info=True)

    def _add_callback(self, handle):
        """Add a Handle to ready or scheduled."""
        assert isinstance(handle, events.Handle), 'A Handle is required here'
        if handle._cancelled:
            return
        if isinstance(handle, events.TimerHandle):
            heapq.heappush(self._scheduled, handle)
        else:
            self._ready.append(handle)

    def _add_callback_signalsafe(self, handle):
        """Like _add_callback() but called from a signal handler."""
        self._add_callback(handle)
        self._write_to_self()

    def _run_once(self):
        """Run one full iteration of the event loop.

        This calls all currently ready callbacks, polls for I/O,
        schedules the resulting callbacks, and finally schedules
        'call_later' callbacks.
        """
        # Remove delayed calls that were cancelled from head of queue.
        while self._scheduled and self._scheduled[0]._cancelled:
            heapq.heappop(self._scheduled)

        timeout = None
        if self._ready:
            timeout = 0
        elif self._scheduled:
            # Compute the desired timeout.
            when = self._scheduled[0]._when
            deadline = max(0, when - self.time())
            if timeout is None:
                timeout = deadline
            else:
                timeout = min(timeout, deadline)

        # TODO: Instrumentation only in debug mode?
        if logger.isEnabledFor(logging.INFO):
            t0 = self.time()
            event_list = self._selector.select(timeout)
            t1 = self.time()
            if t1-t0 >= 1:
                level = logging.INFO
            else:
                level = logging.DEBUG
            if timeout is not None:
                logger.log(level, 'poll %.3f took %.3f seconds',
                           timeout, t1-t0)
            else:
                logger.log(level, 'poll took %.3f seconds', t1-t0)
        else:
            event_list = self._selector.select(timeout)
        self._process_events(event_list)

        # Handle 'later' callbacks that are ready.
        end_time = self.time() + self._clock_resolution
        while self._scheduled:
            handle = self._scheduled[0]
            if handle._when >= end_time:
                break
            handle = heapq.heappop(self._scheduled)
            self._ready.append(handle)

        # This is the only place where callbacks are actually *called*.
        # All other places just add them to ready.
        # Note: We run all currently scheduled callbacks, but not any
        # callbacks scheduled by callbacks run this time around --
        # they will be run the next time (after another I/O poll).
        # Use an idiom that is threadsafe without using locks.
        ntodo = len(self._ready)
        for i in range(ntodo):
            handle = self._ready.popleft()
            if not handle._cancelled:
                handle._run()
        handle = None  # Needed to break cycles when an exception occurs.

    def get_debug(self):
        return self._debug

    def set_debug(self, enabled):
        self._debug = enabled
