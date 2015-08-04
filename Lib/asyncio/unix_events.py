"""Selector event loop for Unix with signal handling."""

import errno
import os
import signal
import socket
import stat
import subprocess
import sys
import threading
import warnings


from . import base_events
from . import base_subprocess
from . import compat
from . import constants
from . import coroutines
from . import events
from . import futures
from . import selector_events
from . import selectors
from . import transports
from .coroutines import coroutine
from .log import logger


__all__ = ['SelectorEventLoop',
           'AbstractChildWatcher', 'SafeChildWatcher',
           'FastChildWatcher', 'DefaultEventLoopPolicy',
           ]

if sys.platform == 'win32':  # pragma: no cover
    raise ImportError('Signals are not really supported on Windows')


def _sighandler_noop(signum, frame):
    """Dummy signal handler."""
    pass


class _UnixSelectorEventLoop(selector_events.BaseSelectorEventLoop):
    """Unix event loop.

    Adds signal handling and UNIX Domain Socket support to SelectorEventLoop.
    """

    def __init__(self, selector=None):
        super().__init__(selector)
        self._signal_handlers = {}

    def _socketpair(self):
        return socket.socketpair()

    def close(self):
        super().close()
        for sig in list(self._signal_handlers):
            self.remove_signal_handler(sig)

    def _process_self_data(self, data):
        for signum in data:
            if not signum:
                # ignore null bytes written by _write_to_self()
                continue
            self._handle_signal(signum)

    def add_signal_handler(self, sig, callback, *args):
        """Add a handler for a signal.  UNIX only.

        Raise ValueError if the signal number is invalid or uncatchable.
        Raise RuntimeError if there is a problem setting up the handler.
        """
        if (coroutines.iscoroutine(callback)
        or coroutines.iscoroutinefunction(callback)):
            raise TypeError("coroutines cannot be used "
                            "with add_signal_handler()")
        self._check_signal(sig)
        self._check_closed()
        try:
            # set_wakeup_fd() raises ValueError if this is not the
            # main thread.  By calling it early we ensure that an
            # event loop running in another thread cannot add a signal
            # handler.
            signal.set_wakeup_fd(self._csock.fileno())
        except (ValueError, OSError) as exc:
            raise RuntimeError(str(exc))

        handle = events.Handle(callback, args, self)
        self._signal_handlers[sig] = handle

        try:
            # Register a dummy signal handler to ask Python to write the signal
            # number in the wakup file descriptor. _process_self_data() will
            # read signal numbers from this file descriptor to handle signals.
            signal.signal(sig, _sighandler_noop)

            # Set SA_RESTART to limit EINTR occurrences.
            signal.siginterrupt(sig, False)
        except OSError as exc:
            del self._signal_handlers[sig]
            if not self._signal_handlers:
                try:
                    signal.set_wakeup_fd(-1)
                except (ValueError, OSError) as nexc:
                    logger.info('set_wakeup_fd(-1) failed: %s', nexc)

            if exc.errno == errno.EINVAL:
                raise RuntimeError('sig {} cannot be caught'.format(sig))
            else:
                raise

    def _handle_signal(self, sig):
        """Internal helper that is the actual signal handler."""
        handle = self._signal_handlers.get(sig)
        if handle is None:
            return  # Assume it's some race condition.
        if handle._cancelled:
            self.remove_signal_handler(sig)  # Remove it properly.
        else:
            self._add_callback_signalsafe(handle)

    def remove_signal_handler(self, sig):
        """Remove a handler for a signal.  UNIX only.

        Return True if a signal handler was removed, False if not.
        """
        self._check_signal(sig)
        try:
            del self._signal_handlers[sig]
        except KeyError:
            return False

        if sig == signal.SIGINT:
            handler = signal.default_int_handler
        else:
            handler = signal.SIG_DFL

        try:
            signal.signal(sig, handler)
        except OSError as exc:
            if exc.errno == errno.EINVAL:
                raise RuntimeError('sig {} cannot be caught'.format(sig))
            else:
                raise

        if not self._signal_handlers:
            try:
                signal.set_wakeup_fd(-1)
            except (ValueError, OSError) as exc:
                logger.info('set_wakeup_fd(-1) failed: %s', exc)

        return True

    def _check_signal(self, sig):
        """Internal helper to validate a signal.

        Raise ValueError if the signal number is invalid or uncatchable.
        Raise RuntimeError if there is a problem setting up the handler.
        """
        if not isinstance(sig, int):
            raise TypeError('sig must be an int, not {!r}'.format(sig))

        if not (1 <= sig < signal.NSIG):
            raise ValueError(
                'sig {} out of range(1, {})'.format(sig, signal.NSIG))

    def _make_read_pipe_transport(self, pipe, protocol, waiter=None,
                                  extra=None):
        return _UnixReadPipeTransport(self, pipe, protocol, waiter, extra)

    def _make_write_pipe_transport(self, pipe, protocol, waiter=None,
                                   extra=None):
        return _UnixWritePipeTransport(self, pipe, protocol, waiter, extra)

    @coroutine
    def _make_subprocess_transport(self, protocol, args, shell,
                                   stdin, stdout, stderr, bufsize,
                                   extra=None, **kwargs):
        with events.get_child_watcher() as watcher:
            waiter = futures.Future(loop=self)
            transp = _UnixSubprocessTransport(self, protocol, args, shell,
                                              stdin, stdout, stderr, bufsize,
                                              waiter=waiter, extra=extra,
                                              **kwargs)

            watcher.add_child_handler(transp.get_pid(),
                                      self._child_watcher_callback, transp)
            try:
                yield from waiter
            except Exception as exc:
                # Workaround CPython bug #23353: using yield/yield-from in an
                # except block of a generator doesn't clear properly
                # sys.exc_info()
                err = exc
            else:
                err = None

            if err is not None:
                transp.close()
                yield from transp._wait()
                raise err

        return transp

    def _child_watcher_callback(self, pid, returncode, transp):
        self.call_soon_threadsafe(transp._process_exited, returncode)

    @coroutine
    def create_unix_connection(self, protocol_factory, path, *,
                               ssl=None, sock=None,
                               server_hostname=None):
        assert server_hostname is None or isinstance(server_hostname, str)
        if ssl:
            if server_hostname is None:
                raise ValueError(
                    'you have to pass server_hostname when using ssl')
        else:
            if server_hostname is not None:
                raise ValueError('server_hostname is only meaningful with ssl')

        if path is not None:
            if sock is not None:
                raise ValueError(
                    'path and sock can not be specified at the same time')

            sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM, 0)
            try:
                sock.setblocking(False)
                yield from self.sock_connect(sock, path)
            except:
                sock.close()
                raise

        else:
            if sock is None:
                raise ValueError('no path and sock were specified')
            sock.setblocking(False)

        transport, protocol = yield from self._create_connection_transport(
            sock, protocol_factory, ssl, server_hostname)
        return transport, protocol

    @coroutine
    def create_unix_server(self, protocol_factory, path=None, *,
                           sock=None, backlog=100, ssl=None):
        if isinstance(ssl, bool):
            raise TypeError('ssl argument must be an SSLContext or None')

        if path is not None:
            if sock is not None:
                raise ValueError(
                    'path and sock can not be specified at the same time')

            sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)

            try:
                sock.bind(path)
            except OSError as exc:
                sock.close()
                if exc.errno == errno.EADDRINUSE:
                    # Let's improve the error message by adding
                    # with what exact address it occurs.
                    msg = 'Address {!r} is already in use'.format(path)
                    raise OSError(errno.EADDRINUSE, msg) from None
                else:
                    raise
            except:
                sock.close()
                raise
        else:
            if sock is None:
                raise ValueError(
                    'path was not specified, and no sock specified')

            if sock.family != socket.AF_UNIX:
                raise ValueError(
                    'A UNIX Domain Socket was expected, got {!r}'.format(sock))

        server = base_events.Server(self, [sock])
        sock.listen(backlog)
        sock.setblocking(False)
        self._start_serving(protocol_factory, sock, ssl, server)
        return server


if hasattr(os, 'set_blocking'):
    def _set_nonblocking(fd):
        os.set_blocking(fd, False)
else:
    import fcntl

    def _set_nonblocking(fd):
        flags = fcntl.fcntl(fd, fcntl.F_GETFL)
        flags = flags | os.O_NONBLOCK
        fcntl.fcntl(fd, fcntl.F_SETFL, flags)


class _UnixReadPipeTransport(transports.ReadTransport):

    max_size = 256 * 1024  # max bytes we read in one event loop iteration

    def __init__(self, loop, pipe, protocol, waiter=None, extra=None):
        super().__init__(extra)
        self._extra['pipe'] = pipe
        self._loop = loop
        self._pipe = pipe
        self._fileno = pipe.fileno()
        mode = os.fstat(self._fileno).st_mode
        if not (stat.S_ISFIFO(mode) or
                stat.S_ISSOCK(mode) or
                stat.S_ISCHR(mode)):
            raise ValueError("Pipe transport is for pipes/sockets only.")
        _set_nonblocking(self._fileno)
        self._protocol = protocol
        self._closing = False
        self._loop.call_soon(self._protocol.connection_made, self)
        # only start reading when connection_made() has been called
        self._loop.call_soon(self._loop.add_reader,
                             self._fileno, self._read_ready)
        if waiter is not None:
            # only wake up the waiter when connection_made() has been called
            self._loop.call_soon(waiter._set_result_unless_cancelled, None)

    def __repr__(self):
        info = [self.__class__.__name__]
        if self._pipe is None:
            info.append('closed')
        elif self._closing:
            info.append('closing')
        info.append('fd=%s' % self._fileno)
        if self._pipe is not None:
            polling = selector_events._test_selector_event(
                          self._loop._selector,
                          self._fileno, selectors.EVENT_READ)
            if polling:
                info.append('polling')
            else:
                info.append('idle')
        else:
            info.append('closed')
        return '<%s>' % ' '.join(info)

    def _read_ready(self):
        try:
            data = os.read(self._fileno, self.max_size)
        except (BlockingIOError, InterruptedError):
            pass
        except OSError as exc:
            self._fatal_error(exc, 'Fatal read error on pipe transport')
        else:
            if data:
                self._protocol.data_received(data)
            else:
                if self._loop.get_debug():
                    logger.info("%r was closed by peer", self)
                self._closing = True
                self._loop.remove_reader(self._fileno)
                self._loop.call_soon(self._protocol.eof_received)
                self._loop.call_soon(self._call_connection_lost, None)

    def pause_reading(self):
        self._loop.remove_reader(self._fileno)

    def resume_reading(self):
        self._loop.add_reader(self._fileno, self._read_ready)

    def close(self):
        if not self._closing:
            self._close(None)

    # On Python 3.3 and older, objects with a destructor part of a reference
    # cycle are never destroyed. It's not more the case on Python 3.4 thanks
    # to the PEP 442.
    if compat.PY34:
        def __del__(self):
            if self._pipe is not None:
                warnings.warn("unclosed transport %r" % self, ResourceWarning)
                self._pipe.close()

    def _fatal_error(self, exc, message='Fatal error on pipe transport'):
        # should be called by exception handler only
        if (isinstance(exc, OSError) and exc.errno == errno.EIO):
            if self._loop.get_debug():
                logger.debug("%r: %s", self, message, exc_info=True)
        else:
            self._loop.call_exception_handler({
                'message': message,
                'exception': exc,
                'transport': self,
                'protocol': self._protocol,
            })
        self._close(exc)

    def _close(self, exc):
        self._closing = True
        self._loop.remove_reader(self._fileno)
        self._loop.call_soon(self._call_connection_lost, exc)

    def _call_connection_lost(self, exc):
        try:
            self._protocol.connection_lost(exc)
        finally:
            self._pipe.close()
            self._pipe = None
            self._protocol = None
            self._loop = None


class _UnixWritePipeTransport(transports._FlowControlMixin,
                              transports.WriteTransport):

    def __init__(self, loop, pipe, protocol, waiter=None, extra=None):
        super().__init__(extra, loop)
        self._extra['pipe'] = pipe
        self._pipe = pipe
        self._fileno = pipe.fileno()
        mode = os.fstat(self._fileno).st_mode
        is_socket = stat.S_ISSOCK(mode)
        if not (is_socket or
                stat.S_ISFIFO(mode) or
                stat.S_ISCHR(mode)):
            raise ValueError("Pipe transport is only for "
                             "pipes, sockets and character devices")
        _set_nonblocking(self._fileno)
        self._protocol = protocol
        self._buffer = []
        self._conn_lost = 0
        self._closing = False  # Set when close() or write_eof() called.

        self._loop.call_soon(self._protocol.connection_made, self)

        # On AIX, the reader trick (to be notified when the read end of the
        # socket is closed) only works for sockets. On other platforms it
        # works for pipes and sockets. (Exception: OS X 10.4?  Issue #19294.)
        if is_socket or not sys.platform.startswith("aix"):
            # only start reading when connection_made() has been called
            self._loop.call_soon(self._loop.add_reader,
                                 self._fileno, self._read_ready)

        if waiter is not None:
            # only wake up the waiter when connection_made() has been called
            self._loop.call_soon(waiter._set_result_unless_cancelled, None)

    def __repr__(self):
        info = [self.__class__.__name__]
        if self._pipe is None:
            info.append('closed')
        elif self._closing:
            info.append('closing')
        info.append('fd=%s' % self._fileno)
        if self._pipe is not None:
            polling = selector_events._test_selector_event(
                          self._loop._selector,
                          self._fileno, selectors.EVENT_WRITE)
            if polling:
                info.append('polling')
            else:
                info.append('idle')

            bufsize = self.get_write_buffer_size()
            info.append('bufsize=%s' % bufsize)
        else:
            info.append('closed')
        return '<%s>' % ' '.join(info)

    def get_write_buffer_size(self):
        return sum(len(data) for data in self._buffer)

    def _read_ready(self):
        # Pipe was closed by peer.
        if self._loop.get_debug():
            logger.info("%r was closed by peer", self)
        if self._buffer:
            self._close(BrokenPipeError())
        else:
            self._close()

    def write(self, data):
        assert isinstance(data, (bytes, bytearray, memoryview)), repr(data)
        if isinstance(data, bytearray):
            data = memoryview(data)
        if not data:
            return

        if self._conn_lost or self._closing:
            if self._conn_lost >= constants.LOG_THRESHOLD_FOR_CONNLOST_WRITES:
                logger.warning('pipe closed by peer or '
                               'os.write(pipe, data) raised exception.')
            self._conn_lost += 1
            return

        if not self._buffer:
            # Attempt to send it right away first.
            try:
                n = os.write(self._fileno, data)
            except (BlockingIOError, InterruptedError):
                n = 0
            except Exception as exc:
                self._conn_lost += 1
                self._fatal_error(exc, 'Fatal write error on pipe transport')
                return
            if n == len(data):
                return
            elif n > 0:
                data = data[n:]
            self._loop.add_writer(self._fileno, self._write_ready)

        self._buffer.append(data)
        self._maybe_pause_protocol()

    def _write_ready(self):
        data = b''.join(self._buffer)
        assert data, 'Data should not be empty'

        self._buffer.clear()
        try:
            n = os.write(self._fileno, data)
        except (BlockingIOError, InterruptedError):
            self._buffer.append(data)
        except Exception as exc:
            self._conn_lost += 1
            # Remove writer here, _fatal_error() doesn't it
            # because _buffer is empty.
            self._loop.remove_writer(self._fileno)
            self._fatal_error(exc, 'Fatal write error on pipe transport')
        else:
            if n == len(data):
                self._loop.remove_writer(self._fileno)
                self._maybe_resume_protocol()  # May append to buffer.
                if not self._buffer and self._closing:
                    self._loop.remove_reader(self._fileno)
                    self._call_connection_lost(None)
                return
            elif n > 0:
                data = data[n:]

            self._buffer.append(data)  # Try again later.

    def can_write_eof(self):
        return True

    def write_eof(self):
        if self._closing:
            return
        assert self._pipe
        self._closing = True
        if not self._buffer:
            self._loop.remove_reader(self._fileno)
            self._loop.call_soon(self._call_connection_lost, None)

    def close(self):
        if self._pipe is not None and not self._closing:
            # write_eof is all what we needed to close the write pipe
            self.write_eof()

    # On Python 3.3 and older, objects with a destructor part of a reference
    # cycle are never destroyed. It's not more the case on Python 3.4 thanks
    # to the PEP 442.
    if compat.PY34:
        def __del__(self):
            if self._pipe is not None:
                warnings.warn("unclosed transport %r" % self, ResourceWarning)
                self._pipe.close()

    def abort(self):
        self._close(None)

    def _fatal_error(self, exc, message='Fatal error on pipe transport'):
        # should be called by exception handler only
        if isinstance(exc, (BrokenPipeError, ConnectionResetError)):
            if self._loop.get_debug():
                logger.debug("%r: %s", self, message, exc_info=True)
        else:
            self._loop.call_exception_handler({
                'message': message,
                'exception': exc,
                'transport': self,
                'protocol': self._protocol,
            })
        self._close(exc)

    def _close(self, exc=None):
        self._closing = True
        if self._buffer:
            self._loop.remove_writer(self._fileno)
        self._buffer.clear()
        self._loop.remove_reader(self._fileno)
        self._loop.call_soon(self._call_connection_lost, exc)

    def _call_connection_lost(self, exc):
        try:
            self._protocol.connection_lost(exc)
        finally:
            self._pipe.close()
            self._pipe = None
            self._protocol = None
            self._loop = None


if hasattr(os, 'set_inheritable'):
    # Python 3.4 and newer
    _set_inheritable = os.set_inheritable
else:
    import fcntl

    def _set_inheritable(fd, inheritable):
        cloexec_flag = getattr(fcntl, 'FD_CLOEXEC', 1)

        old = fcntl.fcntl(fd, fcntl.F_GETFD)
        if not inheritable:
            fcntl.fcntl(fd, fcntl.F_SETFD, old | cloexec_flag)
        else:
            fcntl.fcntl(fd, fcntl.F_SETFD, old & ~cloexec_flag)


class _UnixSubprocessTransport(base_subprocess.BaseSubprocessTransport):

    def _start(self, args, shell, stdin, stdout, stderr, bufsize, **kwargs):
        stdin_w = None
        if stdin == subprocess.PIPE:
            # Use a socket pair for stdin, since not all platforms
            # support selecting read events on the write end of a
            # socket (which we use in order to detect closing of the
            # other end).  Notably this is needed on AIX, and works
            # just fine on other platforms.
            stdin, stdin_w = self._loop._socketpair()

            # Mark the write end of the stdin pipe as non-inheritable,
            # needed by close_fds=False on Python 3.3 and older
            # (Python 3.4 implements the PEP 446, socketpair returns
            # non-inheritable sockets)
            _set_inheritable(stdin_w.fileno(), False)
        self._proc = subprocess.Popen(
            args, shell=shell, stdin=stdin, stdout=stdout, stderr=stderr,
            universal_newlines=False, bufsize=bufsize, **kwargs)
        if stdin_w is not None:
            stdin.close()
            self._proc.stdin = open(stdin_w.detach(), 'wb', buffering=bufsize)


class AbstractChildWatcher:
    """Abstract base class for monitoring child processes.

    Objects derived from this class monitor a collection of subprocesses and
    report their termination or interruption by a signal.

    New callbacks are registered with .add_child_handler(). Starting a new
    process must be done within a 'with' block to allow the watcher to suspend
    its activity until the new process if fully registered (this is needed to
    prevent a race condition in some implementations).

    Example:
        with watcher:
            proc = subprocess.Popen("sleep 1")
            watcher.add_child_handler(proc.pid, callback)

    Notes:
        Implementations of this class must be thread-safe.

        Since child watcher objects may catch the SIGCHLD signal and call
        waitpid(-1), there should be only one active object per process.
    """

    def add_child_handler(self, pid, callback, *args):
        """Register a new child handler.

        Arrange for callback(pid, returncode, *args) to be called when
        process 'pid' terminates. Specifying another callback for the same
        process replaces the previous handler.

        Note: callback() must be thread-safe.
        """
        raise NotImplementedError()

    def remove_child_handler(self, pid):
        """Removes the handler for process 'pid'.

        The function returns True if the handler was successfully removed,
        False if there was nothing to remove."""

        raise NotImplementedError()

    def attach_loop(self, loop):
        """Attach the watcher to an event loop.

        If the watcher was previously attached to an event loop, then it is
        first detached before attaching to the new loop.

        Note: loop may be None.
        """
        raise NotImplementedError()

    def close(self):
        """Close the watcher.

        This must be called to make sure that any underlying resource is freed.
        """
        raise NotImplementedError()

    def __enter__(self):
        """Enter the watcher's context and allow starting new processes

        This function must return self"""
        raise NotImplementedError()

    def __exit__(self, a, b, c):
        """Exit the watcher's context"""
        raise NotImplementedError()


class BaseChildWatcher(AbstractChildWatcher):

    def __init__(self):
        self._loop = None

    def close(self):
        self.attach_loop(None)

    def _do_waitpid(self, expected_pid):
        raise NotImplementedError()

    def _do_waitpid_all(self):
        raise NotImplementedError()

    def attach_loop(self, loop):
        assert loop is None or isinstance(loop, events.AbstractEventLoop)

        if self._loop is not None:
            self._loop.remove_signal_handler(signal.SIGCHLD)

        self._loop = loop
        if loop is not None:
            loop.add_signal_handler(signal.SIGCHLD, self._sig_chld)

            # Prevent a race condition in case a child terminated
            # during the switch.
            self._do_waitpid_all()

    def _sig_chld(self):
        try:
            self._do_waitpid_all()
        except Exception as exc:
            # self._loop should always be available here
            # as '_sig_chld' is added as a signal handler
            # in 'attach_loop'
            self._loop.call_exception_handler({
                'message': 'Unknown exception in SIGCHLD handler',
                'exception': exc,
            })

    def _compute_returncode(self, status):
        if os.WIFSIGNALED(status):
            # The child process died because of a signal.
            return -os.WTERMSIG(status)
        elif os.WIFEXITED(status):
            # The child process exited (e.g sys.exit()).
            return os.WEXITSTATUS(status)
        else:
            # The child exited, but we don't understand its status.
            # This shouldn't happen, but if it does, let's just
            # return that status; perhaps that helps debug it.
            return status


class SafeChildWatcher(BaseChildWatcher):
    """'Safe' child watcher implementation.

    This implementation avoids disrupting other code spawning processes by
    polling explicitly each process in the SIGCHLD handler instead of calling
    os.waitpid(-1).

    This is a safe solution but it has a significant overhead when handling a
    big number of children (O(n) each time SIGCHLD is raised)
    """

    def __init__(self):
        super().__init__()
        self._callbacks = {}

    def close(self):
        self._callbacks.clear()
        super().close()

    def __enter__(self):
        return self

    def __exit__(self, a, b, c):
        pass

    def add_child_handler(self, pid, callback, *args):
        self._callbacks[pid] = (callback, args)

        # Prevent a race condition in case the child is already terminated.
        self._do_waitpid(pid)

    def remove_child_handler(self, pid):
        try:
            del self._callbacks[pid]
            return True
        except KeyError:
            return False

    def _do_waitpid_all(self):

        for pid in list(self._callbacks):
            self._do_waitpid(pid)

    def _do_waitpid(self, expected_pid):
        assert expected_pid > 0

        try:
            pid, status = os.waitpid(expected_pid, os.WNOHANG)
        except ChildProcessError:
            # The child process is already reaped
            # (may happen if waitpid() is called elsewhere).
            pid = expected_pid
            returncode = 255
            logger.warning(
                "Unknown child process pid %d, will report returncode 255",
                pid)
        else:
            if pid == 0:
                # The child process is still alive.
                return

            returncode = self._compute_returncode(status)
            if self._loop.get_debug():
                logger.debug('process %s exited with returncode %s',
                             expected_pid, returncode)

        try:
            callback, args = self._callbacks.pop(pid)
        except KeyError:  # pragma: no cover
            # May happen if .remove_child_handler() is called
            # after os.waitpid() returns.
            if self._loop.get_debug():
                logger.warning("Child watcher got an unexpected pid: %r",
                               pid, exc_info=True)
        else:
            callback(pid, returncode, *args)


class FastChildWatcher(BaseChildWatcher):
    """'Fast' child watcher implementation.

    This implementation reaps every terminated processes by calling
    os.waitpid(-1) directly, possibly breaking other code spawning processes
    and waiting for their termination.

    There is no noticeable overhead when handling a big number of children
    (O(1) each time a child terminates).
    """
    def __init__(self):
        super().__init__()
        self._callbacks = {}
        self._lock = threading.Lock()
        self._zombies = {}
        self._forks = 0

    def close(self):
        self._callbacks.clear()
        self._zombies.clear()
        super().close()

    def __enter__(self):
        with self._lock:
            self._forks += 1

            return self

    def __exit__(self, a, b, c):
        with self._lock:
            self._forks -= 1

            if self._forks or not self._zombies:
                return

            collateral_victims = str(self._zombies)
            self._zombies.clear()

        logger.warning(
            "Caught subprocesses termination from unknown pids: %s",
            collateral_victims)

    def add_child_handler(self, pid, callback, *args):
        assert self._forks, "Must use the context manager"
        with self._lock:
            try:
                returncode = self._zombies.pop(pid)
            except KeyError:
                # The child is running.
                self._callbacks[pid] = callback, args
                return

        # The child is dead already. We can fire the callback.
        callback(pid, returncode, *args)

    def remove_child_handler(self, pid):
        try:
            del self._callbacks[pid]
            return True
        except KeyError:
            return False

    def _do_waitpid_all(self):
        # Because of signal coalescing, we must keep calling waitpid() as
        # long as we're able to reap a child.
        while True:
            try:
                pid, status = os.waitpid(-1, os.WNOHANG)
            except ChildProcessError:
                # No more child processes exist.
                return
            else:
                if pid == 0:
                    # A child process is still alive.
                    return

                returncode = self._compute_returncode(status)

            with self._lock:
                try:
                    callback, args = self._callbacks.pop(pid)
                except KeyError:
                    # unknown child
                    if self._forks:
                        # It may not be registered yet.
                        self._zombies[pid] = returncode
                        if self._loop.get_debug():
                            logger.debug('unknown process %s exited '
                                         'with returncode %s',
                                         pid, returncode)
                        continue
                    callback = None
                else:
                    if self._loop.get_debug():
                        logger.debug('process %s exited with returncode %s',
                                     pid, returncode)

            if callback is None:
                logger.warning(
                    "Caught subprocess termination from unknown pid: "
                    "%d -> %d", pid, returncode)
            else:
                callback(pid, returncode, *args)


class _UnixDefaultEventLoopPolicy(events.BaseDefaultEventLoopPolicy):
    """UNIX event loop policy with a watcher for child processes."""
    _loop_factory = _UnixSelectorEventLoop

    def __init__(self):
        super().__init__()
        self._watcher = None

    def _init_watcher(self):
        with events._lock:
            if self._watcher is None:  # pragma: no branch
                self._watcher = SafeChildWatcher()
                if isinstance(threading.current_thread(),
                              threading._MainThread):
                    self._watcher.attach_loop(self._local._loop)

    def set_event_loop(self, loop):
        """Set the event loop.

        As a side effect, if a child watcher was set before, then calling
        .set_event_loop() from the main thread will call .attach_loop(loop) on
        the child watcher.
        """

        super().set_event_loop(loop)

        if self._watcher is not None and \
            isinstance(threading.current_thread(), threading._MainThread):
            self._watcher.attach_loop(loop)

    def get_child_watcher(self):
        """Get the watcher for child processes.

        If not yet set, a SafeChildWatcher object is automatically created.
        """
        if self._watcher is None:
            self._init_watcher()

        return self._watcher

    def set_child_watcher(self, watcher):
        """Set the watcher for child processes."""

        assert watcher is None or isinstance(watcher, AbstractChildWatcher)

        if self._watcher is not None:
            self._watcher.close()

        self._watcher = watcher

SelectorEventLoop = _UnixSelectorEventLoop
DefaultEventLoopPolicy = _UnixDefaultEventLoopPolicy
