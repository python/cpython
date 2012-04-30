#
# Module to allow connection and socket objects to be transferred
# between processes
#
# multiprocessing/reduction.py
#
# Copyright (c) 2006-2008, R Oudkerk
# Licensed to PSF under a Contributor Agreement.
#

__all__ = ['reduce_socket', 'reduce_connection', 'send_handle', 'recv_handle']

import os
import sys
import socket
import threading
import struct
import signal

from multiprocessing import current_process
from multiprocessing.util import register_after_fork, debug, sub_debug
from multiprocessing.util import is_exiting, sub_warning


#
#
#

if not(sys.platform == 'win32' or (hasattr(socket, 'CMSG_LEN') and
                                   hasattr(socket, 'SCM_RIGHTS'))):
    raise ImportError('pickling of connections not supported')

#
# Platform specific definitions
#

if sys.platform == 'win32':
    # Windows
    __all__ += ['reduce_pipe_connection']
    import _winapi

    def send_handle(conn, handle, destination_pid):
        dh = DupHandle(handle, _winapi.DUPLICATE_SAME_ACCESS, destination_pid)
        conn.send(dh)

    def recv_handle(conn):
        return conn.recv().detach()

    class DupHandle(object):
        def __init__(self, handle, access, pid=None):
            # duplicate handle for process with given pid
            if pid is None:
                pid = os.getpid()
            proc = _winapi.OpenProcess(_winapi.PROCESS_DUP_HANDLE, False, pid)
            try:
                self._handle = _winapi.DuplicateHandle(
                    _winapi.GetCurrentProcess(),
                    handle, proc, access, False, 0)
            finally:
                _winapi.CloseHandle(proc)
            self._access = access
            self._pid = pid

        def detach(self):
            # retrieve handle from process which currently owns it
            if self._pid == os.getpid():
                return self._handle
            proc = _winapi.OpenProcess(_winapi.PROCESS_DUP_HANDLE, False,
                                       self._pid)
            try:
                return _winapi.DuplicateHandle(
                    proc, self._handle, _winapi.GetCurrentProcess(),
                    self._access, False, _winapi.DUPLICATE_CLOSE_SOURCE)
            finally:
                _winapi.CloseHandle(proc)

    class DupSocket(object):
        def __init__(self, sock):
            new_sock = sock.dup()
            def send(conn, pid):
                share = new_sock.share(pid)
                conn.send_bytes(share)
            self._id = resource_sharer.register(send, new_sock.close)

        def detach(self):
            conn = resource_sharer.get_connection(self._id)
            try:
                share = conn.recv_bytes()
                return socket.fromshare(share)
            finally:
                conn.close()

    def reduce_socket(s):
        return rebuild_socket, (DupSocket(s),)

    def rebuild_socket(ds):
        return ds.detach()

    def reduce_connection(conn):
        handle = conn.fileno()
        with socket.fromfd(handle, socket.AF_INET, socket.SOCK_STREAM) as s:
            ds = DupSocket(s)
            return rebuild_connection, (ds, conn.readable, conn.writable)

    def rebuild_connection(ds, readable, writable):
        from .connection import Connection
        sock = ds.detach()
        return Connection(sock.detach(), readable, writable)

    def reduce_pipe_connection(conn):
        access = ((_winapi.FILE_GENERIC_READ if conn.readable else 0) |
                  (_winapi.FILE_GENERIC_WRITE if conn.writable else 0))
        dh = DupHandle(conn.fileno(), access)
        return rebuild_pipe_connection, (dh, conn.readable, conn.writable)

    def rebuild_pipe_connection(dh, readable, writable):
        from .connection import PipeConnection
        handle = dh.detach()
        return PipeConnection(handle, readable, writable)

else:
    # Unix
    def send_handle(conn, handle, destination_pid):
        with socket.fromfd(conn.fileno(), socket.AF_UNIX, socket.SOCK_STREAM) as s:
            s.sendmsg([b'x'], [(socket.SOL_SOCKET, socket.SCM_RIGHTS,
                                struct.pack("@i", handle))])

    def recv_handle(conn):
        size = struct.calcsize("@i")
        with socket.fromfd(conn.fileno(), socket.AF_UNIX, socket.SOCK_STREAM) as s:
            msg, ancdata, flags, addr = s.recvmsg(1, socket.CMSG_LEN(size))
            try:
                cmsg_level, cmsg_type, cmsg_data = ancdata[0]
                if (cmsg_level == socket.SOL_SOCKET and
                    cmsg_type == socket.SCM_RIGHTS):
                    return struct.unpack("@i", cmsg_data[:size])[0]
            except (ValueError, IndexError, struct.error):
                pass
            raise RuntimeError('Invalid data received')

    class DupFd(object):
        def __init__(self, fd):
            new_fd = os.dup(fd)
            def send(conn, pid):
                send_handle(conn, new_fd, pid)
            def close():
                os.close(new_fd)
            self._id = resource_sharer.register(send, close)

        def detach(self):
            conn = resource_sharer.get_connection(self._id)
            try:
                return recv_handle(conn)
            finally:
                conn.close()

    def reduce_socket(s):
        df = DupFd(s.fileno())
        return rebuild_socket, (df, s.family, s.type, s.proto)

    def rebuild_socket(df, family, type, proto):
        fd = df.detach()
        s = socket.fromfd(fd, family, type, proto)
        os.close(fd)
        return s

    def reduce_connection(conn):
        df = DupFd(conn.fileno())
        return rebuild_connection, (df, conn.readable, conn.writable)

    def rebuild_connection(df, readable, writable):
        from .connection import Connection
        fd = df.detach()
        return Connection(fd, readable, writable)

#
# Server which shares registered resources with clients
#

class ResourceSharer(object):
    def __init__(self):
        self._key = 0
        self._cache = {}
        self._old_locks = []
        self._lock = threading.Lock()
        self._listener = None
        self._address = None
        self._thread = None
        register_after_fork(self, ResourceSharer._afterfork)

    def register(self, send, close):
        with self._lock:
            if self._address is None:
                self._start()
            self._key += 1
            self._cache[self._key] = (send, close)
            return (self._address, self._key)

    @staticmethod
    def get_connection(ident):
        from .connection import Client
        address, key = ident
        c = Client(address, authkey=current_process().authkey)
        c.send((key, os.getpid()))
        return c

    def stop(self, timeout=None):
        from .connection import Client
        with self._lock:
            if self._address is not None:
                c = Client(self._address, authkey=current_process().authkey)
                c.send(None)
                c.close()
                self._thread.join(timeout)
                if self._thread.is_alive():
                    sub_warn('ResourceSharer thread did not stop when asked')
                self._listener.close()
                self._thread = None
                self._address = None
                self._listener = None
                for key, (send, close) in self._cache.items():
                    close()
                self._cache.clear()

    def _afterfork(self):
        for key, (send, close) in self._cache.items():
            close()
        self._cache.clear()
        # If self._lock was locked at the time of the fork, it may be broken
        # -- see issue 6721.  Replace it without letting it be gc'ed.
        self._old_locks.append(self._lock)
        self._lock = threading.Lock()
        if self._listener is not None:
            self._listener.close()
        self._listener = None
        self._address = None
        self._thread = None

    def _start(self):
        from .connection import Listener
        assert self._listener is None
        debug('starting listener and thread for sending handles')
        self._listener = Listener(authkey=current_process().authkey)
        self._address = self._listener.address
        t = threading.Thread(target=self._serve)
        t.daemon = True
        t.start()
        self._thread = t

    def _serve(self):
        if hasattr(signal, 'pthread_sigmask'):
            signal.pthread_sigmask(signal.SIG_BLOCK, range(1, signal.NSIG))
        while 1:
            try:
                conn = self._listener.accept()
                msg = conn.recv()
                if msg is None:
                    break
                key, destination_pid = msg
                send, close = self._cache.pop(key)
                send(conn, destination_pid)
                close()
                conn.close()
            except:
                if not is_exiting():
                    import traceback
                    sub_warning(
                        'thread for sharing handles raised exception :\n' +
                        '-'*79 + '\n' + traceback.format_exc() + '-'*79
                        )

resource_sharer = ResourceSharer()
