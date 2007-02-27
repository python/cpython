"""New I/O library.

This is an early prototype; eventually some of this will be
reimplemented in C and the rest may be turned into a package.

See PEP XXX; for now: http://docs.google.com/Doc?id=dfksfvqd_1cn5g5m
"""

__author__ = ("Guido van Rossum <guido@python.org>, "
              "Mike Verdone <mike.verdone@gmail.com>")

__all__ = ["open", "RawIOBase", "FileIO", "SocketIO", "BytesIO",
           "BufferedReader", "BufferedWriter", "BufferedRWPair", "EOF"]

import os

DEFAULT_BUFFER_SIZE = 8 * 1024 # bytes
EOF = b""

def open(filename, mode="r", buffering=None, *, encoding=None):
    """Replacement for the built-in open function.

    Args:
      filename: string giving the name of the file to be opened
      mode: optional mode string; see below
      buffering: optional int >= 0 giving the buffer size; values
                 can be: 0 = unbuffered, 1 = line buffered,
                 larger = fully buffered
      encoding: optional string giving the text encoding (*must* be given
                as a keyword argument)

    Mode strings characters:
      'r': open for reading (default)
      'w': open for writing, truncating the file first
      'a': open for writing, appending to the end if the file exists
      'b': binary mode
      't': text mode (default)
      '+': open a disk file for updating (implies reading and writing)

    Constraints:
      - encoding must not be given when a binary mode is given
      - buffering must not be zero when a text mode is given

    Returns:
      Depending on the mode and buffering arguments, either a raw
      binary stream, a buffered binary stream, or a buffered text
      stream, open for reading and/or writing.
    """
    assert isinstance(filename, str)
    assert isinstance(mode, str)
    assert buffering is None or isinstance(buffering, int)
    assert encoding is None or isinstance(encoding, str)
    modes = set(mode)
    if modes - set("arwb+t") or len(mode) > len(modes):
        raise ValueError("invalid mode: %r" % mode)
    reading = "r" in modes
    writing = "w" in modes
    appending = "a" in modes
    updating = "+" in modes
    text = "t" in modes
    binary = "b" in modes
    if text and binary:
        raise ValueError("can't have text and binary mode at once")
    if reading + writing + appending > 1:
        raise ValueError("can't have read/write/append mode at once")
    if not (reading or writing or appending):
        raise ValueError("must have exactly one of read/write/append mode")
    if binary and encoding is not None:
        raise ValueError("binary mode doesn't take an encoding")
    raw = FileIO(filename,
                 (reading and "r" or "") +
                 (writing and "w" or "") +
                 (appending and "a" or "") +
                 (updating and "+" or ""))
    if buffering is None:
        buffering = DEFAULT_BUFFER_SIZE
        # XXX Should default to line buffering if os.isatty(raw.fileno())
        try:
            bs = os.fstat(raw.fileno()).st_blksize
        except (os.error, AttributeError):
            if bs > 1:
                buffering = bs
    if buffering < 0:
        raise ValueError("invalid buffering size")
    if buffering == 0:
        if binary:
            return raw
        raise ValueError("can't have unbuffered text I/O")
    if updating:
        buffer = BufferedRandom(raw, buffering)
    elif writing or appending:
        buffer = BufferedWriter(raw, buffering)
    else:
        assert reading
        buffer = BufferedReader(raw, buffering)
    if binary:
        return buffer
    # XXX What about newline conventions?
    textio = TextIOWrapper(buffer, encoding)
    return textio


class RawIOBase:

    """Base class for raw binary I/O.

    This class provides dummy implementations for all methods that
    derived classes can override selectively; the default
    implementations represent a file that cannot be read, written or
    seeked.

    The read() method is implemented by calling readinto(); derived
    classes that want to support readon only need to implement
    readinto() as a primitive operation.
    """

    # XXX Add individual method docstrings

    def read(self, n):
        b = bytes(n.__index__())
        self.readinto(b)
        return b

    def readinto(self, b):
        raise IOError(".readinto() not supported")

    def write(self, b):
        raise IOError(".write() not supported")

    def seek(self, pos, whence=0):
        raise IOError(".seek() not supported")

    def tell(self):
        raise IOError(".tell() not supported")

    def truncate(self, pos=None):
        raise IOError(".truncate() not supported")

    def close(self):
        pass

    def seekable(self):
        return False

    def readable(self):
        return False

    def writable(self):
        return False

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()

    def fileno(self):
        raise IOError(".fileno() not supported")


class FileIO(RawIOBase):

    """Raw I/O implementation for OS files."""

    # XXX More docs

    def __init__(self, filename, mode):
        self._seekable = None
        self._mode = mode
        if mode == "r":
            flags = os.O_RDONLY
        elif mode == "w":
            flags = os.O_WRONLY | os.O_CREAT | os.O_TRUNC
            self._writable = True
        elif mode == "r+":
            flags = os.O_RDWR
        else:
            assert 0, "unsupported mode %r (for now)" % mode
        if hasattr(os, "O_BINARY"):
            flags |= os.O_BINARY
        self._fd = os.open(filename, flags)

    def readinto(self, b):
        # XXX We really should have os.readinto()
        b[:] = os.read(self._fd, len(b))
        return len(b)

    def write(self, b):
        return os.write(self._fd, b)

    def seek(self, pos, whence=0):
        os.lseek(self._fd, pos, whence)

    def tell(self):
        return os.lseek(self._fd, 0, 1)

    def truncate(self, pos=None):
        if pos is None:
            pos = self.tell()
        os.ftruncate(self._fd, pos)

    def close(self):
        os.close(self._fd)

    def readable(self):
        return "r" in self._mode or "+" in self._mode

    def writable(self):
        return "w" in self._mode or "+" in self._mode or "a" in self._mode

    def seekable(self):
        if self._seekable is None:
            try:
                os.lseek(self._fd, 0, 1)
            except os.error:
                self._seekable = False
            else:
                self._seekable = True
        return self._seekable

    def fileno(self):
        return self._fd


class SocketIO(RawIOBase):

    """Raw I/O implementation for stream sockets."""

    # XXX More docs

    def __init__(self, sock, mode):
        assert mode in ("r", "w", "rw")
        self._sock = sock
        self._mode = mode
        self._readable = "r" in mode
        self._writable = "w" in mode
        self._seekable = False

    def readinto(self, b):
        return self._sock.recv_into(b)

    def write(self, b):
        return self._sock.send(b)

    def close(self):
        self._sock.close()

    def readable(self):
        return "r" in self._mode

    def writable(self):
        return "w" in self._mode

    def fileno(self):
        return self._sock.fileno()


class BufferedIOBase(RawIOBase):

    """XXX Docstring."""


class BytesIO(BufferedIOBase):

    """Buffered I/O implementation using a bytes buffer, like StringIO."""

    # XXX More docs

    def __init__(self, inital_bytes=None):
        self._buffer = b""
        self._pos = 0
        if inital_bytes is not None:
            self._buffer += inital_bytes

    def getvalue(self):
        return self._buffer

    def read(self, n=None):
        if n is None:
            n = len(self._buffer)
        assert n >= 0
        newpos = min(len(self._buffer), self._pos + n)
        b = self._buffer[self._pos : newpos]
        self._pos = newpos
        return b

    def readinto(self, b):
        b[:] = self.read(len(b))

    def write(self, b):
        n = len(b)
        newpos = self._pos + n
        self._buffer[self._pos:newpos] = b
        self._pos = newpos
        return n

    def seek(self, pos, whence=0):
        if whence == 0:
            self._pos = max(0, pos)
        elif whence == 1:
            self._pos = max(0, self._pos + pos)
        elif whence == 2:
            self._pos = max(0, len(self._buffer) + pos)
        else:
            raise IOError("invalid whence value")

    def tell(self):
        return self._pos

    def truncate(self, pos=None):
        if pos is None:
            pos = self._pos
        else:
            self._pos = max(0, pos)
        del self._buffer[pos:]

    def readable(self):
        return True

    def writable(self):
        return True

    def seekable(self):
        return True


class BufferedReader(BufferedIOBase):

    """Buffered reader.

    Buffer for a readable sequential RawIO object. Does not allow
    random access (seek, tell).
    """

    def __init__(self, raw):
        """
        Create a new buffered reader using the given readable raw IO object.
        """
        assert raw.readable()
        self.raw = raw
        self._read_buf = b''
        if hasattr(raw, 'fileno'):
            self.fileno = raw.fileno

    def read(self, n=None):
        """
        Read n bytes. Returns exactly n bytes of data unless the underlying
        raw IO stream reaches EOF of if the call would block in non-blocking
        mode. If n is None, read until EOF or until read() would block.
        """
        nodata_val = EOF
        while (len(self._read_buf) < n) if (n is not None) else True:
            current = self.raw.read(n)
            if current in (EOF, None):
                nodata_val = current
                break
            self._read_buf += current # XXX using += is bad
        read = self._read_buf[:n]
        if (not self._read_buf):
            return nodata_val
        self._read_buf = self._read_buf[n if n else 0:]
        return read

    def write(self, b):
        raise IOError(".write() unsupported")

    def readable(self):
        return True

    def flush(self):
        # Flush is a no-op
        pass


class BufferedWriter(BufferedIOBase):

    """Buffered writer.

    XXX More docs.
    """

    def __init__(self, raw, buffer_size=DEFAULT_BUFFER_SIZE):
        assert raw.writeable()
        self.raw = raw
        self.buffer_size = buffer_size
        self._write_buf_stack = []
        self._write_buf_size = 0
        if hasattr(raw, 'fileno'):
            self.fileno = raw.fileno

    def read(self, n=None):
        raise IOError(".read() not supported")

    def write(self, b):
        assert issubclass(type(b), bytes)
        self._write_buf_stack.append(b)
        self._write_buf_size += len(b)
        if (self._write_buf_size > self.buffer_size):
            self.flush()

    def writeable(self):
        return True

    def flush(self):
        buf = b''.join(self._write_buf_stack)
        while len(buf):
            buf = buf[self.raw.write(buf):]
        self._write_buf_stack = []
        self._write_buf_size = 0

    # XXX support flushing buffer on close, del


class BufferedRWPair(BufferedReader, BufferedWriter):

    """Buffered Read/Write Pair.

    A buffered reader object and buffered writer object put together to
    form a sequential IO object that can read and write.
    """

    def __init__(self, bufferedReader, bufferedWriter):
        assert bufferedReader.readable()
        assert bufferedWriter.writeable()
        self.bufferedReader = bufferedReader
        self.bufferedWriter = bufferedWriter
        self.read = bufferedReader.read
        self.write = bufferedWriter.write
        self.flush = bufferedWriter.flush
        self.readable = bufferedReader.readable
        self.writeable = bufferedWriter.writeable

    def seekable(self):
        return False
