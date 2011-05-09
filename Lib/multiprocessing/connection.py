#
# A higher level module for using sockets (or Windows named pipes)
#
# multiprocessing/connection.py
#
# Copyright (c) 2006-2008, R Oudkerk
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of author nor the names of any contributors may be
#    used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#

__all__ = [ 'Client', 'Listener', 'Pipe' ]

import io
import os
import sys
import pickle
import select
import socket
import struct
import errno
import time
import tempfile
import itertools

import _multiprocessing
from multiprocessing import current_process, AuthenticationError, BufferTooShort
from multiprocessing.util import get_temp_dir, Finalize, sub_debug, debug
try:
    from _multiprocessing import win32
except ImportError:
    if sys.platform == 'win32':
        raise
    win32 = None

#
#
#

BUFSIZE = 8192
# A very generous timeout when it comes to local connections...
CONNECTION_TIMEOUT = 20.

_mmap_counter = itertools.count()

default_family = 'AF_INET'
families = ['AF_INET']

if hasattr(socket, 'AF_UNIX'):
    default_family = 'AF_UNIX'
    families += ['AF_UNIX']

if sys.platform == 'win32':
    default_family = 'AF_PIPE'
    families += ['AF_PIPE']


def _init_timeout(timeout=CONNECTION_TIMEOUT):
    return time.time() + timeout

def _check_timeout(t):
    return time.time() > t

#
#
#

def arbitrary_address(family):
    '''
    Return an arbitrary free address for the given family
    '''
    if family == 'AF_INET':
        return ('localhost', 0)
    elif family == 'AF_UNIX':
        return tempfile.mktemp(prefix='listener-', dir=get_temp_dir())
    elif family == 'AF_PIPE':
        return tempfile.mktemp(prefix=r'\\.\pipe\pyc-%d-%d-' %
                               (os.getpid(), next(_mmap_counter)))
    else:
        raise ValueError('unrecognized family')


def address_type(address):
    '''
    Return the types of the address

    This can be 'AF_INET', 'AF_UNIX', or 'AF_PIPE'
    '''
    if type(address) == tuple:
        return 'AF_INET'
    elif type(address) is str and address.startswith('\\\\'):
        return 'AF_PIPE'
    elif type(address) is str:
        return 'AF_UNIX'
    else:
        raise ValueError('address type of %r unrecognized' % address)

#
# Connection classes
#

class _ConnectionBase:
    _handle = None

    def __init__(self, handle, readable=True, writable=True):
        handle = handle.__index__()
        if handle < 0:
            raise ValueError("invalid handle")
        if not readable and not writable:
            raise ValueError(
                "at least one of `readable` and `writable` must be True")
        self._handle = handle
        self._readable = readable
        self._writable = writable

    def __del__(self):
        if self._handle is not None:
            self._close()

    def _check_closed(self):
        if self._handle is None:
            raise IOError("handle is closed")

    def _check_readable(self):
        if not self._readable:
            raise IOError("connection is write-only")

    def _check_writable(self):
        if not self._writable:
            raise IOError("connection is read-only")

    def _bad_message_length(self):
        if self._writable:
            self._readable = False
        else:
            self.close()
        raise IOError("bad message length")

    @property
    def closed(self):
        """True if the connection is closed"""
        return self._handle is None

    @property
    def readable(self):
        """True if the connection is readable"""
        return self._readable

    @property
    def writable(self):
        """True if the connection is writable"""
        return self._writable

    def fileno(self):
        """File descriptor or handle of the connection"""
        self._check_closed()
        return self._handle

    def close(self):
        """Close the connection"""
        if self._handle is not None:
            try:
                self._close()
            finally:
                self._handle = None

    def send_bytes(self, buf, offset=0, size=None):
        """Send the bytes data from a bytes-like object"""
        self._check_closed()
        self._check_writable()
        m = memoryview(buf)
        # HACK for byte-indexing of non-bytewise buffers (e.g. array.array)
        if m.itemsize > 1:
            m = memoryview(bytes(m))
        n = len(m)
        if offset < 0:
            raise ValueError("offset is negative")
        if n < offset:
            raise ValueError("buffer length < offset")
        if size is None:
            size = n - offset
        elif size < 0:
            raise ValueError("size is negative")
        elif offset + size > n:
            raise ValueError("buffer length < offset + size")
        self._send_bytes(m[offset:offset + size])

    def send(self, obj):
        """Send a (picklable) object"""
        self._check_closed()
        self._check_writable()
        buf = pickle.dumps(obj, protocol=pickle.HIGHEST_PROTOCOL)
        self._send_bytes(memoryview(buf))

    def recv_bytes(self, maxlength=None):
        """
        Receive bytes data as a bytes object.
        """
        self._check_closed()
        self._check_readable()
        if maxlength is not None and maxlength < 0:
            raise ValueError("negative maxlength")
        buf = self._recv_bytes(maxlength)
        if buf is None:
            self._bad_message_length()
        return buf.getvalue()

    def recv_bytes_into(self, buf, offset=0):
        """
        Receive bytes data into a writeable buffer-like object.
        Return the number of bytes read.
        """
        self._check_closed()
        self._check_readable()
        with memoryview(buf) as m:
            # Get bytesize of arbitrary buffer
            itemsize = m.itemsize
            bytesize = itemsize * len(m)
            if offset < 0:
                raise ValueError("negative offset")
            elif offset > bytesize:
                raise ValueError("offset too large")
            result = self._recv_bytes()
            size = result.tell()
            if bytesize < offset + size:
                raise BufferTooShort(result.getvalue())
            # Message can fit in dest
            result.seek(0)
            result.readinto(m[offset // itemsize :
                              (offset + size) // itemsize])
            return size

    def recv(self):
        """Receive a (picklable) object"""
        self._check_closed()
        self._check_readable()
        buf = self._recv_bytes()
        return pickle.loads(buf.getbuffer())

    def poll(self, timeout=0.0):
        """Whether there is any input available to be read"""
        self._check_closed()
        self._check_readable()
        if timeout < 0.0:
            timeout = None
        return self._poll(timeout)


if win32:

    class PipeConnection(_ConnectionBase):
        """
        Connection class based on a Windows named pipe.
        """

        def _close(self):
            win32.CloseHandle(self._handle)

        def _send_bytes(self, buf):
            nwritten = win32.WriteFile(self._handle, buf)
            assert nwritten == len(buf)

        def _recv_bytes(self, maxsize=None):
            buf = io.BytesIO()
            bufsize = 512
            if maxsize is not None:
                bufsize = min(bufsize, maxsize)
            try:
                firstchunk, complete = win32.ReadFile(self._handle, bufsize)
            except IOError as e:
                if e.errno == win32.ERROR_BROKEN_PIPE:
                    raise EOFError
                raise
            lenfirstchunk = len(firstchunk)
            buf.write(firstchunk)
            if complete:
                return buf
            navail, nleft = win32.PeekNamedPipe(self._handle)
            if maxsize is not None and lenfirstchunk + nleft > maxsize:
                return None
            lastchunk, complete = win32.ReadFile(self._handle, nleft)
            assert complete
            buf.write(lastchunk)
            return buf

        def _poll(self, timeout):
            navail, nleft = win32.PeekNamedPipe(self._handle)
            if navail > 0:
                return True
            elif timeout == 0.0:
                return False
            # Setup a polling loop (translated straight from old
            # pipe_connection.c)
            if timeout < 0.0:
                deadline = None
            else:
                deadline = time.time() + timeout
            delay = 0.001
            max_delay = 0.02
            while True:
                time.sleep(delay)
                navail, nleft = win32.PeekNamedPipe(self._handle)
                if navail > 0:
                    return True
                if deadline and time.time() > deadline:
                    return False
                if delay < max_delay:
                    delay += 0.001


class Connection(_ConnectionBase):
    """
    Connection class based on an arbitrary file descriptor (Unix only), or
    a socket handle (Windows).
    """

    if win32:
        def _close(self):
            win32.closesocket(self._handle)
        _write = win32.send
        _read = win32.recv
    else:
        def _close(self):
            os.close(self._handle)
        _write = os.write
        _read = os.read

    def _send(self, buf, write=_write):
        remaining = len(buf)
        while True:
            n = write(self._handle, buf)
            remaining -= n
            if remaining == 0:
                break
            buf = buf[n:]

    def _recv(self, size, read=_read):
        buf = io.BytesIO()
        remaining = size
        while remaining > 0:
            chunk = read(self._handle, remaining)
            n = len(chunk)
            if n == 0:
                if remaining == size:
                    raise EOFError
                else:
                    raise IOError("got end of file during message")
            buf.write(chunk)
            remaining -= n
        return buf

    def _send_bytes(self, buf):
        # For wire compatibility with 3.2 and lower
        n = len(buf)
        self._send(struct.pack("=i", len(buf)))
        # The condition is necessary to avoid "broken pipe" errors
        # when sending a 0-length buffer if the other end closed the pipe.
        if n > 0:
            self._send(buf)

    def _recv_bytes(self, maxsize=None):
        buf = self._recv(4)
        size, = struct.unpack("=i", buf.getvalue())
        if maxsize is not None and size > maxsize:
            return None
        return self._recv(size)

    def _poll(self, timeout):
        r = select.select([self._handle], [], [], timeout)[0]
        return bool(r)


#
# Public functions
#

class Listener(object):
    '''
    Returns a listener object.

    This is a wrapper for a bound socket which is 'listening' for
    connections, or for a Windows named pipe.
    '''
    def __init__(self, address=None, family=None, backlog=1, authkey=None):
        family = family or (address and address_type(address)) \
                 or default_family
        address = address or arbitrary_address(family)

        if family == 'AF_PIPE':
            self._listener = PipeListener(address, backlog)
        else:
            self._listener = SocketListener(address, family, backlog)

        if authkey is not None and not isinstance(authkey, bytes):
            raise TypeError('authkey should be a byte string')

        self._authkey = authkey

    def accept(self):
        '''
        Accept a connection on the bound socket or named pipe of `self`.

        Returns a `Connection` object.
        '''
        c = self._listener.accept()
        if self._authkey:
            deliver_challenge(c, self._authkey)
            answer_challenge(c, self._authkey)
        return c

    def close(self):
        '''
        Close the bound socket or named pipe of `self`.
        '''
        return self._listener.close()

    address = property(lambda self: self._listener._address)
    last_accepted = property(lambda self: self._listener._last_accepted)


def Client(address, family=None, authkey=None):
    '''
    Returns a connection to the address of a `Listener`
    '''
    family = family or address_type(address)
    if family == 'AF_PIPE':
        c = PipeClient(address)
    else:
        c = SocketClient(address)

    if authkey is not None and not isinstance(authkey, bytes):
        raise TypeError('authkey should be a byte string')

    if authkey is not None:
        answer_challenge(c, authkey)
        deliver_challenge(c, authkey)

    return c


if sys.platform != 'win32':

    def Pipe(duplex=True):
        '''
        Returns pair of connection objects at either end of a pipe
        '''
        if duplex:
            s1, s2 = socket.socketpair()
            c1 = Connection(os.dup(s1.fileno()))
            c2 = Connection(os.dup(s2.fileno()))
            s1.close()
            s2.close()
        else:
            fd1, fd2 = os.pipe()
            c1 = Connection(fd1, writable=False)
            c2 = Connection(fd2, readable=False)

        return c1, c2

else:

    def Pipe(duplex=True):
        '''
        Returns pair of connection objects at either end of a pipe
        '''
        address = arbitrary_address('AF_PIPE')
        if duplex:
            openmode = win32.PIPE_ACCESS_DUPLEX
            access = win32.GENERIC_READ | win32.GENERIC_WRITE
            obsize, ibsize = BUFSIZE, BUFSIZE
        else:
            openmode = win32.PIPE_ACCESS_INBOUND
            access = win32.GENERIC_WRITE
            obsize, ibsize = 0, BUFSIZE

        h1 = win32.CreateNamedPipe(
            address, openmode,
            win32.PIPE_TYPE_MESSAGE | win32.PIPE_READMODE_MESSAGE |
            win32.PIPE_WAIT,
            1, obsize, ibsize, win32.NMPWAIT_WAIT_FOREVER, win32.NULL
            )
        h2 = win32.CreateFile(
            address, access, 0, win32.NULL, win32.OPEN_EXISTING, 0, win32.NULL
            )
        win32.SetNamedPipeHandleState(
            h2, win32.PIPE_READMODE_MESSAGE, None, None
            )

        try:
            win32.ConnectNamedPipe(h1, win32.NULL)
        except WindowsError as e:
            if e.args[0] != win32.ERROR_PIPE_CONNECTED:
                raise

        c1 = PipeConnection(h1, writable=duplex)
        c2 = PipeConnection(h2, readable=duplex)

        return c1, c2

#
# Definitions for connections based on sockets
#

class SocketListener(object):
    '''
    Representation of a socket which is bound to an address and listening
    '''
    def __init__(self, address, family, backlog=1):
        self._socket = socket.socket(getattr(socket, family))
        self._socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._socket.bind(address)
        self._socket.listen(backlog)
        self._address = self._socket.getsockname()
        self._family = family
        self._last_accepted = None

        if family == 'AF_UNIX':
            self._unlink = Finalize(
                self, os.unlink, args=(address,), exitpriority=0
                )
        else:
            self._unlink = None

    def accept(self):
        s, self._last_accepted = self._socket.accept()
        fd = duplicate(s.fileno())
        conn = Connection(fd)
        s.close()
        return conn

    def close(self):
        self._socket.close()
        if self._unlink is not None:
            self._unlink()


def SocketClient(address):
    '''
    Return a connection object connected to the socket given by `address`
    '''
    family = address_type(address)
    with socket.socket( getattr(socket, family) ) as s:
        t = _init_timeout()

        while 1:
            try:
                s.connect(address)
            except socket.error as e:
                if e.args[0] != errno.ECONNREFUSED or _check_timeout(t):
                    debug('failed to connect to address %s', address)
                    raise
                time.sleep(0.01)
            else:
                break
        else:
            raise

        fd = duplicate(s.fileno())
    conn = Connection(fd)
    return conn

#
# Definitions for connections based on named pipes
#

if sys.platform == 'win32':

    class PipeListener(object):
        '''
        Representation of a named pipe
        '''
        def __init__(self, address, backlog=None):
            self._address = address
            handle = win32.CreateNamedPipe(
                address, win32.PIPE_ACCESS_DUPLEX,
                win32.PIPE_TYPE_MESSAGE | win32.PIPE_READMODE_MESSAGE |
                win32.PIPE_WAIT,
                win32.PIPE_UNLIMITED_INSTANCES, BUFSIZE, BUFSIZE,
                win32.NMPWAIT_WAIT_FOREVER, win32.NULL
                )
            self._handle_queue = [handle]
            self._last_accepted = None

            sub_debug('listener created with address=%r', self._address)

            self.close = Finalize(
                self, PipeListener._finalize_pipe_listener,
                args=(self._handle_queue, self._address), exitpriority=0
                )

        def accept(self):
            newhandle = win32.CreateNamedPipe(
                self._address, win32.PIPE_ACCESS_DUPLEX,
                win32.PIPE_TYPE_MESSAGE | win32.PIPE_READMODE_MESSAGE |
                win32.PIPE_WAIT,
                win32.PIPE_UNLIMITED_INSTANCES, BUFSIZE, BUFSIZE,
                win32.NMPWAIT_WAIT_FOREVER, win32.NULL
                )
            self._handle_queue.append(newhandle)
            handle = self._handle_queue.pop(0)
            try:
                win32.ConnectNamedPipe(handle, win32.NULL)
            except WindowsError as e:
                if e.args[0] != win32.ERROR_PIPE_CONNECTED:
                    raise
            return PipeConnection(handle)

        @staticmethod
        def _finalize_pipe_listener(queue, address):
            sub_debug('closing listener with address=%r', address)
            for handle in queue:
                close(handle)

    def PipeClient(address):
        '''
        Return a connection object connected to the pipe given by `address`
        '''
        t = _init_timeout()
        while 1:
            try:
                win32.WaitNamedPipe(address, 1000)
                h = win32.CreateFile(
                    address, win32.GENERIC_READ | win32.GENERIC_WRITE,
                    0, win32.NULL, win32.OPEN_EXISTING, 0, win32.NULL
                    )
            except WindowsError as e:
                if e.args[0] not in (win32.ERROR_SEM_TIMEOUT,
                                     win32.ERROR_PIPE_BUSY) or _check_timeout(t):
                    raise
            else:
                break
        else:
            raise

        win32.SetNamedPipeHandleState(
            h, win32.PIPE_READMODE_MESSAGE, None, None
            )
        return PipeConnection(h)

#
# Authentication stuff
#

MESSAGE_LENGTH = 20

CHALLENGE = b'#CHALLENGE#'
WELCOME = b'#WELCOME#'
FAILURE = b'#FAILURE#'

def deliver_challenge(connection, authkey):
    import hmac
    assert isinstance(authkey, bytes)
    message = os.urandom(MESSAGE_LENGTH)
    connection.send_bytes(CHALLENGE + message)
    digest = hmac.new(authkey, message).digest()
    response = connection.recv_bytes(256)        # reject large message
    if response == digest:
        connection.send_bytes(WELCOME)
    else:
        connection.send_bytes(FAILURE)
        raise AuthenticationError('digest received was wrong')

def answer_challenge(connection, authkey):
    import hmac
    assert isinstance(authkey, bytes)
    message = connection.recv_bytes(256)         # reject large message
    assert message[:len(CHALLENGE)] == CHALLENGE, 'message = %r' % message
    message = message[len(CHALLENGE):]
    digest = hmac.new(authkey, message).digest()
    connection.send_bytes(digest)
    response = connection.recv_bytes(256)        # reject large message
    if response != WELCOME:
        raise AuthenticationError('digest sent was rejected')

#
# Support for using xmlrpclib for serialization
#

class ConnectionWrapper(object):
    def __init__(self, conn, dumps, loads):
        self._conn = conn
        self._dumps = dumps
        self._loads = loads
        for attr in ('fileno', 'close', 'poll', 'recv_bytes', 'send_bytes'):
            obj = getattr(conn, attr)
            setattr(self, attr, obj)
    def send(self, obj):
        s = self._dumps(obj)
        self._conn.send_bytes(s)
    def recv(self):
        s = self._conn.recv_bytes()
        return self._loads(s)

def _xml_dumps(obj):
    return xmlrpclib.dumps((obj,), None, None, None, 1).encode('utf-8')

def _xml_loads(s):
    (obj,), method = xmlrpclib.loads(s.decode('utf-8'))
    return obj

class XmlListener(Listener):
    def accept(self):
        global xmlrpclib
        import xmlrpc.client as xmlrpclib
        obj = Listener.accept(self)
        return ConnectionWrapper(obj, _xml_dumps, _xml_loads)

def XmlClient(*args, **kwds):
    global xmlrpclib
    import xmlrpc.client as xmlrpclib
    return ConnectionWrapper(Client(*args, **kwds), _xml_dumps, _xml_loads)


# Late import because of circular import
from multiprocessing.forking import duplicate, close
