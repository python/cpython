"""Base implementation of event loop.

The event loop can be broken up into a multiplexer (the part
responsible for notifying us of I/O events) and the event loop proper,
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
import inspect
import itertools
import logging
import os
import socket
import subprocess
import threading
import time
import traceback
import sys
import warnings

from . import compat
from . import coroutines
from . import events
from . import futures
from . import tasks
from .coroutines import coroutine
from .log import logger


__all__ = ['BaseEventLoop']


# Argument for default thread pool executor creation.
_MAX_WORKERS = 5

# Minimum number of _scheduled timer handles before cleanup of
# cancelled handles is performed.
_MIN_SCHEDULED_TIMER_HANDLES = 100

# Minimum fraction of _scheduled timer handles that are cancelled
# before cleanup of cancelled handles is performed.
_MIN_CANCELLED_TIMER_HANDLES_FRACTION = 0.5

# Exceptions which must not call the exception handler in fatal error
# methods (_fatal_error())
_FATAL_ERROR_IGNORE = (BrokenPipeError,
                       ConnectionResetError, ConnectionAbortedError)


def _format_handle(handle):
    cb = handle._callback
    if inspect.ismethod(cb) and isinstance(cb.__self__, tasks.Task):
        # format the task
        return repr(cb.__self__)
    else:
        return str(handle)


def _format_pipe(fd):
    if fd == subprocess.PIPE:
        return '<pipe>'
    elif fd == subprocess.STDOUT:
        return '<stdout>'
    else:
        return repr(fd)


# Linux's sock.type is a bitmask that can include extra info about socket.
_SOCKET_TYPE_MASK = 0
if hasattr(socket, 'SOCK_NONBLOCK'):
    _SOCKET_TYPE_MASK |= socket.SOCK_NONBLOCK
if hasattr(socket, 'SOCK_CLOEXEC'):
    _SOCKET_TYPE_MASK |= socket.SOCK_CLOEXEC


def _ipaddr_info(host, port, family, type, proto):
    # Try to skip getaddrinfo if "host" is already an IP. Users might have
    # handled name resolution in their own code and pass in resolved IPs.
    if not hasattr(socket, 'inet_pton'):
        return

    if proto not in {0, socket.IPPROTO_TCP, socket.IPPROTO_UDP} or \
            host is None:
        return None

    type &= ~_SOCKET_TYPE_MASK
    if type == socket.SOCK_STREAM:
        proto = socket.IPPROTO_TCP
    elif type == socket.SOCK_DGRAM:
        proto = socket.IPPROTO_UDP
    else:
        return None

    if port is None:
        port = 0
    elif isinstance(port, bytes):
        if port == b'':
            port = 0
        else:
            try:
                port = int(port)
            except ValueError:
                # Might be a service name like b"http".
                port = socket.getservbyname(port.decode('ascii'))
    elif isinstance(port, str):
        if port == '':
            port = 0
        else:
            try:
                port = int(port)
            except ValueError:
                # Might be a service name like "http".
                port = socket.getservbyname(port)

    if family == socket.AF_UNSPEC:
        afs = [socket.AF_INET, socket.AF_INET6]
    else:
        afs = [family]

    if isinstance(host, bytes):
        host = host.decode('idna')
    if '%' in host:
        # Linux's inet_pton doesn't accept an IPv6 zone index after host,
        # like '::1%lo0'.
        return None

    for af in afs:
        try:
            socket.inet_pton(af, host)
            # The host has already been resolved.
            return af, type, proto, '', (host, port)
        except OSError:
            pass

    # "host" is not an IP address.
    return None


def _ensure_resolved(address, *, family=0, type=socket.SOCK_STREAM, proto=0,
                     flags=0, loop):
    host, port = address[:2]
    info = _ipaddr_info(host, port, family, type, proto)
    if info is not None:
        # "host" is already a resolved IP.
        fut = loop.create_future()
        fut.set_result([info])
        return fut
    else:
        return loop.getaddrinfo(host, port, family=family, type=type,
                                proto=proto, flags=flags)


def _run_until_complete_cb(fut):
    exc = fut._exception
    if (isinstance(exc, BaseException)
    and not isinstance(exc, Exception)):
        # Issue #22429: run_forever() already finished, no need to
        # stop it.
        return
    fut._loop.stop()


class Server(events.AbstractServer):

    def __init__(self, loop, sockets):
        self._loop = loop
        self.sockets = sockets
        self._active_count = 0
        self._waiters = []

    def __repr__(self):
        return '<%s sockets=%r>' % (self.__class__.__name__, self.sockets)

    def _attach(self):
        assert self.sockets is not None
        self._active_count += 1

    def _detach(self):
        assert self._active_count > 0
        self._active_count -= 1
        if self._active_count == 0 and self.sockets is None:
            self._wakeup()

    def close(self):
        sockets = self.sockets
        if sockets is None:
            return
        self.sockets = None
        for sock in sockets:
            self._loop._stop_serving(sock)
        if self._active_count == 0:
            self._wakeup()

    def _wakeup(self):
        waiters = self._waiters
        self._waiters = None
        for waiter in waiters:
            if not waiter.done():
                waiter.set_result(waiter)

    @coroutine
    def wait_closed(self):
        if self.sockets is None or self._waiters is None:
            return
        waiter = self._loop.create_future()
        self._waiters.append(waiter)
        yield from waiter


class BaseEventLoop(events.AbstractEventLoop):

    def __init__(self):
        self._timer_cancelled_count = 0
        self._closed = False
        self._stopping = False
        self._ready = collections.deque()
        self._scheduled = []
        self._default_executor = None
        self._internal_fds = 0
        # Identifier of the thread running the event loop, or None if the
        # event loop is not running
        self._thread_id = None
        self._clock_resolution = time.get_clock_info('monotonic').resolution
        self._exception_handler = None
        self.set_debug((not sys.flags.ignore_environment
                        and bool(os.environ.get('PYTHONASYNCIODEBUG'))))
        # In debug mode, if the execution of a callback or a step of a task
        # exceed this duration in seconds, the slow callback/task is logged.
        self.slow_callback_duration = 0.1
        self._current_handle = None
        self._task_factory = None
        self._coroutine_wrapper_set = False

    def __repr__(self):
        return ('<%s running=%s closed=%s debug=%s>'
                % (self.__class__.__name__, self.is_running(),
                   self.is_closed(), self.get_debug()))

    def create_future(self):
        """Create a Future object attached to the loop."""
        return futures.Future(loop=self)

    def create_task(self, coro):
        """Schedule a coroutine object.

        Return a task object.
        """
        self._check_closed()
        if self._task_factory is None:
            task = tasks.Task(coro, loop=self)
            if task._source_traceback:
                del task._source_traceback[-1]
        else:
            task = self._task_factory(self, coro)
        return task

    def set_task_factory(self, factory):
        """Set a task factory that will be used by loop.create_task().

        If factory is None the default task factory will be set.

        If factory is a callable, it should have a signature matching
        '(loop, coro)', where 'loop' will be a reference to the active
        event loop, 'coro' will be a coroutine object.  The callable
        must return a Future.
        """
        if factory is not None and not callable(factory):
            raise TypeError('task factory must be a callable or None')
        self._task_factory = factory

    def get_task_factory(self):
        """Return a task factory, or None if the default one is in use."""
        return self._task_factory

    def _make_socket_transport(self, sock, protocol, waiter=None, *,
                               extra=None, server=None):
        """Create socket transport."""
        raise NotImplementedError

    def _make_ssl_transport(self, rawsock, protocol, sslcontext, waiter=None,
                            *, server_side=False, server_hostname=None,
                            extra=None, server=None):
        """Create SSL transport."""
        raise NotImplementedError

    def _make_datagram_transport(self, sock, protocol,
                                 address=None, waiter=None, extra=None):
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

    @coroutine
    def _make_subprocess_transport(self, protocol, args, shell,
                                   stdin, stdout, stderr, bufsize,
                                   extra=None, **kwargs):
        """Create subprocess transport."""
        raise NotImplementedError

    def _write_to_self(self):
        """Write a byte to self-pipe, to wake up the event loop.

        This may be called from a different thread.

        The subclass is responsible for implementing the self-pipe.
        """
        raise NotImplementedError

    def _process_events(self, event_list):
        """Process selector events."""
        raise NotImplementedError

    def _check_closed(self):
        if self._closed:
            raise RuntimeError('Event loop is closed')

    def run_forever(self):
        """Run until stop() is called."""
        self._check_closed()
        if self.is_running():
            raise RuntimeError('Event loop is running.')
        self._set_coroutine_wrapper(self._debug)
        self._thread_id = threading.get_ident()
        try:
            while True:
                self._run_once()
                if self._stopping:
                    break
        finally:
            self._stopping = False
            self._thread_id = None
            self._set_coroutine_wrapper(False)

    def run_until_complete(self, future):
        """Run until the Future is done.

        If the argument is a coroutine, it is wrapped in a Task.

        WARNING: It would be disastrous to call run_until_complete()
        with the same coroutine twice -- it would wrap it in two
        different Tasks and that can't be good.

        Return the Future's result, or raise its exception.
        """
        self._check_closed()

        new_task = not isinstance(future, futures.Future)
        future = tasks.ensure_future(future, loop=self)
        if new_task:
            # An exception is raised if the future didn't complete, so there
            # is no need to log the "destroy pending task" message
            future._log_destroy_pending = False

        future.add_done_callback(_run_until_complete_cb)
        try:
            self.run_forever()
        except:
            if new_task and future.done() and not future.cancelled():
                # The coroutine raised a BaseException. Consume the exception
                # to not log a warning, the caller doesn't have access to the
                # local task.
                future.exception()
            raise
        future.remove_done_callback(_run_until_complete_cb)
        if not future.done():
            raise RuntimeError('Event loop stopped before Future completed.')

        return future.result()

    def stop(self):
        """Stop running the event loop.

        Every callback already scheduled will still run.  This simply informs
        run_forever to stop looping after a complete iteration.
        """
        self._stopping = True

    def close(self):
        """Close the event loop.

        This clears the queues and shuts down the executor,
        but does not wait for the executor to finish.

        The event loop must not be running.
        """
        if self.is_running():
            raise RuntimeError("Cannot close a running event loop")
        if self._closed:
            return
        if self._debug:
            logger.debug("Close %r", self)
        self._closed = True
        self._ready.clear()
        self._scheduled.clear()
        executor = self._default_executor
        if executor is not None:
            self._default_executor = None
            executor.shutdown(wait=False)

    def is_closed(self):
        """Returns True if the event loop was closed."""
        return self._closed

    # On Python 3.3 and older, objects with a destructor part of a reference
    # cycle are never destroyed. It's not more the case on Python 3.4 thanks
    # to the PEP 442.
    if compat.PY34:
        def __del__(self):
            if not self.is_closed():
                warnings.warn("unclosed event loop %r" % self, ResourceWarning)
                if not self.is_running():
                    self.close()

    def is_running(self):
        """Returns True if the event loop is running."""
        return (self._thread_id is not None)

    def time(self):
        """Return the time according to the event loop's clock.

        This is a float expressed in seconds since an epoch, but the
        epoch, precision, accuracy and drift are unspecified and may
        differ per event loop.
        """
        return time.monotonic()

    def call_later(self, delay, callback, *args):
        """Arrange for a callback to be called at a given time.

        Return a Handle: an opaque object with a cancel() method that
        can be used to cancel the call.

        The delay can be an int or float, expressed in seconds.  It is
        always relative to the current time.

        Each callback will be called exactly once.  If two callbacks
        are scheduled for exactly the same time, it undefined which
        will be called first.

        Any positional arguments after the callback will be passed to
        the callback when it is called.
        """
        timer = self.call_at(self.time() + delay, callback, *args)
        if timer._source_traceback:
            del timer._source_traceback[-1]
        return timer

    def call_at(self, when, callback, *args):
        """Like call_later(), but uses an absolute time.

        Absolute time corresponds to the event loop's time() method.
        """
        if (coroutines.iscoroutine(callback)
        or coroutines.iscoroutinefunction(callback)):
            raise TypeError("coroutines cannot be used with call_at()")
        self._check_closed()
        if self._debug:
            self._check_thread()
        timer = events.TimerHandle(when, callback, args, self)
        if timer._source_traceback:
            del timer._source_traceback[-1]
        heapq.heappush(self._scheduled, timer)
        timer._scheduled = True
        return timer

    def call_soon(self, callback, *args):
        """Arrange for a callback to be called as soon as possible.

        This operates as a FIFO queue: callbacks are called in the
        order in which they are registered.  Each callback will be
        called exactly once.

        Any positional arguments after the callback will be passed to
        the callback when it is called.
        """
        if self._debug:
            self._check_thread()
        handle = self._call_soon(callback, args)
        if handle._source_traceback:
            del handle._source_traceback[-1]
        return handle

    def _call_soon(self, callback, args):
        if (coroutines.iscoroutine(callback)
        or coroutines.iscoroutinefunction(callback)):
            raise TypeError("coroutines cannot be used with call_soon()")
        self._check_closed()
        handle = events.Handle(callback, args, self)
        if handle._source_traceback:
            del handle._source_traceback[-1]
        self._ready.append(handle)
        return handle

    def _check_thread(self):
        """Check that the current thread is the thread running the event loop.

        Non-thread-safe methods of this class make this assumption and will
        likely behave incorrectly when the assumption is violated.

        Should only be called when (self._debug == True).  The caller is
        responsible for checking this condition for performance reasons.
        """
        if self._thread_id is None:
            return
        thread_id = threading.get_ident()
        if thread_id != self._thread_id:
            raise RuntimeError(
                "Non-thread-safe operation invoked on an event loop other "
                "than the current one")

    def call_soon_threadsafe(self, callback, *args):
        """Like call_soon(), but thread-safe."""
        handle = self._call_soon(callback, args)
        if handle._source_traceback:
            del handle._source_traceback[-1]
        self._write_to_self()
        return handle

    def run_in_executor(self, executor, func, *args):
        if (coroutines.iscoroutine(func)
        or coroutines.iscoroutinefunction(func)):
            raise TypeError("coroutines cannot be used with run_in_executor()")
        self._check_closed()
        if isinstance(func, events.Handle):
            assert not args
            assert not isinstance(func, events.TimerHandle)
            if func._cancelled:
                f = self.create_future()
                f.set_result(None)
                return f
            func, args = func._callback, func._args
        if executor is None:
            executor = self._default_executor
            if executor is None:
                executor = concurrent.futures.ThreadPoolExecutor(_MAX_WORKERS)
                self._default_executor = executor
        return futures.wrap_future(executor.submit(func, *args), loop=self)

    def set_default_executor(self, executor):
        self._default_executor = executor

    def _getaddrinfo_debug(self, host, port, family, type, proto, flags):
        msg = ["%s:%r" % (host, port)]
        if family:
            msg.append('family=%r' % family)
        if type:
            msg.append('type=%r' % type)
        if proto:
            msg.append('proto=%r' % proto)
        if flags:
            msg.append('flags=%r' % flags)
        msg = ', '.join(msg)
        logger.debug('Get address info %s', msg)

        t0 = self.time()
        addrinfo = socket.getaddrinfo(host, port, family, type, proto, flags)
        dt = self.time() - t0

        msg = ('Getting address info %s took %.3f ms: %r'
               % (msg, dt * 1e3, addrinfo))
        if dt >= self.slow_callback_duration:
            logger.info(msg)
        else:
            logger.debug(msg)
        return addrinfo

    def getaddrinfo(self, host, port, *,
                    family=0, type=0, proto=0, flags=0):
        if self._debug:
            return self.run_in_executor(None, self._getaddrinfo_debug,
                                        host, port, family, type, proto, flags)
        else:
            return self.run_in_executor(None, socket.getaddrinfo,
                                        host, port, family, type, proto, flags)

    def getnameinfo(self, sockaddr, flags=0):
        return self.run_in_executor(None, socket.getnameinfo, sockaddr, flags)

    @coroutine
    def create_connection(self, protocol_factory, host=None, port=None, *,
                          ssl=None, family=0, proto=0, flags=0, sock=None,
                          local_addr=None, server_hostname=None):
        """Connect to a TCP server.

        Create a streaming transport connection to a given Internet host and
        port: socket family AF_INET or socket.AF_INET6 depending on host (or
        family if specified), socket type SOCK_STREAM. protocol_factory must be
        a callable returning a protocol instance.

        This method is a coroutine which will try to establish the connection
        in the background.  When successful, the coroutine returns a
        (transport, protocol) pair.
        """
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

            f1 = _ensure_resolved((host, port), family=family,
                                  type=socket.SOCK_STREAM, proto=proto,
                                  flags=flags, loop=self)
            fs = [f1]
            if local_addr is not None:
                f2 = _ensure_resolved(local_addr, family=family,
                                      type=socket.SOCK_STREAM, proto=proto,
                                      flags=flags, loop=self)
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
                    if self._debug:
                        logger.debug("connect %r to %r", sock, address)
                    yield from self.sock_connect(sock, address)
                except OSError as exc:
                    if sock is not None:
                        sock.close()
                    exceptions.append(exc)
                except:
                    if sock is not None:
                        sock.close()
                    raise
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
        if self._debug:
            # Get the socket from the transport because SSL transport closes
            # the old socket and creates a new SSL socket
            sock = transport.get_extra_info('socket')
            logger.debug("%r connected to %s:%r: (%r, %r)",
                         sock, host, port, transport, protocol)
        return transport, protocol

    @coroutine
    def _create_connection_transport(self, sock, protocol_factory, ssl,
                                     server_hostname):
        protocol = protocol_factory()
        waiter = self.create_future()
        if ssl:
            sslcontext = None if isinstance(ssl, bool) else ssl
            transport = self._make_ssl_transport(
                sock, protocol, sslcontext, waiter,
                server_side=False, server_hostname=server_hostname)
        else:
            transport = self._make_socket_transport(sock, protocol, waiter)

        try:
            yield from waiter
        except:
            transport.close()
            raise

        return transport, protocol

    @coroutine
    def create_datagram_endpoint(self, protocol_factory,
                                 local_addr=None, remote_addr=None, *,
                                 family=0, proto=0, flags=0,
                                 reuse_address=None, reuse_port=None,
                                 allow_broadcast=None, sock=None):
        """Create datagram connection."""
        if sock is not None:
            if (local_addr or remote_addr or
                    family or proto or flags or
                    reuse_address or reuse_port or allow_broadcast):
                # show the problematic kwargs in exception msg
                opts = dict(local_addr=local_addr, remote_addr=remote_addr,
                            family=family, proto=proto, flags=flags,
                            reuse_address=reuse_address, reuse_port=reuse_port,
                            allow_broadcast=allow_broadcast)
                problems = ', '.join(
                    '{}={}'.format(k, v) for k, v in opts.items() if v)
                raise ValueError(
                    'socket modifier keyword arguments can not be used '
                    'when sock is specified. ({})'.format(problems))
            sock.setblocking(False)
            r_addr = None
        else:
            if not (local_addr or remote_addr):
                if family == 0:
                    raise ValueError('unexpected address family')
                addr_pairs_info = (((family, proto), (None, None)),)
            else:
                # join address by (family, protocol)
                addr_infos = collections.OrderedDict()
                for idx, addr in ((0, local_addr), (1, remote_addr)):
                    if addr is not None:
                        assert isinstance(addr, tuple) and len(addr) == 2, (
                            '2-tuple is expected')

                        infos = yield from _ensure_resolved(
                            addr, family=family, type=socket.SOCK_DGRAM,
                            proto=proto, flags=flags, loop=self)
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

            if reuse_address is None:
                reuse_address = os.name == 'posix' and sys.platform != 'cygwin'

            for ((family, proto),
                 (local_address, remote_address)) in addr_pairs_info:
                sock = None
                r_addr = None
                try:
                    sock = socket.socket(
                        family=family, type=socket.SOCK_DGRAM, proto=proto)
                    if reuse_address:
                        sock.setsockopt(
                            socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                    if reuse_port:
                        if not hasattr(socket, 'SO_REUSEPORT'):
                            raise ValueError(
                                'reuse_port not supported by socket module')
                        else:
                            sock.setsockopt(
                                socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
                    if allow_broadcast:
                        sock.setsockopt(
                            socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
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
                except:
                    if sock is not None:
                        sock.close()
                    raise
                else:
                    break
            else:
                raise exceptions[0]

        protocol = protocol_factory()
        waiter = self.create_future()
        transport = self._make_datagram_transport(
            sock, protocol, r_addr, waiter)
        if self._debug:
            if local_addr:
                logger.info("Datagram endpoint local_addr=%r remote_addr=%r "
                            "created: (%r, %r)",
                            local_addr, remote_addr, transport, protocol)
            else:
                logger.debug("Datagram endpoint remote_addr=%r created: "
                             "(%r, %r)",
                             remote_addr, transport, protocol)

        try:
            yield from waiter
        except:
            transport.close()
            raise

        return transport, protocol

    @coroutine
    def _create_server_getaddrinfo(self, host, port, family, flags):
        infos = yield from _ensure_resolved((host, port), family=family,
                                            type=socket.SOCK_STREAM,
                                            flags=flags, loop=self)
        if not infos:
            raise OSError('getaddrinfo({!r}) returned empty list'.format(host))
        return infos

    @coroutine
    def create_server(self, protocol_factory, host=None, port=None,
                      *,
                      family=socket.AF_UNSPEC,
                      flags=socket.AI_PASSIVE,
                      sock=None,
                      backlog=100,
                      ssl=None,
                      reuse_address=None,
                      reuse_port=None):
        """Create a TCP server.

        The host parameter can be a string, in that case the TCP server is bound
        to host and port.

        The host parameter can also be a sequence of strings and in that case
        the TCP server is bound to all hosts of the sequence. If a host
        appears multiple times (possibly indirectly e.g. when hostnames
        resolve to the same IP address), the server is only bound once to that
        host.

        Return a Server object which can be used to stop the service.

        This method is a coroutine.
        """
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
                hosts = [None]
            elif (isinstance(host, str) or
                  not isinstance(host, collections.Iterable)):
                hosts = [host]
            else:
                hosts = host

            fs = [self._create_server_getaddrinfo(host, port, family=family,
                                                  flags=flags)
                  for host in hosts]
            infos = yield from tasks.gather(*fs, loop=self)
            infos = set(itertools.chain.from_iterable(infos))

            completed = False
            try:
                for res in infos:
                    af, socktype, proto, canonname, sa = res
                    try:
                        sock = socket.socket(af, socktype, proto)
                    except socket.error:
                        # Assume it's a bad family/type/protocol combination.
                        if self._debug:
                            logger.warning('create_server() failed to create '
                                           'socket.socket(%r, %r, %r)',
                                           af, socktype, proto, exc_info=True)
                        continue
                    sockets.append(sock)
                    if reuse_address:
                        sock.setsockopt(
                            socket.SOL_SOCKET, socket.SO_REUSEADDR, True)
                    if reuse_port:
                        if not hasattr(socket, 'SO_REUSEPORT'):
                            raise ValueError(
                                'reuse_port not supported by socket module')
                        else:
                            sock.setsockopt(
                                socket.SOL_SOCKET, socket.SO_REUSEPORT, True)
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
                raise ValueError('Neither host/port nor sock were specified')
            sockets = [sock]

        server = Server(self, sockets)
        for sock in sockets:
            sock.listen(backlog)
            sock.setblocking(False)
            self._start_serving(protocol_factory, sock, ssl, server)
        if self._debug:
            logger.info("%r is serving", server)
        return server

    @coroutine
    def connect_read_pipe(self, protocol_factory, pipe):
        protocol = protocol_factory()
        waiter = self.create_future()
        transport = self._make_read_pipe_transport(pipe, protocol, waiter)

        try:
            yield from waiter
        except:
            transport.close()
            raise

        if self._debug:
            logger.debug('Read pipe %r connected: (%r, %r)',
                         pipe.fileno(), transport, protocol)
        return transport, protocol

    @coroutine
    def connect_write_pipe(self, protocol_factory, pipe):
        protocol = protocol_factory()
        waiter = self.create_future()
        transport = self._make_write_pipe_transport(pipe, protocol, waiter)

        try:
            yield from waiter
        except:
            transport.close()
            raise

        if self._debug:
            logger.debug('Write pipe %r connected: (%r, %r)',
                         pipe.fileno(), transport, protocol)
        return transport, protocol

    def _log_subprocess(self, msg, stdin, stdout, stderr):
        info = [msg]
        if stdin is not None:
            info.append('stdin=%s' % _format_pipe(stdin))
        if stdout is not None and stderr == subprocess.STDOUT:
            info.append('stdout=stderr=%s' % _format_pipe(stdout))
        else:
            if stdout is not None:
                info.append('stdout=%s' % _format_pipe(stdout))
            if stderr is not None:
                info.append('stderr=%s' % _format_pipe(stderr))
        logger.debug(' '.join(info))

    @coroutine
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
        if self._debug:
            # don't log parameters: they may contain sensitive information
            # (password) and may be too long
            debug_log = 'run shell command %r' % cmd
            self._log_subprocess(debug_log, stdin, stdout, stderr)
        transport = yield from self._make_subprocess_transport(
            protocol, cmd, True, stdin, stdout, stderr, bufsize, **kwargs)
        if self._debug:
            logger.info('%s: %r' % (debug_log, transport))
        return transport, protocol

    @coroutine
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
        if self._debug:
            # don't log parameters: they may contain sensitive information
            # (password) and may be too long
            debug_log = 'execute program %r' % program
            self._log_subprocess(debug_log, stdin, stdout, stderr)
        transport = yield from self._make_subprocess_transport(
            protocol, popen_args, False, stdin, stdout, stderr,
            bufsize, **kwargs)
        if self._debug:
            logger.info('%s: %r' % (debug_log, transport))
        return transport, protocol

    def get_exception_handler(self):
        """Return an exception handler, or None if the default one is in use.
        """
        return self._exception_handler

    def set_exception_handler(self, handler):
        """Set handler as the new event loop exception handler.

        If handler is None, the default exception handler will
        be set.

        If handler is a callable object, it should have a
        signature matching '(loop, context)', where 'loop'
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

        The context parameter has the same meaning as in
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

        if ('source_traceback' not in context
        and self._current_handle is not None
        and self._current_handle._source_traceback):
            context['handle_traceback'] = self._current_handle._source_traceback

        log_lines = [message]
        for key in sorted(context):
            if key in {'message', 'exception'}:
                continue
            value = context[key]
            if key == 'source_traceback':
                tb = ''.join(traceback.format_list(value))
                value = 'Object created at (most recent call last):\n'
                value += tb.rstrip()
            elif key == 'handle_traceback':
                tb = ''.join(traceback.format_list(value))
                value = 'Handle created at (most recent call last):\n'
                value += tb.rstrip()
            else:
                value = repr(value)
            log_lines.append('{}: {}'.format(key, value))

        logger.error('\n'.join(log_lines), exc_info=exc_info)

    def call_exception_handler(self, context):
        """Call the current event loop's exception handler.

        The context argument is a dict containing the following keys:

        - 'message': Error message;
        - 'exception' (optional): Exception object;
        - 'future' (optional): Future instance;
        - 'handle' (optional): Handle instance;
        - 'protocol' (optional): Protocol instance;
        - 'transport' (optional): Transport instance;
        - 'socket' (optional): Socket instance.

        New keys maybe introduced in the future.

        Note: do not overload this method in an event loop subclass.
        For custom exception handling, use the
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
                    # Guard 'default_exception_handler' in case it is
                    # overloaded.
                    logger.error('Exception in default exception handler '
                                 'while handling an unexpected error '
                                 'in custom exception handler',
                                 exc_info=True)

    def _add_callback(self, handle):
        """Add a Handle to _scheduled (TimerHandle) or _ready."""
        assert isinstance(handle, events.Handle), 'A Handle is required here'
        if handle._cancelled:
            return
        assert not isinstance(handle, events.TimerHandle)
        self._ready.append(handle)

    def _add_callback_signalsafe(self, handle):
        """Like _add_callback() but called from a signal handler."""
        self._add_callback(handle)
        self._write_to_self()

    def _timer_handle_cancelled(self, handle):
        """Notification that a TimerHandle has been cancelled."""
        if handle._scheduled:
            self._timer_cancelled_count += 1

    def _run_once(self):
        """Run one full iteration of the event loop.

        This calls all currently ready callbacks, polls for I/O,
        schedules the resulting callbacks, and finally schedules
        'call_later' callbacks.
        """

        sched_count = len(self._scheduled)
        if (sched_count > _MIN_SCHEDULED_TIMER_HANDLES and
            self._timer_cancelled_count / sched_count >
                _MIN_CANCELLED_TIMER_HANDLES_FRACTION):
            # Remove delayed calls that were cancelled if their number
            # is too high
            new_scheduled = []
            for handle in self._scheduled:
                if handle._cancelled:
                    handle._scheduled = False
                else:
                    new_scheduled.append(handle)

            heapq.heapify(new_scheduled)
            self._scheduled = new_scheduled
            self._timer_cancelled_count = 0
        else:
            # Remove delayed calls that were cancelled from head of queue.
            while self._scheduled and self._scheduled[0]._cancelled:
                self._timer_cancelled_count -= 1
                handle = heapq.heappop(self._scheduled)
                handle._scheduled = False

        timeout = None
        if self._ready or self._stopping:
            timeout = 0
        elif self._scheduled:
            # Compute the desired timeout.
            when = self._scheduled[0]._when
            timeout = max(0, when - self.time())

        if self._debug and timeout != 0:
            t0 = self.time()
            event_list = self._selector.select(timeout)
            dt = self.time() - t0
            if dt >= 1.0:
                level = logging.INFO
            else:
                level = logging.DEBUG
            nevent = len(event_list)
            if timeout is None:
                logger.log(level, 'poll took %.3f ms: %s events',
                           dt * 1e3, nevent)
            elif nevent:
                logger.log(level,
                           'poll %.3f ms took %.3f ms: %s events',
                           timeout * 1e3, dt * 1e3, nevent)
            elif dt >= 1.0:
                logger.log(level,
                           'poll %.3f ms took %.3f ms: timeout',
                           timeout * 1e3, dt * 1e3)
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
            handle._scheduled = False
            self._ready.append(handle)

        # This is the only place where callbacks are actually *called*.
        # All other places just add them to ready.
        # Note: We run all currently scheduled callbacks, but not any
        # callbacks scheduled by callbacks run this time around --
        # they will be run the next time (after another I/O poll).
        # Use an idiom that is thread-safe without using locks.
        ntodo = len(self._ready)
        for i in range(ntodo):
            handle = self._ready.popleft()
            if handle._cancelled:
                continue
            if self._debug:
                try:
                    self._current_handle = handle
                    t0 = self.time()
                    handle._run()
                    dt = self.time() - t0
                    if dt >= self.slow_callback_duration:
                        logger.warning('Executing %s took %.3f seconds',
                                       _format_handle(handle), dt)
                finally:
                    self._current_handle = None
            else:
                handle._run()
        handle = None  # Needed to break cycles when an exception occurs.

    def _set_coroutine_wrapper(self, enabled):
        try:
            set_wrapper = sys.set_coroutine_wrapper
            get_wrapper = sys.get_coroutine_wrapper
        except AttributeError:
            return

        enabled = bool(enabled)
        if self._coroutine_wrapper_set == enabled:
            return

        wrapper = coroutines.debug_wrapper
        current_wrapper = get_wrapper()

        if enabled:
            if current_wrapper not in (None, wrapper):
                warnings.warn(
                    "loop.set_debug(True): cannot set debug coroutine "
                    "wrapper; another wrapper is already set %r" %
                    current_wrapper, RuntimeWarning)
            else:
                set_wrapper(wrapper)
                self._coroutine_wrapper_set = True
        else:
            if current_wrapper not in (None, wrapper):
                warnings.warn(
                    "loop.set_debug(False): cannot unset debug coroutine "
                    "wrapper; another wrapper was set %r" %
                    current_wrapper, RuntimeWarning)
            else:
                set_wrapper(None)
                self._coroutine_wrapper_set = False

    def get_debug(self):
        return self._debug

    def set_debug(self, enabled):
        self._debug = enabled

        if self.is_running():
            self._set_coroutine_wrapper(enabled)
