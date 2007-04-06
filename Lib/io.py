"""New I/O library.

This is an early prototype; eventually some of this will be
reimplemented in C and the rest may be turned into a package.

See PEP 3116.

XXX need to default buffer size to 1 if isatty()
XXX need to support 1 meaning line-buffered
XXX change behavior of blocking I/O
XXX don't use assert to validate input requirements
"""

__author__ = ("Guido van Rossum <guido@python.org>, "
              "Mike Verdone <mike.verdone@gmail.com>, "
              "Mark Russell <mark.russell@zen.co.uk>")

__all__ = ["open", "RawIOBase", "FileIO", "SocketIO", "BytesIO",
           "BufferedReader", "BufferedWriter", "BufferedRWPair",
           "BufferedRandom"]

import os
import sys
import codecs
import warnings

DEFAULT_BUFFER_SIZE = 8 * 1024 # bytes
DEFAULT_MAX_BUFFER_SIZE = 16 * 1024 # bytes


class BlockingIO(IOError):

    def __init__(self, errno, strerror, characters_written):
        IOError.__init__(self, errno, strerror)
        self.characters_written = characters_written


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
            pass
        else:
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
    classes that want to support read() only need to implement
    readinto() as a primitive operation.
    """

    def read(self, n):
        """read(n: int) -> bytes.  Read and return up to n bytes.

        Returns an empty bytes array on EOF, or None if the object is
        set not to block and has no data to read.
        """
        b = bytes(n.__index__())
        n = self.readinto(b)
        del b[n:]
        return b

    def readinto(self, b):
        """readinto(b: bytes) -> None.  Read up to len(b) bytes into b.

        Returns number of bytes read (0 for EOF), or None if the object
        is set not to block as has no data to read.
        """
        raise IOError(".readinto() not supported")

    def write(self, b):
        """write(b: bytes) -> int.  Write the given buffer to the IO stream.

        Returns the number of bytes written, which may be less than len(b).
        """
        raise IOError(".write() not supported")

    def seek(self, pos, whence=0):
        """seek(pos: int, whence: int = 0) -> None.  Change stream position.

        Seek to byte offset pos relative to position indicated by whence:
             0  Start of stream (the default).  pos should be >= 0;
             1  Current position - whence may be negative;
             2  End of stream - whence usually negative.
        """
        raise IOError(".seek() not supported")

    def tell(self):
        """tell() -> int.  Return current stream position."""
        raise IOError(".tell() not supported")

    def truncate(self, pos=None):
        """truncate(size: int = None) -> None. Truncate file to size bytes.

        Size defaults to the current IO position as reported by tell().
        """
        raise IOError(".truncate() not supported")

    def close(self):
        """close() -> None.  Close IO object."""
        pass

    def seekable(self):
        """seekable() -> bool.  Return whether object supports random access.

        If False, seek(), tell() and truncate() will raise IOError.
        This method may need to do a test seek().
        """
        return False

    def readable(self):
        """readable() -> bool.  Return whether object was opened for reading.

        If False, read() will raise IOError.
        """
        return False

    def writable(self):
        """writable() -> bool.  Return whether object was opened for writing.

        If False, write() and truncate() will raise IOError.
        """
        return False

    def __enter__(self):
        """Context management protocol.  Returns self."""
        return self

    def __exit__(self, *args):
        """Context management protocol.  Same as close()"""
        self.close()

    def fileno(self):
        """fileno() -> int.  Return underlying file descriptor if there is one.

        Raises IOError if the IO object does not use a file descriptor.
        """
        raise IOError(".fileno() not supported")


class _PyFileIO(RawIOBase):

    """Raw I/O implementation for OS files."""

    # XXX More docs

    def __init__(self, filename, mode):
        self._seekable = None
        self._mode = mode
        if mode == "r":
            flags = os.O_RDONLY
        elif mode == "w":
            flags = os.O_WRONLY | os.O_CREAT | os.O_TRUNC
        elif mode == "r+":
            flags = os.O_RDWR
        else:
            assert 0, "unsupported mode %r (for now)" % mode
        if hasattr(os, "O_BINARY"):
            flags |= os.O_BINARY
        self._fd = os.open(filename, flags)

    def readinto(self, b):
        # XXX We really should have os.readinto()
        tmp = os.read(self._fd, len(b))
        n = len(tmp)
        b[:n] = tmp
        return n

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
        # Must be idempotent
        # XXX But what about thread-safe?
        fd = self._fd
        self._fd = -1
        if fd >= 0:
            os.close(fd)

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


try:
    import _fileio
except ImportError:
    # Let's use the Python version
    warnings.warn("Can't import _fileio, using slower Python lookalike",
                  RuntimeWarning)
    FileIO = _PyFileIO
else:
    # Create a trivial subclass with the proper inheritance structure
    class FileIO(_fileio._FileIO, RawIOBase):
        """Raw I/O implementation for OS files."""
        # XXX More docs


class SocketIO(RawIOBase):

    """Raw I/O implementation for stream sockets."""

    # XXX More docs

    def __init__(self, sock, mode):
        assert mode in ("r", "w", "rw")
        self._sock = sock
        self._mode = mode

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


class _MemoryIOBase(RawIOBase):

    # XXX docstring

    def __init__(self, buffer):
        self._buffer = buffer
        self._pos = 0

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
        tmp = self.read(len(b))
        n = len(tmp)
        b[:n] = tmp
        return n

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


class BytesIO(_MemoryIOBase):

    """Buffered I/O implementation using a bytes buffer, like StringIO."""

    # XXX More docs

    def __init__(self, inital_bytes=None):
        buffer = b""
        if inital_bytes is not None:
            buffer += inital_bytes
        _MemoryIOBase.__init__(self, buffer)


class StringIO(_MemoryIOBase):

    """Buffered I/O implementation using a string buffer, like StringIO."""

    # XXX More docs

    # XXX Reuses the same code as BytesIO, just with a string rather
    # that bytes as the _buffer value.  That won't work in C of course.

    def __init__(self, inital_string=None):
        buffer = ""
        if inital_string is not None:
            buffer += inital_string
        _MemoryIOBase.__init__(self, buffer)


class BufferedIOBase(RawIOBase):

    """Base class for buffered IO objects."""

    def flush(self):
        """Flush the buffer to the underlying raw IO object."""
        raise IOError(".flush() unsupported")

    def seekable(self):
        return self.raw.seekable()


class BufferedReader(BufferedIOBase):

    """Buffer for a readable sequential RawIO object.

    Does not allow random access (seek, tell).
    """

    def __init__(self, raw, buffer_size=DEFAULT_BUFFER_SIZE):
        """Create a new buffered reader using the given readable raw IO object.
        """
        assert raw.readable()
        self.raw = raw
        self._read_buf = b""
        self.buffer_size = buffer_size
        if hasattr(raw, 'fileno'):
            self.fileno = raw.fileno

    def read(self, n=None):
        """Read n bytes.

        Returns exactly n bytes of data unless the underlying raw IO
        stream reaches EOF of if the call would block in non-blocking
        mode. If n is None, read until EOF or until read() would
        block.
        """
        # XXX n == 0 should return b""? n < 0 should be the same as n is None?
        assert n is None or n > 0, '.read(): Bad read size %r' % n
        nodata_val = b""
        while n is None or len(self._read_buf) < n:
            to_read = max(self.buffer_size,
                          n if n is not None else 2*len(self._read_buf))
            current = self.raw.read(to_read)

            if current in (b"", None):
                nodata_val = current
                break
            self._read_buf += current
        if self._read_buf:
            if n is None:
                n = len(self._read_buf)
            out = self._read_buf[:n]
            self._read_buf = self._read_buf[n:]
        else:
            out = nodata_val
        return out

    def readable(self):
        return True

    def fileno(self):
        return self.raw.fileno()

    def flush(self):
        # Flush is a no-op
        pass

    def tell(self):
        return self.raw.tell() - len(self._read_buf)

    def seek(self, pos, whence=0):
        if whence == 1:
            pos -= len(self._read_buf)
        self.raw.seek(pos, whence)
        self._read_buf = b""

    def close(self):
        self.raw.close()


class BufferedWriter(BufferedIOBase):

    # XXX docstring

    def __init__(self, raw, buffer_size=DEFAULT_BUFFER_SIZE,
                 max_buffer_size=DEFAULT_MAX_BUFFER_SIZE):
        assert raw.writable()
        self.raw = raw
        self.buffer_size = buffer_size
        self.max_buffer_size = max_buffer_size
        self._write_buf = b""

    def write(self, b):
        # XXX we can implement some more tricks to try and avoid partial writes
        assert issubclass(type(b), bytes)
        if len(self._write_buf) > self.buffer_size:
            # We're full, so let's pre-flush the buffer
            try:
                self.flush()
            except BlockingIO as e:
                # We can't accept anything else.
                # XXX Why not just let the exception pass through?
                raise BlockingIO(e.errno, e.strerror, 0)
        self._write_buf += b
        if len(self._write_buf) > self.buffer_size:
            try:
                self.flush()
            except BlockingIO as e:
                if (len(self._write_buf) > self.max_buffer_size):
                    # We've hit max_buffer_size. We have to accept a partial
                    # write and cut back our buffer.
                    overage = len(self._write_buf) - self.max_buffer_size
                    self._write_buf = self._write_buf[:self.max_buffer_size]
                    raise BlockingIO(e.errno, e.strerror, overage)

    def writable(self):
        return True

    def flush(self):
        written = 0
        try:
            while self._write_buf:
                n = self.raw.write(self._write_buf)
                del self._write_buf[:n]
                written += n
        except BlockingIO as e:
            n = e.characters_written
            del self._write_buf[:n]
            written += n
            raise BlockingIO(e.errno, e.strerror, written)

    def tell(self):
        return self.raw.tell() + len(self._write_buf)

    def seek(self, pos, whence=0):
        self.flush()
        self.raw.seek(pos, whence)

    def fileno(self):
        return self.raw.fileno()

    def close(self):
        self.flush()
        self.raw.close()

    def __del__(self):
        self.close()


class BufferedRWPair(BufferedReader, BufferedWriter):

    """A buffered reader and writer object together.

    A buffered reader object and buffered writer object put together to
    form a sequential IO object that can read and write.

    This is typically used with a socket or two-way pipe.
    """

    def __init__(self, reader, writer, buffer_size=DEFAULT_BUFFER_SIZE,
                 max_buffer_size=DEFAULT_MAX_BUFFER_SIZE):
        assert reader.readable()
        assert writer.writable()
        BufferedReader.__init__(self, reader)
        BufferedWriter.__init__(self, writer, buffer_size, max_buffer_size)
        self.reader = reader
        self.writer = writer

    def read(self, n=None):
        return self.reader.read(n)

    def write(self, b):
        return self.writer.write(b)

    def readable(self):
        return self.reader.readable()

    def writable(self):
        return self.writer.writable()

    def flush(self):
        return self.writer.flush()

    def seekable(self):
        return False

    def fileno(self):
        # XXX whose fileno do we return? Reader's? Writer's? Unsupported?
        raise IOError(".fileno() unsupported")

    def close(self):
        self.reader.close()
        self.writer.close()


class BufferedRandom(BufferedReader, BufferedWriter):

    # XXX docstring

    def __init__(self, raw, buffer_size=DEFAULT_BUFFER_SIZE,
                 max_buffer_size=DEFAULT_MAX_BUFFER_SIZE):
        assert raw.seekable()
        BufferedReader.__init__(self, raw)
        BufferedWriter.__init__(self, raw, buffer_size, max_buffer_size)

    def readable(self):
        return self.raw.readable()

    def writable(self):
        return self.raw.writable()

    def seek(self, pos, whence=0):
        self.flush()
        # First do the raw seek, then empty the read buffer, so that
        # if the raw seek fails, we don't lose buffered data forever.
        self.raw.seek(pos, whence)
        self._read_buf = b""
        # XXX I suppose we could implement some magic here to move through the
        # existing read buffer in the case of seek(<some small +ve number>, 1)
        # XXX OTOH it might be good to *guarantee* that the buffer is
        # empty after a seek or flush; for small relative forward
        # seeks one might as well use small reads instead.

    def tell(self):
        if (self._write_buf):
            return self.raw.tell() + len(self._write_buf)
        else:
            return self.raw.tell() - len(self._read_buf)

    def read(self, n=None):
        self.flush()
        return BufferedReader.read(self, n)

    def write(self, b):
        if self._read_buf:
            self.raw.seek(-len(self._read_buf), 1) # Undo readahead
            self._read_buf = b""
        return BufferedWriter.write(self, b)

    def flush(self):
        BufferedWriter.flush(self)

    def close(self):
        self.raw.close()


class TextIOBase(BufferedIOBase):

    """Base class for text I/O.

    This class provides a character and line based interface to stream I/O.
    """

    def read(self, n: int = -1) -> str:
        """read(n: int = -1) -> str.  Read at most n characters from stream.

        Read from underlying buffer until we have n characters or we hit EOF.
        If n is negative or omitted, read until EOF.
        """
        raise IOError(".read() not supported")

    def write(self, s: str):
        """write(s: str) -> None.  Write string s to stream.
        """
        raise IOError(".write() not supported")

    def readline(self) -> str:
        """readline() -> str.  Read until newline or EOF.

        Returns an empty string if EOF is hit immediately.
        """
        raise IOError(".readline() not supported")

    def __iter__(self):
        """__iter__() -> Iterator.  Return line iterator (actually just self).
        """
        return self

    def next(self):
        """Same as readline() except raises StopIteration on immediate EOF.
        """
        line = self.readline()
        if line == '':
            raise StopIteration
        return line


class TextIOWrapper(TextIOBase):

    """Buffered text stream.

    Character and line based layer over a BufferedIOBase object.
    """

    # XXX tell(), seek()

    def __init__(self, buffer, encoding=None, newline=None):
        if newline not in (None, '\n', '\r\n'):
            raise IOError("illegal newline %s" % newline) # XXX: ValueError?
        if encoding is None:
            # XXX This is questionable
            encoding = sys.getfilesystemencoding()
            if encoding is None:
                encoding = "latin-1"  # XXX, but this is best for transparancy

        self.buffer = buffer
        self._encoding = encoding
        self._newline = newline or os.linesep
        self._fix_newlines = newline is None
        self._decoder = None
        self._pending = ''

    def write(self, s: str):
        return self.buffer.write(s.encode(self._encoding))

    def _get_decoder(self):
        make_decoder = codecs.getincrementaldecoder(self._encoding)
        if make_decoder is None:
            raise IOError(".readline() not supported for encoding %s" %
                          self._encoding)
        decoder = self._decoder = make_decoder()  # XXX: errors
        if isinstance(decoder, codecs.BufferedIncrementalDecoder):
            # XXX Hack: make the codec use bytes instead of strings
            decoder.buffer = b""
        return decoder

    def read(self, n: int = -1):
        decoder = self._decoder or self._get_decoder()
        res = self._pending
        if n < 0:
            res += decoder.decode(self.buffer.read(), True)
            self._pending = ''
            return res
        else:
            while len(res) < n:
                data = self.buffer.read(64)
                res += decoder.decode(data, not data)
                if not data:
                    break
            self._pending = res[n:]
            return res[:n]

    def readline(self):
        line = self._pending
        start = 0
        decoder = self._decoder or self._get_decoder()

        while True:
            # In C we'd look for these in parallel of course.
            nlpos = line.find("\n", start)
            crpos = line.find("\r", start)
            if nlpos >= 0 and crpos >= 0:
                endpos = min(nlpos, crpos)
            else:
                endpos = nlpos if nlpos >= 0 else crpos

            if endpos != -1:
                endc = line[endpos]
                if endc == "\n":
                    ending = "\n"
                    break

                # We've seen \r - is it standalone, \r\n or \r at end of line?
                if endpos + 1 < len(line):
                    if line[endpos+1] == '\n':
                        ending = "\r\n"
                    else:
                        ending = "\r"
                    break
                # There might be a following \n in the next block of data ...
                start = endpos
            else:
                start = len(line)

            # No line ending seen yet - get more data
            while True:
                data = self.buffer.read(64)
                more_line = decoder.decode(data, not data)
                if more_line != "" or not data:
                    break

            if more_line == "":
                ending = ''
                endpos = len(line)
                break

            line += more_line

        nextpos = endpos + len(ending)
        self._pending = line[nextpos:]

        # XXX Update self.newlines here if we want to support that

        if self._fix_newlines and ending != "\n" and ending != '':
            return line[:endpos] + "\n"
        else:
            return line[:nextpos]
