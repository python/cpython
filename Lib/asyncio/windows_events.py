"""Selector and proactor event loops for Windows."""

import _winapi
import errno
import math
import socket
import struct
import weakref

from . import events
from . import base_subprocess
from . import futures
from . import proactor_events
from . import selector_events
from . import tasks
from . import windows_utils
from .log import logger
from . import _overlapped


__all__ = ['SelectorEventLoop', 'ProactorEventLoop', 'IocpProactor',
           'DefaultEventLoopPolicy',
           ]


NULL = 0
INFINITE = 0xffffffff
ERROR_CONNECTION_REFUSED = 1225
ERROR_CONNECTION_ABORTED = 1236


class _OverlappedFuture(futures.Future):
    """Subclass of Future which represents an overlapped operation.

    Cancelling it will immediately cancel the overlapped operation.
    """

    def __init__(self, ov, *, loop=None):
        super().__init__(loop=loop)
        self.ov = ov

    def cancel(self):
        try:
            self.ov.cancel()
        except OSError:
            pass
        return super().cancel()


class _WaitHandleFuture(futures.Future):
    """Subclass of Future which represents a wait handle."""

    def __init__(self, wait_handle, *, loop=None):
        super().__init__(loop=loop)
        self._wait_handle = wait_handle

    def cancel(self):
        super().cancel()
        try:
            _overlapped.UnregisterWait(self._wait_handle)
        except OSError as e:
            if e.winerror != _overlapped.ERROR_IO_PENDING:
                raise


class PipeServer(object):
    """Class representing a pipe server.

    This is much like a bound, listening socket.
    """
    def __init__(self, address):
        self._address = address
        self._free_instances = weakref.WeakSet()
        self._pipe = self._server_pipe_handle(True)

    def _get_unconnected_pipe(self):
        # Create new instance and return previous one.  This ensures
        # that (until the server is closed) there is always at least
        # one pipe handle for address.  Therefore if a client attempt
        # to connect it will not fail with FileNotFoundError.
        tmp, self._pipe = self._pipe, self._server_pipe_handle(False)
        return tmp

    def _server_pipe_handle(self, first):
        # Return a wrapper for a new pipe handle.
        if self._address is None:
            return None
        flags = _winapi.PIPE_ACCESS_DUPLEX | _winapi.FILE_FLAG_OVERLAPPED
        if first:
            flags |= _winapi.FILE_FLAG_FIRST_PIPE_INSTANCE
        h = _winapi.CreateNamedPipe(
            self._address, flags,
            _winapi.PIPE_TYPE_MESSAGE | _winapi.PIPE_READMODE_MESSAGE |
            _winapi.PIPE_WAIT,
            _winapi.PIPE_UNLIMITED_INSTANCES,
            windows_utils.BUFSIZE, windows_utils.BUFSIZE,
            _winapi.NMPWAIT_WAIT_FOREVER, _winapi.NULL)
        pipe = windows_utils.PipeHandle(h)
        self._free_instances.add(pipe)
        return pipe

    def close(self):
        # Close all instances which have not been connected to by a client.
        if self._address is not None:
            for pipe in self._free_instances:
                pipe.close()
            self._pipe = None
            self._address = None
            self._free_instances.clear()

    __del__ = close


class _WindowsSelectorEventLoop(selector_events.BaseSelectorEventLoop):
    """Windows version of selector event loop."""

    def _socketpair(self):
        return windows_utils.socketpair()


class ProactorEventLoop(proactor_events.BaseProactorEventLoop):
    """Windows version of proactor event loop using IOCP."""

    def __init__(self, proactor=None):
        if proactor is None:
            proactor = IocpProactor()
        super().__init__(proactor)

    def _socketpair(self):
        return windows_utils.socketpair()

    @tasks.coroutine
    def create_pipe_connection(self, protocol_factory, address):
        f = self._proactor.connect_pipe(address)
        pipe = yield from f
        protocol = protocol_factory()
        trans = self._make_duplex_pipe_transport(pipe, protocol,
                                                 extra={'addr': address})
        return trans, protocol

    @tasks.coroutine
    def start_serving_pipe(self, protocol_factory, address):
        server = PipeServer(address)

        def loop(f=None):
            pipe = None
            try:
                if f:
                    pipe = f.result()
                    server._free_instances.discard(pipe)
                    protocol = protocol_factory()
                    self._make_duplex_pipe_transport(
                        pipe, protocol, extra={'addr': address})
                pipe = server._get_unconnected_pipe()
                if pipe is None:
                    return
                f = self._proactor.accept_pipe(pipe)
            except OSError as exc:
                if pipe and pipe.fileno() != -1:
                    self.call_exception_handler({
                        'message': 'Pipe accept failed',
                        'exception': exc,
                        'pipe': pipe,
                    })
                    pipe.close()
            except futures.CancelledError:
                if pipe:
                    pipe.close()
            else:
                f.add_done_callback(loop)

        self.call_soon(loop)
        return [server]

    @tasks.coroutine
    def _make_subprocess_transport(self, protocol, args, shell,
                                   stdin, stdout, stderr, bufsize,
                                   extra=None, **kwargs):
        transp = _WindowsSubprocessTransport(self, protocol, args, shell,
                                             stdin, stdout, stderr, bufsize,
                                             extra=extra, **kwargs)
        yield from transp._post_init()
        return transp


class IocpProactor:
    """Proactor implementation using IOCP."""

    def __init__(self, concurrency=0xffffffff):
        self._loop = None
        self._results = []
        self._iocp = _overlapped.CreateIoCompletionPort(
            _overlapped.INVALID_HANDLE_VALUE, NULL, 0, concurrency)
        self._cache = {}
        self._registered = weakref.WeakSet()
        self._stopped_serving = weakref.WeakSet()

    def set_loop(self, loop):
        self._loop = loop

    def select(self, timeout=None):
        if not self._results:
            self._poll(timeout)
        tmp = self._results
        self._results = []
        return tmp

    def recv(self, conn, nbytes, flags=0):
        self._register_with_iocp(conn)
        ov = _overlapped.Overlapped(NULL)
        if isinstance(conn, socket.socket):
            ov.WSARecv(conn.fileno(), nbytes, flags)
        else:
            ov.ReadFile(conn.fileno(), nbytes)

        def finish(trans, key, ov):
            try:
                return ov.getresult()
            except OSError as exc:
                if exc.winerror == _overlapped.ERROR_NETNAME_DELETED:
                    raise ConnectionResetError(*exc.args)
                else:
                    raise

        return self._register(ov, conn, finish)

    def send(self, conn, buf, flags=0):
        self._register_with_iocp(conn)
        ov = _overlapped.Overlapped(NULL)
        if isinstance(conn, socket.socket):
            ov.WSASend(conn.fileno(), buf, flags)
        else:
            ov.WriteFile(conn.fileno(), buf)

        def finish(trans, key, ov):
            try:
                return ov.getresult()
            except OSError as exc:
                if exc.winerror == _overlapped.ERROR_NETNAME_DELETED:
                    raise ConnectionResetError(*exc.args)
                else:
                    raise

        return self._register(ov, conn, finish)

    def accept(self, listener):
        self._register_with_iocp(listener)
        conn = self._get_accept_socket(listener.family)
        ov = _overlapped.Overlapped(NULL)
        ov.AcceptEx(listener.fileno(), conn.fileno())

        def finish_accept(trans, key, ov):
            ov.getresult()
            # Use SO_UPDATE_ACCEPT_CONTEXT so getsockname() etc work.
            buf = struct.pack('@P', listener.fileno())
            conn.setsockopt(socket.SOL_SOCKET,
                            _overlapped.SO_UPDATE_ACCEPT_CONTEXT, buf)
            conn.settimeout(listener.gettimeout())
            return conn, conn.getpeername()

        @tasks.coroutine
        def accept_coro(future, conn):
            # Coroutine closing the accept socket if the future is cancelled
            try:
                yield from future
            except futures.CancelledError:
                conn.close()
                raise

        future = self._register(ov, listener, finish_accept)
        coro = accept_coro(future, conn)
        tasks.async(coro, loop=self._loop)
        return future

    def connect(self, conn, address):
        self._register_with_iocp(conn)
        # The socket needs to be locally bound before we call ConnectEx().
        try:
            _overlapped.BindLocal(conn.fileno(), conn.family)
        except OSError as e:
            if e.winerror != errno.WSAEINVAL:
                raise
            # Probably already locally bound; check using getsockname().
            if conn.getsockname()[1] == 0:
                raise
        ov = _overlapped.Overlapped(NULL)
        ov.ConnectEx(conn.fileno(), address)

        def finish_connect(trans, key, ov):
            ov.getresult()
            # Use SO_UPDATE_CONNECT_CONTEXT so getsockname() etc work.
            conn.setsockopt(socket.SOL_SOCKET,
                            _overlapped.SO_UPDATE_CONNECT_CONTEXT, 0)
            return conn

        return self._register(ov, conn, finish_connect)

    def accept_pipe(self, pipe):
        self._register_with_iocp(pipe)
        ov = _overlapped.Overlapped(NULL)
        ov.ConnectNamedPipe(pipe.fileno())

        def finish(trans, key, ov):
            ov.getresult()
            return pipe

        return self._register(ov, pipe, finish)

    def connect_pipe(self, address):
        ov = _overlapped.Overlapped(NULL)
        ov.WaitNamedPipeAndConnect(address, self._iocp, ov.address)

        def finish(err, handle, ov):
            # err, handle were arguments passed to PostQueuedCompletionStatus()
            # in a function run in a thread pool.
            if err == _overlapped.ERROR_SEM_TIMEOUT:
                # Connection did not succeed within time limit.
                msg = _overlapped.FormatMessage(err)
                raise ConnectionRefusedError(0, msg, None, err)
            elif err != 0:
                msg = _overlapped.FormatMessage(err)
                raise OSError(0, msg, None, err)
            else:
                return windows_utils.PipeHandle(handle)

        return self._register(ov, None, finish, wait_for_post=True)

    def wait_for_handle(self, handle, timeout=None):
        if timeout is None:
            ms = _winapi.INFINITE
        else:
            # RegisterWaitForSingleObject() has a resolution of 1 millisecond,
            # round away from zero to wait *at least* timeout seconds.
            ms = math.ceil(timeout * 1e3)

        # We only create ov so we can use ov.address as a key for the cache.
        ov = _overlapped.Overlapped(NULL)
        wh = _overlapped.RegisterWaitWithQueue(
            handle, self._iocp, ov.address, ms)
        f = _WaitHandleFuture(wh, loop=self._loop)

        def finish(trans, key, ov):
            if not f.cancelled():
                try:
                    _overlapped.UnregisterWait(wh)
                except OSError as e:
                    if e.winerror != _overlapped.ERROR_IO_PENDING:
                        raise
            # Note that this second wait means that we should only use
            # this with handles types where a successful wait has no
            # effect.  So events or processes are all right, but locks
            # or semaphores are not.  Also note if the handle is
            # signalled and then quickly reset, then we may return
            # False even though we have not timed out.
            return (_winapi.WaitForSingleObject(handle, 0) ==
                    _winapi.WAIT_OBJECT_0)

        self._cache[ov.address] = (f, ov, None, finish)
        return f

    def _register_with_iocp(self, obj):
        # To get notifications of finished ops on this objects sent to the
        # completion port, were must register the handle.
        if obj not in self._registered:
            self._registered.add(obj)
            _overlapped.CreateIoCompletionPort(obj.fileno(), self._iocp, 0, 0)
            # XXX We could also use SetFileCompletionNotificationModes()
            # to avoid sending notifications to completion port of ops
            # that succeed immediately.

    def _register(self, ov, obj, callback, wait_for_post=False):
        # Return a future which will be set with the result of the
        # operation when it completes.  The future's value is actually
        # the value returned by callback().
        f = _OverlappedFuture(ov, loop=self._loop)
        if ov.pending or wait_for_post:
            # Register the overlapped operation for later.  Note that
            # we only store obj to prevent it from being garbage
            # collected too early.
            self._cache[ov.address] = (f, ov, obj, callback)
        else:
            # The operation has completed, so no need to postpone the
            # work.  We cannot take this short cut if we need the
            # NumberOfBytes, CompletionKey values returned by
            # PostQueuedCompletionStatus().
            try:
                value = callback(None, None, ov)
            except OSError as e:
                f.set_exception(e)
            else:
                f.set_result(value)
        return f

    def _get_accept_socket(self, family):
        s = socket.socket(family)
        s.settimeout(0)
        return s

    def _poll(self, timeout=None):
        if timeout is None:
            ms = INFINITE
        elif timeout < 0:
            raise ValueError("negative timeout")
        else:
            # GetQueuedCompletionStatus() has a resolution of 1 millisecond,
            # round away from zero to wait *at least* timeout seconds.
            ms = math.ceil(timeout * 1e3)
            if ms >= INFINITE:
                raise ValueError("timeout too big")
        while True:
            status = _overlapped.GetQueuedCompletionStatus(self._iocp, ms)
            if status is None:
                return
            err, transferred, key, address = status
            try:
                f, ov, obj, callback = self._cache.pop(address)
            except KeyError:
                # key is either zero, or it is used to return a pipe
                # handle which should be closed to avoid a leak.
                if key not in (0, _overlapped.INVALID_HANDLE_VALUE):
                    _winapi.CloseHandle(key)
                ms = 0
                continue
            if obj in self._stopped_serving:
                f.cancel()
            elif not f.cancelled():
                try:
                    value = callback(transferred, key, ov)
                except OSError as e:
                    f.set_exception(e)
                    self._results.append(f)
                else:
                    f.set_result(value)
                    self._results.append(f)
            ms = 0

    def _stop_serving(self, obj):
        # obj is a socket or pipe handle.  It will be closed in
        # BaseProactorEventLoop._stop_serving() which will make any
        # pending operations fail quickly.
        self._stopped_serving.add(obj)

    def close(self):
        # Cancel remaining registered operations.
        for address, (f, ov, obj, callback) in list(self._cache.items()):
            if obj is None:
                # The operation was started with connect_pipe() which
                # queues a task to Windows' thread pool.  This cannot
                # be cancelled, so just forget it.
                del self._cache[address]
            else:
                try:
                    ov.cancel()
                except OSError:
                    pass

        while self._cache:
            if not self._poll(1):
                logger.debug('taking long time to close proactor')

        self._results = []
        if self._iocp is not None:
            _winapi.CloseHandle(self._iocp)
            self._iocp = None


class _WindowsSubprocessTransport(base_subprocess.BaseSubprocessTransport):

    def _start(self, args, shell, stdin, stdout, stderr, bufsize, **kwargs):
        self._proc = windows_utils.Popen(
            args, shell=shell, stdin=stdin, stdout=stdout, stderr=stderr,
            bufsize=bufsize, **kwargs)

        def callback(f):
            returncode = self._proc.poll()
            self._process_exited(returncode)

        f = self._loop._proactor.wait_for_handle(int(self._proc._handle))
        f.add_done_callback(callback)


SelectorEventLoop = _WindowsSelectorEventLoop


class _WindowsDefaultEventLoopPolicy(events.BaseDefaultEventLoopPolicy):
    _loop_factory = SelectorEventLoop


DefaultEventLoopPolicy = _WindowsDefaultEventLoopPolicy
