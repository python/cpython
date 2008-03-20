"""New I/O library conforming to PEP 3116.

This is a prototype; hopefully eventually some of this will be
reimplemented in C.

Conformance of alternative implementations: all arguments are intended
to be positional-only except the arguments of the open() function.
Argument names except those of the open() function are not part of the
specification.  Instance variables and methods whose name starts with
a leading underscore are not part of the specification (except "magic"
names like __iter__).  Only the top-level names listed in the __all__
variable are part of the specification.

XXX edge cases when switching between reading/writing
XXX need to support 1 meaning line-buffered
XXX whenever an argument is None, use the default value
XXX read/write ops should check readable/writable
XXX buffered readinto should work with arbitrary buffer objects
XXX use incremental encoder for text output, at least for UTF-16 and UTF-8-SIG
XXX check writable, readable and seekable in appropriate places
"""

__author__ = ("Guido van Rossum <guido@python.org>, "
              "Mike Verdone <mike.verdone@gmail.com>, "
              "Mark Russell <mark.russell@zen.co.uk>")

__all__ = ["BlockingIOError", "open", "IOBase", "RawIOBase", "FileIO",
           "BytesIO", "StringIO", "BufferedIOBase",
           "BufferedReader", "BufferedWriter", "BufferedRWPair",
           "BufferedRandom", "TextIOBase", "TextIOWrapper"]

import os
import abc
import sys
import codecs
import _fileio
import warnings

# open() uses st_blksize whenever we can
DEFAULT_BUFFER_SIZE = 8 * 1024  # bytes


class BlockingIOError(IOError):

    """Exception raised when I/O would block on a non-blocking I/O stream."""

    def __init__(self, errno, strerror, characters_written=0):
        IOError.__init__(self, errno, strerror)
        self.characters_written = characters_written


def open(file, mode="r", buffering=None, encoding=None, errors=None,
         newline=None, closefd=True):
    r"""Replacement for the built-in open function.

    Args:
      file: string giving the name of the file to be opened;
            or integer file descriptor of the file to be wrapped (*).
      mode: optional mode string; see below.
      buffering: optional int >= 0 giving the buffer size; values
                 can be: 0 = unbuffered, 1 = line buffered,
                 larger = fully buffered.
      encoding: optional string giving the text encoding.
      errors: optional string giving the encoding error handling.
      newline: optional newlines specifier; must be None, '', '\n', '\r'
               or '\r\n'; all other values are illegal.  It controls the
               handling of line endings.  It works as follows:

        * On input, if `newline` is `None`, universal newlines
          mode is enabled.  Lines in the input can end in `'\n'`,
          `'\r'`, or `'\r\n'`, and these are translated into
          `'\n'` before being returned to the caller.  If it is
          `''`, universal newline mode is enabled, but line endings
          are returned to the caller untranslated.  If it has any of
          the other legal values, input lines are only terminated by
          the given string, and the line ending is returned to the
          caller untranslated.

        * On output, if `newline` is `None`, any `'\n'`
          characters written are translated to the system default
          line separator, `os.linesep`.  If `newline` is `''`,
          no translation takes place.  If `newline` is any of the
          other legal values, any `'\n'` characters written are
          translated to the given string.

      closefd: optional argument to keep the underlying file descriptor
               open when the file is closed.  It must not be false when
               a filename is given.

    (*) If a file descriptor is given, it is closed when the returned
    I/O object is closed, unless closefd=False is given.

    Mode strings characters:
      'r': open for reading (default)
      'w': open for writing, truncating the file first
      'a': open for writing, appending to the end if the file exists
      'b': binary mode
      't': text mode (default)
      '+': open a disk file for updating (implies reading and writing)
      'U': universal newline mode (for backwards compatibility)

    Constraints:
      - encoding or errors must not be given when a binary mode is given
      - buffering must not be zero when a text mode is given

    Returns:
      Depending on the mode and buffering arguments, either a raw
      binary stream, a buffered binary stream, or a buffered text
      stream, open for reading and/or writing.
    """
    if not isinstance(file, (str, int)):
        raise TypeError("invalid file: %r" % file)
    if not isinstance(mode, str):
        raise TypeError("invalid mode: %r" % mode)
    if buffering is not None and not isinstance(buffering, int):
        raise TypeError("invalid buffering: %r" % buffering)
    if encoding is not None and not isinstance(encoding, str):
        raise TypeError("invalid encoding: %r" % encoding)
    if errors is not None and not isinstance(errors, str):
        raise TypeError("invalid errors: %r" % errors)
    modes = set(mode)
    if modes - set("arwb+tU") or len(mode) > len(modes):
        raise ValueError("invalid mode: %r" % mode)
    reading = "r" in modes
    writing = "w" in modes
    appending = "a" in modes
    updating = "+" in modes
    text = "t" in modes
    binary = "b" in modes
    if "U" in modes:
        if writing or appending:
            raise ValueError("can't use U and writing mode at once")
        reading = True
    if text and binary:
        raise ValueError("can't have text and binary mode at once")
    if reading + writing + appending > 1:
        raise ValueError("can't have read/write/append mode at once")
    if not (reading or writing or appending):
        raise ValueError("must have exactly one of read/write/append mode")
    if binary and encoding is not None:
        raise ValueError("binary mode doesn't take an encoding argument")
    if binary and errors is not None:
        raise ValueError("binary mode doesn't take an errors argument")
    if binary and newline is not None:
        raise ValueError("binary mode doesn't take a newline argument")
    raw = FileIO(file,
                 (reading and "r" or "") +
                 (writing and "w" or "") +
                 (appending and "a" or "") +
                 (updating and "+" or ""),
                 closefd)
    if buffering is None:
        buffering = -1
    line_buffering = False
    if buffering == 1 or buffering < 0 and raw.isatty():
        buffering = -1
        line_buffering = True
    if buffering < 0:
        buffering = DEFAULT_BUFFER_SIZE
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
            raw._name = file
            raw._mode = mode
            return raw
        raise ValueError("can't have unbuffered text I/O")
    if updating:
        buffer = BufferedRandom(raw, buffering)
    elif writing or appending:
        buffer = BufferedWriter(raw, buffering)
    elif reading:
        buffer = BufferedReader(raw, buffering)
    else:
        raise ValueError("unknown mode: %r" % mode)
    if binary:
        buffer.name = file
        buffer.mode = mode
        return buffer
    text = TextIOWrapper(buffer, encoding, errors, newline, line_buffering)
    text.name = file
    text.mode = mode
    return text

class _DocDescriptor:
    """Helper for builtins.open.__doc__
    """
    def __get__(self, obj, typ):
        return (
            "open(file, mode='r', buffering=None, encoding=None, "
                 "errors=None, newline=None, closefd=True)\n\n" +
            open.__doc__)

class OpenWrapper:
    """Wrapper for builtins.open

    Trick so that open won't become a bound method when stored
    as a class variable (as dumbdbm does).

    See initstdio() in Python/pythonrun.c.
    """
    __doc__ = _DocDescriptor()

    def __new__(cls, *args, **kwargs):
        return open(*args, **kwargs)


class UnsupportedOperation(ValueError, IOError):
    pass


class IOBase(metaclass=abc.ABCMeta):

    """Base class for all I/O classes.

    This class provides dummy implementations for many methods that
    derived classes can override selectively; the default
    implementations represent a file that cannot be read, written or
    seeked.

    This does not define read(), readinto() and write(), nor
    readline() and friends, since their signatures vary per layer.

    Not that calling any method (even inquiries) on a closed file is
    undefined.  Implementations may raise IOError in this case.
    """

    ### Internal ###

    def _unsupported(self, name: str) -> IOError:
        """Internal: raise an exception for unsupported operations."""
        raise UnsupportedOperation("%s.%s() not supported" %
                                   (self.__class__.__name__, name))

    ### Positioning ###

    def seek(self, pos: int, whence: int = 0) -> int:
        """seek(pos: int, whence: int = 0) -> int.  Change stream position.

        Seek to byte offset pos relative to position indicated by whence:
             0  Start of stream (the default).  pos should be >= 0;
             1  Current position - whence may be negative;
             2  End of stream - whence usually negative.
        Returns the new absolute position.
        """
        self._unsupported("seek")

    def tell(self) -> int:
        """tell() -> int.  Return current stream position."""
        return self.seek(0, 1)

    def truncate(self, pos: int = None) -> int:
        """truncate(size: int = None) -> int. Truncate file to size bytes.

        Size defaults to the current IO position as reported by tell().
        Returns the new size.
        """
        self._unsupported("truncate")

    ### Flush and close ###

    def flush(self) -> None:
        """flush() -> None.  Flushes write buffers, if applicable.

        This is a no-op for read-only and non-blocking streams.
        """
        # XXX Should this return the number of bytes written???

    __closed = False

    def close(self) -> None:
        """close() -> None.  Flushes and closes the IO object.

        This must be idempotent.  It should also set a flag for the
        'closed' property (see below) to test.
        """
        if not self.__closed:
            try:
                self.flush()
            except IOError:
                pass  # If flush() fails, just give up
            self.__closed = True

    def __del__(self) -> None:
        """Destructor.  Calls close()."""
        # The try/except block is in case this is called at program
        # exit time, when it's possible that globals have already been
        # deleted, and then the close() call might fail.  Since
        # there's nothing we can do about such failures and they annoy
        # the end users, we suppress the traceback.
        try:
            self.close()
        except:
            pass

    ### Inquiries ###

    def seekable(self) -> bool:
        """seekable() -> bool.  Return whether object supports random access.

        If False, seek(), tell() and truncate() will raise IOError.
        This method may need to do a test seek().
        """
        return False

    def _checkSeekable(self, msg=None):
        """Internal: raise an IOError if file is not seekable
        """
        if not self.seekable():
            raise IOError("File or stream is not seekable."
                          if msg is None else msg)


    def readable(self) -> bool:
        """readable() -> bool.  Return whether object was opened for reading.

        If False, read() will raise IOError.
        """
        return False

    def _checkReadable(self, msg=None):
        """Internal: raise an IOError if file is not readable
        """
        if not self.readable():
            raise IOError("File or stream is not readable."
                          if msg is None else msg)

    def writable(self) -> bool:
        """writable() -> bool.  Return whether object was opened for writing.

        If False, write() and truncate() will raise IOError.
        """
        return False

    def _checkWritable(self, msg=None):
        """Internal: raise an IOError if file is not writable
        """
        if not self.writable():
            raise IOError("File or stream is not writable."
                          if msg is None else msg)

    @property
    def closed(self):
        """closed: bool.  True iff the file has been closed.

        For backwards compatibility, this is a property, not a predicate.
        """
        return self.__closed

    def _checkClosed(self, msg=None):
        """Internal: raise an ValueError if file is closed
        """
        if self.closed:
            raise ValueError("I/O operation on closed file."
                             if msg is None else msg)

    ### Context manager ###

    def __enter__(self) -> "IOBase":  # That's a forward reference
        """Context management protocol.  Returns self."""
        self._checkClosed()
        return self

    def __exit__(self, *args) -> None:
        """Context management protocol.  Calls close()"""
        self.close()

    ### Lower-level APIs ###

    # XXX Should these be present even if unimplemented?

    def fileno(self) -> int:
        """fileno() -> int.  Returns underlying file descriptor if one exists.

        Raises IOError if the IO object does not use a file descriptor.
        """
        self._unsupported("fileno")

    def isatty(self) -> bool:
        """isatty() -> int.  Returns whether this is an 'interactive' stream.

        Returns False if we don't know.
        """
        self._checkClosed()
        return False

    ### Readline[s] and writelines ###

    def readline(self, limit: int = -1) -> bytes:
        """For backwards compatibility, a (slowish) readline()."""
        if hasattr(self, "peek"):
            def nreadahead():
                readahead = self.peek(1)
                if not readahead:
                    return 1
                n = (readahead.find(b"\n") + 1) or len(readahead)
                if limit >= 0:
                    n = min(n, limit)
                return n
        else:
            def nreadahead():
                return 1
        if limit is None:
            limit = -1
        res = bytearray()
        while limit < 0 or len(res) < limit:
            b = self.read(nreadahead())
            if not b:
                break
            res += b
            if res.endswith(b"\n"):
                break
        return bytes(res)

    def __iter__(self):
        self._checkClosed()
        return self

    def __next__(self):
        line = self.readline()
        if not line:
            raise StopIteration
        return line

    def readlines(self, hint=None):
        if hint is None:
            return list(self)
        n = 0
        lines = []
        for line in self:
            lines.append(line)
            n += len(line)
            if n >= hint:
                break
        return lines

    def writelines(self, lines):
        self._checkClosed()
        for line in lines:
            self.write(line)


class RawIOBase(IOBase):

    """Base class for raw binary I/O.

    The read() method is implemented by calling readinto(); derived
    classes that want to support read() only need to implement
    readinto() as a primitive operation.  In general, readinto()
    can be more efficient than read().

    (It would be tempting to also provide an implementation of
    readinto() in terms of read(), in case the latter is a more
    suitable primitive operation, but that would lead to nasty
    recursion in case a subclass doesn't implement either.)
    """

    def read(self, n: int = -1) -> bytes:
        """read(n: int) -> bytes.  Read and return up to n bytes.

        Returns an empty bytes array on EOF, or None if the object is
        set not to block and has no data to read.
        """
        if n is None:
            n = -1
        if n < 0:
            return self.readall()
        b = bytearray(n.__index__())
        n = self.readinto(b)
        del b[n:]
        return bytes(b)

    def readall(self):
        """readall() -> bytes.  Read until EOF, using multiple read() call."""
        res = bytearray()
        while True:
            data = self.read(DEFAULT_BUFFER_SIZE)
            if not data:
                break
            res += data
        return bytes(res)

    def readinto(self, b: bytes) -> int:
        """readinto(b: bytes) -> int.  Read up to len(b) bytes into b.

        Returns number of bytes read (0 for EOF), or None if the object
        is set not to block as has no data to read.
        """
        self._unsupported("readinto")

    def write(self, b: bytes) -> int:
        """write(b: bytes) -> int.  Write the given buffer to the IO stream.

        Returns the number of bytes written, which may be less than len(b).
        """
        self._unsupported("write")


class FileIO(_fileio._FileIO, RawIOBase):

    """Raw I/O implementation for OS files.

    This multiply inherits from _FileIO and RawIOBase to make
    isinstance(io.FileIO(), io.RawIOBase) return True without
    requiring that _fileio._FileIO inherits from io.RawIOBase (which
    would be hard to do since _fileio.c is written in C).
    """

    def close(self):
        _fileio._FileIO.close(self)
        RawIOBase.close(self)

    @property
    def name(self):
        return self._name

    @property
    def mode(self):
        return self._mode


class BufferedIOBase(IOBase):

    """Base class for buffered IO objects.

    The main difference with RawIOBase is that the read() method
    supports omitting the size argument, and does not have a default
    implementation that defers to readinto().

    In addition, read(), readinto() and write() may raise
    BlockingIOError if the underlying raw stream is in non-blocking
    mode and not ready; unlike their raw counterparts, they will never
    return None.

    A typical implementation should not inherit from a RawIOBase
    implementation, but wrap one.
    """

    def read(self, n: int = None) -> bytes:
        """read(n: int = None) -> bytes.  Read and return up to n bytes.

        If the argument is omitted, None, or negative, reads and
        returns all data until EOF.

        If the argument is positive, and the underlying raw stream is
        not 'interactive', multiple raw reads may be issued to satisfy
        the byte count (unless EOF is reached first).  But for
        interactive raw streams (XXX and for pipes?), at most one raw
        read will be issued, and a short result does not imply that
        EOF is imminent.

        Returns an empty bytes array on EOF.

        Raises BlockingIOError if the underlying raw stream has no
        data at the moment.
        """
        self._unsupported("read")

    def readinto(self, b: bytes) -> int:
        """readinto(b: bytes) -> int.  Read up to len(b) bytes into b.

        Like read(), this may issue multiple reads to the underlying
        raw stream, unless the latter is 'interactive' (XXX or a
        pipe?).

        Returns the number of bytes read (0 for EOF).

        Raises BlockingIOError if the underlying raw stream has no
        data at the moment.
        """
        # XXX This ought to work with anything that supports the buffer API
        data = self.read(len(b))
        n = len(data)
        try:
            b[:n] = data
        except TypeError as err:
            import array
            if not isinstance(b, array.array):
                raise err
            b[:n] = array.array('b', data)
        return n

    def write(self, b: bytes) -> int:
        """write(b: bytes) -> int.  Write the given buffer to the IO stream.

        Returns the number of bytes written, which is never less than
        len(b).

        Raises BlockingIOError if the buffer is full and the
        underlying raw stream cannot accept more data at the moment.
        """
        self._unsupported("write")


class _BufferedIOMixin(BufferedIOBase):

    """A mixin implementation of BufferedIOBase with an underlying raw stream.

    This passes most requests on to the underlying raw stream.  It
    does *not* provide implementations of read(), readinto() or
    write().
    """

    def __init__(self, raw):
        self.raw = raw

    ### Positioning ###

    def seek(self, pos, whence=0):
        return self.raw.seek(pos, whence)

    def tell(self):
        return self.raw.tell()

    def truncate(self, pos=None):
        # Flush the stream.  We're mixing buffered I/O with lower-level I/O,
        # and a flush may be necessary to synch both views of the current
        # file state.
        self.flush()

        if pos is None:
            pos = self.tell()
        return self.raw.truncate(pos)

    ### Flush and close ###

    def flush(self):
        self.raw.flush()

    def close(self):
        if not self.closed:
            try:
                self.flush()
            except IOError:
                pass  # If flush() fails, just give up
            self.raw.close()

    ### Inquiries ###

    def seekable(self):
        return self.raw.seekable()

    def readable(self):
        return self.raw.readable()

    def writable(self):
        return self.raw.writable()

    @property
    def closed(self):
        return self.raw.closed

    ### Lower-level APIs ###

    def fileno(self):
        return self.raw.fileno()

    def isatty(self):
        return self.raw.isatty()


class BytesIO(BufferedIOBase):

    """Buffered I/O implementation using an in-memory bytes buffer."""

    # XXX More docs

    def __init__(self, initial_bytes=None):
        buf = bytearray()
        if initial_bytes is not None:
            buf += initial_bytes
        self._buffer = buf
        self._pos = 0

    def getvalue(self):
        return bytes(self._buffer)

    def read(self, n=None):
        if n is None:
            n = -1
        if n < 0:
            n = len(self._buffer)
        newpos = min(len(self._buffer), self._pos + n)
        b = self._buffer[self._pos : newpos]
        self._pos = newpos
        return bytes(b)

    def read1(self, n):
        return self.read(n)

    def write(self, b):
        if self.closed:
            raise ValueError("write to closed file")
        if isinstance(b, str):
            raise TypeError("can't write str to binary stream")
        n = len(b)
        newpos = self._pos + n
        if newpos > len(self._buffer):
            # Inserts null bytes between the current end of the file
            # and the new write position.
            padding = b'\x00' * (newpos - len(self._buffer) - n)
            self._buffer[self._pos:newpos - n] = padding
        self._buffer[self._pos:newpos] = b
        self._pos = newpos
        return n

    def seek(self, pos, whence=0):
        try:
            pos = pos.__index__()
        except AttributeError as err:
            raise TypeError("an integer is required") from err
        if whence == 0:
            self._pos = max(0, pos)
        elif whence == 1:
            self._pos = max(0, self._pos + pos)
        elif whence == 2:
            self._pos = max(0, len(self._buffer) + pos)
        else:
            raise IOError("invalid whence value")
        return self._pos

    def tell(self):
        return self._pos

    def truncate(self, pos=None):
        if pos is None:
            pos = self._pos
        del self._buffer[pos:]
        return pos

    def readable(self):
        return True

    def writable(self):
        return True

    def seekable(self):
        return True


class BufferedReader(_BufferedIOMixin):

    """Buffer for a readable sequential RawIO object."""

    def __init__(self, raw, buffer_size=DEFAULT_BUFFER_SIZE):
        """Create a new buffered reader using the given readable raw IO object.
        """
        raw._checkReadable()
        _BufferedIOMixin.__init__(self, raw)
        self._read_buf = b""
        self.buffer_size = buffer_size

    def read(self, n=None):
        """Read n bytes.

        Returns exactly n bytes of data unless the underlying raw IO
        stream reaches EOF or if the call would block in non-blocking
        mode. If n is negative, read until EOF or until read() would
        block.
        """
        if n is None:
            n = -1
        nodata_val = b""
        while n < 0 or len(self._read_buf) < n:
            to_read = max(self.buffer_size,
                          n if n is not None else 2*len(self._read_buf))
            current = self.raw.read(to_read)
            if current in (b"", None):
                nodata_val = current
                break
            self._read_buf += current
        if self._read_buf:
            if n < 0:
                n = len(self._read_buf)
            out = self._read_buf[:n]
            self._read_buf = self._read_buf[n:]
        else:
            out = nodata_val
        return out

    def peek(self, n=0):
        """Returns buffered bytes without advancing the position.

        The argument indicates a desired minimal number of bytes; we
        do at most one raw read to satisfy it.  We never return more
        than self.buffer_size.
        """
        want = min(n, self.buffer_size)
        have = len(self._read_buf)
        if have < want:
            to_read = self.buffer_size - have
            current = self.raw.read(to_read)
            if current:
                self._read_buf += current
        return self._read_buf

    def read1(self, n):
        """Reads up to n bytes, with at most one read() system call.

        Returns up to n bytes.  If at least one byte is buffered, we
        only return buffered bytes.  Otherwise, we do one raw read.
        """
        if n <= 0:
            return b""
        self.peek(1)
        return self.read(min(n, len(self._read_buf)))

    def tell(self):
        return self.raw.tell() - len(self._read_buf)

    def seek(self, pos, whence=0):
        if whence == 1:
            pos -= len(self._read_buf)
        pos = self.raw.seek(pos, whence)
        self._read_buf = b""
        return pos


class BufferedWriter(_BufferedIOMixin):

    # XXX docstring

    def __init__(self, raw,
                 buffer_size=DEFAULT_BUFFER_SIZE, max_buffer_size=None):
        raw._checkWritable()
        _BufferedIOMixin.__init__(self, raw)
        self.buffer_size = buffer_size
        self.max_buffer_size = (2*buffer_size
                                if max_buffer_size is None
                                else max_buffer_size)
        self._write_buf = bytearray()

    def write(self, b):
        if self.closed:
            raise ValueError("write to closed file")
        if isinstance(b, str):
            raise TypeError("can't write str to binary stream")
        # XXX we can implement some more tricks to try and avoid partial writes
        if len(self._write_buf) > self.buffer_size:
            # We're full, so let's pre-flush the buffer
            try:
                self.flush()
            except BlockingIOError as e:
                # We can't accept anything else.
                # XXX Why not just let the exception pass through?
                raise BlockingIOError(e.errno, e.strerror, 0)
        before = len(self._write_buf)
        self._write_buf.extend(b)
        written = len(self._write_buf) - before
        if len(self._write_buf) > self.buffer_size:
            try:
                self.flush()
            except BlockingIOError as e:
                if (len(self._write_buf) > self.max_buffer_size):
                    # We've hit max_buffer_size. We have to accept a partial
                    # write and cut back our buffer.
                    overage = len(self._write_buf) - self.max_buffer_size
                    self._write_buf = self._write_buf[:self.max_buffer_size]
                    raise BlockingIOError(e.errno, e.strerror, overage)
        return written

    def flush(self):
        if self.closed:
            raise ValueError("flush of closed file")
        written = 0
        try:
            while self._write_buf:
                n = self.raw.write(self._write_buf)
                del self._write_buf[:n]
                written += n
        except BlockingIOError as e:
            n = e.characters_written
            del self._write_buf[:n]
            written += n
            raise BlockingIOError(e.errno, e.strerror, written)

    def tell(self):
        return self.raw.tell() + len(self._write_buf)

    def seek(self, pos, whence=0):
        self.flush()
        return self.raw.seek(pos, whence)


class BufferedRWPair(BufferedIOBase):

    """A buffered reader and writer object together.

    A buffered reader object and buffered writer object put together
    to form a sequential IO object that can read and write.

    This is typically used with a socket or two-way pipe.

    XXX The usefulness of this (compared to having two separate IO
    objects) is questionable.
    """

    def __init__(self, reader, writer,
                 buffer_size=DEFAULT_BUFFER_SIZE, max_buffer_size=None):
        """Constructor.

        The arguments are two RawIO instances.
        """
        reader._checkReadable()
        writer._checkWritable()
        self.reader = BufferedReader(reader, buffer_size)
        self.writer = BufferedWriter(writer, buffer_size, max_buffer_size)

    def read(self, n=None):
        if n is None:
            n = -1
        return self.reader.read(n)

    def readinto(self, b):
        return self.reader.readinto(b)

    def write(self, b):
        return self.writer.write(b)

    def peek(self, n=0):
        return self.reader.peek(n)

    def read1(self, n):
        return self.reader.read1(n)

    def readable(self):
        return self.reader.readable()

    def writable(self):
        return self.writer.writable()

    def flush(self):
        return self.writer.flush()

    def close(self):
        self.writer.close()
        self.reader.close()

    def isatty(self):
        return self.reader.isatty() or self.writer.isatty()

    @property
    def closed(self):
        return self.writer.closed()


class BufferedRandom(BufferedWriter, BufferedReader):

    # XXX docstring

    def __init__(self, raw,
                 buffer_size=DEFAULT_BUFFER_SIZE, max_buffer_size=None):
        raw._checkSeekable()
        BufferedReader.__init__(self, raw, buffer_size)
        BufferedWriter.__init__(self, raw, buffer_size, max_buffer_size)

    def seek(self, pos, whence=0):
        self.flush()
        # First do the raw seek, then empty the read buffer, so that
        # if the raw seek fails, we don't lose buffered data forever.
        pos = self.raw.seek(pos, whence)
        self._read_buf = b""
        return pos

    def tell(self):
        if (self._write_buf):
            return self.raw.tell() + len(self._write_buf)
        else:
            return self.raw.tell() - len(self._read_buf)

    def read(self, n=None):
        if n is None:
            n = -1
        self.flush()
        return BufferedReader.read(self, n)

    def readinto(self, b):
        self.flush()
        return BufferedReader.readinto(self, b)

    def peek(self, n=0):
        self.flush()
        return BufferedReader.peek(self, n)

    def read1(self, n):
        self.flush()
        return BufferedReader.read1(self, n)

    def write(self, b):
        if self._read_buf:
            self.raw.seek(-len(self._read_buf), 1) # Undo readahead
            self._read_buf = b""
        return BufferedWriter.write(self, b)


class TextIOBase(IOBase):

    """Base class for text I/O.

    This class provides a character and line based interface to stream I/O.

    There is no readinto() method, as character strings are immutable.
    """

    def read(self, n: int = -1) -> str:
        """read(n: int = -1) -> str.  Read at most n characters from stream.

        Read from underlying buffer until we have n characters or we hit EOF.
        If n is negative or omitted, read until EOF.
        """
        self._unsupported("read")

    def write(self, s: str) -> int:
        """write(s: str) -> int.  Write string s to stream."""
        self._unsupported("write")

    def truncate(self, pos: int = None) -> int:
        """truncate(pos: int = None) -> int.  Truncate size to pos."""
        self.flush()
        if pos is None:
            pos = self.tell()
        self.seek(pos)
        return self.buffer.truncate()

    def readline(self) -> str:
        """readline() -> str.  Read until newline or EOF.

        Returns an empty string if EOF is hit immediately.
        """
        self._unsupported("readline")

    @property
    def encoding(self):
        """Subclasses should override."""
        return None

    @property
    def newlines(self):
        """newlines -> None | str | tuple of str. Line endings translated
        so far.

        Only line endings translated during reading are considered.

        Subclasses should override.
        """
        return None


class IncrementalNewlineDecoder(codecs.IncrementalDecoder):
    """Codec used when reading a file in universal newlines mode.
    It wraps another incremental decoder, translating \\r\\n and \\r into \\n.
    It also records the types of newlines encountered.
    When used with translate=False, it ensures that the newline sequence is
    returned in one piece.
    """
    def __init__(self, decoder, translate, errors='strict'):
        codecs.IncrementalDecoder.__init__(self, errors=errors)
        self.buffer = b''
        self.translate = translate
        self.decoder = decoder
        self.seennl = 0

    def decode(self, input, final=False):
        # decode input (with the eventual \r from a previous pass)
        if self.buffer:
            input = self.buffer + input

        output = self.decoder.decode(input, final=final)

        # retain last \r even when not translating data:
        # then readline() is sure to get \r\n in one pass
        if output.endswith("\r") and not final:
            output = output[:-1]
            self.buffer = b'\r'
        else:
            self.buffer = b''

        # Record which newlines are read
        crlf = output.count('\r\n')
        cr = output.count('\r') - crlf
        lf = output.count('\n') - crlf
        self.seennl |= (lf and self._LF) | (cr and self._CR) \
                    | (crlf and self._CRLF)

        if self.translate:
            if crlf:
                output = output.replace("\r\n", "\n")
            if cr:
                output = output.replace("\r", "\n")

        return output

    def getstate(self):
        buf, flag = self.decoder.getstate()
        return buf + self.buffer, flag

    def setstate(self, state):
        buf, flag = state
        if buf.endswith(b'\r'):
            self.buffer = b'\r'
            buf = buf[:-1]
        else:
            self.buffer = b''
        self.decoder.setstate((buf, flag))

    def reset(self):
        self.seennl = 0
        self.buffer = b''
        self.decoder.reset()

    _LF = 1
    _CR = 2
    _CRLF = 4

    @property
    def newlines(self):
        return (None,
                "\n",
                "\r",
                ("\r", "\n"),
                "\r\n",
                ("\n", "\r\n"),
                ("\r", "\r\n"),
                ("\r", "\n", "\r\n")
               )[self.seennl]


class TextIOWrapper(TextIOBase):

    """Buffered text stream.

    Character and line based layer over a BufferedIOBase object.
    """

    _CHUNK_SIZE = 128

    def __init__(self, buffer, encoding=None, errors=None, newline=None,
                 line_buffering=False):
        if newline not in (None, "", "\n", "\r", "\r\n"):
            raise ValueError("illegal newline value: %r" % (newline,))
        if encoding is None:
            try:
                encoding = os.device_encoding(buffer.fileno())
            except (AttributeError, UnsupportedOperation):
                pass
            if encoding is None:
                try:
                    import locale
                except ImportError:
                    # Importing locale may fail if Python is being built
                    encoding = "ascii"
                else:
                    encoding = locale.getpreferredencoding()

        if not isinstance(encoding, str):
            raise ValueError("invalid encoding: %r" % encoding)

        if errors is None:
            errors = "strict"
        else:
            if not isinstance(errors, str):
                raise ValueError("invalid errors: %r" % errors)

        self.buffer = buffer
        self._line_buffering = line_buffering
        self._encoding = encoding
        self._errors = errors
        self._readuniversal = not newline
        self._readtranslate = newline is None
        self._readnl = newline
        self._writetranslate = newline != ''
        self._writenl = newline or os.linesep
        self._encoder = None
        self._decoder = None
        self._decoded_text = ""  # buffer for text produced by decoder
        self._decoded_text_offset = 0  # offset to text returned by read()
        self._snapshot = None  # info for reconstructing decoder state
        self._seekable = self._telling = self.buffer.seekable()

    # A word about _snapshot.  This attribute is either None, or a tuple
    # (decoder_state, next_input) where decoder_state is the second
    # (integer) item of the decoder state, and next_input is the chunk
    # of bytes that comes after the snapshot point in the input.
    # We use this to reconstruct intermediate decoder states in tell().

    # Naming convention:
    #   - integer variables ending in "_bytes" count input bytes
    #   - integer variables ending in "_chars" count decoded characters

    def __repr__(self):
        return '<TIOW %x>' % id(self)

    @property
    def encoding(self):
        return self._encoding

    @property
    def errors(self):
        return self._errors

    @property
    def line_buffering(self):
        return self._line_buffering

    def seekable(self):
        return self._seekable

    def flush(self):
        self.buffer.flush()
        self._telling = self._seekable

    def close(self):
        try:
            self.flush()
        except:
            pass  # If flush() fails, just give up
        self.buffer.close()

    @property
    def closed(self):
        return self.buffer.closed

    def fileno(self):
        return self.buffer.fileno()

    def isatty(self):
        return self.buffer.isatty()

    def write(self, s: str):
        if self.closed:
            raise ValueError("write to closed file")
        if not isinstance(s, str):
            raise TypeError("can't write %s to text stream" %
                            s.__class__.__name__)
        length = len(s)
        haslf = (self._writetranslate or self._line_buffering) and "\n" in s
        if haslf and self._writetranslate and self._writenl != "\n":
            s = s.replace("\n", self._writenl)
        encoder = self._encoder or self._get_encoder()
        # XXX What if we were just reading?
        b = encoder.encode(s)
        self.buffer.write(b)
        if self._line_buffering and (haslf or "\r" in s):
            self.flush()
        self._snapshot = None
        if self._decoder:
            self._decoder.reset()
        return length

    def _get_encoder(self):
        make_encoder = codecs.getincrementalencoder(self._encoding)
        self._encoder = make_encoder(self._errors)
        return self._encoder

    def _get_decoder(self):
        make_decoder = codecs.getincrementaldecoder(self._encoding)
        decoder = make_decoder(self._errors)
        if self._readuniversal:
            decoder = IncrementalNewlineDecoder(decoder, self._readtranslate)
        self._decoder = decoder
        return decoder

    def _read_chunk(self):
        """
        Read and decode the next chunk of data from the BufferedReader.

        The return value is True unless EOF was reached.  The decoded string
        is placed in self._decoded_text (replacing its previous value).
        (The entire input chunk is sent to the decoder, though some of it
        may remain buffered in the decoder, yet to be converted.)
        """

        if self._decoder is None:
            raise ValueError("no decoder")
        if not self._telling:
            # No one should call tell(), so don't bother taking a snapshot.
            input_chunk = self.buffer.read1(self._CHUNK_SIZE)
            eof = not input_chunk
            self._decoded_text = self._decoder.decode(input_chunk, eof)
            self._decoded_text_offset = 0
            return not eof

        # The cookie returned by tell() cannot include the contents of
        # the decoder's buffer, so we need to snapshot a point in the
        # input where the decoder has nothing in its input buffer.

        dec_buffer, dec_flags = self._decoder.getstate()
        # The state tuple returned by getstate() contains the decoder's
        # input buffer and an integer representing any other state.  Thus,
        # there is a valid snapshot point len(decoder_buffer) bytes ago in
        # the input, with the state tuple (b'', decoder_state).

        input_chunk = self.buffer.read1(self._CHUNK_SIZE)
        eof = not input_chunk
        self._decoded_text = self._decoder.decode(input_chunk, eof)
        self._decoded_text_offset = 0

        # At the snapshot point, len(dec_buffer) bytes ago, the next input
        # to be passed to the decoder is dec_buffer + input_chunk.
        self._snapshot = (dec_flags, dec_buffer + input_chunk)
        return not eof

    def _pack_cookie(self, position, dec_flags=0,
                            feed_bytes=0, need_eof=0, skip_chars=0):
        # The meaning of a tell() cookie is: seek to position, set the
        # decoder flags to dec_flags, read feed_bytes bytes, feed them
        # into the decoder with need_eof as the EOF flag, then skip
        # skip_chars characters of the decoded result.  For most simple
        # decoders, this should often just be the position.
        return (position | (dec_flags<<64) | (feed_bytes<<128) |
                (skip_chars<<192) | bool(need_eof)<<256)

    def _unpack_cookie(self, bigint):
        rest, position = divmod(bigint, 1<<64)
        rest, dec_flags = divmod(rest, 1<<64)
        rest, feed_bytes = divmod(rest, 1<<64)
        need_eof, skip_chars = divmod(rest, 1<<64)
        return position, dec_flags, feed_bytes, need_eof, skip_chars

    def tell(self):
        if not self._seekable:
            raise IOError("underlying stream is not seekable")
        if not self._telling:
            raise IOError("telling position disabled by next() call")
        self.flush()
        position = self.buffer.tell()
        decoder = self._decoder
        if decoder is None or self._snapshot is None:
            if self._decoded_text:
                # This should never happen.
                raise AssertionError("pending decoded text")
            return position

        # Skip backward to the snapshot point (see _read_chunk).
        dec_flags, next_input = self._snapshot
        position -= len(next_input)

        # How many decoded characters have been returned since the snapshot?
        skip_chars = self._decoded_text_offset
        if skip_chars == 0:
            # We haven't moved from the snapshot point.
            return self._pack_cookie(position, dec_flags)

        # Walk the decoder forward, one byte at a time, to find the minimum
        # input necessary to give us the decoded characters we need to skip.
        # As we go, look for the "safe point" nearest to the current location
        # (i.e. a point where the decoder has nothing buffered, so we can
        # safely start from there when trying to return to this location).
        saved_state = decoder.getstate()
        try:
            decoder.setstate((b"", dec_flags))
            fed_bytes = 0
            decoded_chars = 0
            need_eof = 0
            last_safe_point = (dec_flags, 0, 0)

            next_byte = bytearray(1)
            for next_byte[0] in next_input:
                decoded = decoder.decode(next_byte)
                fed_bytes += 1
                decoded_chars += len(decoded)
                dec_buffer, dec_flags = decoder.getstate()
                if not dec_buffer and decoded_chars <= skip_chars:
                    # Decoder buffer is empty, so it's safe to start from here.
                    last_safe_point = (dec_flags, fed_bytes, decoded_chars)
                if decoded_chars >= skip_chars:
                    break
            else:
                # We didn't get enough decoded data; signal EOF to get more.
                decoded = decoder.decode(b"", final=True)
                decoded_chars += len(decoded)
                need_eof = 1
                if decoded_chars < skip_chars:
                    raise IOError("can't reconstruct logical file position")

            # Advance the starting position to the last safe point.
            dec_flags, safe_fed_bytes, safe_decoded_chars = last_safe_point
            position += safe_fed_bytes
            fed_bytes -= safe_fed_bytes
            skip_chars -= safe_decoded_chars
            return self._pack_cookie(
                position, dec_flags, fed_bytes, need_eof, skip_chars)
        finally:
            decoder.setstate(saved_state)

    def seek(self, cookie, whence=0):
        if not self._seekable:
            raise IOError("underlying stream is not seekable")
        if whence == 1: # seek relative to current position
            if cookie != 0:
                raise IOError("can't do nonzero cur-relative seeks")
            # Seeking to the current position should attempt to
            # sync the underlying buffer with the current position.
            whence = 0
            cookie = self.tell()
        if whence == 2: # seek relative to end of file
            if cookie != 0:
                raise IOError("can't do nonzero end-relative seeks")
            self.flush()
            position = self.buffer.seek(0, 2)
            self._clear_decoded_text()
            if self._decoder:
                self._decoder.reset()
            return position
        if whence != 0:
            raise ValueError("invalid whence (%r, should be 0, 1 or 2)" %
                             (whence,))
        if cookie < 0:
            raise ValueError("negative seek position %r" % (cookie,))
        self.flush()

        # Seek back to the snapshot point.
        position, dec_flags, feed_bytes, need_eof, skip_chars = \
            self._unpack_cookie(cookie)
        self.buffer.seek(position)
        self._clear_decoded_text()

        if self._decoder or dec_flags or feed_bytes or need_eof:
            # Restore the decoder flags to their values from the snapshot.
            self._decoder = self._decoder or self._get_decoder()
            self._decoder.setstate((b"", dec_flags))
            self._snapshot = (dec_flags, b'')

        if feed_bytes or need_eof:
            # Feed feed_bytes bytes to the decoder.
            input_chunk = self.buffer.read(feed_bytes)
            self._decoded_text = self._decoder.decode(input_chunk, need_eof)
            if len(self._decoded_text) < skip_chars:
                raise IOError("can't restore logical file position")

            # Skip skip_chars of the decoded characters.
            self._decoded_text_offset = skip_chars

            # Restore the snapshot.
            self._snapshot = (dec_flags, input_chunk)
        return cookie

    def _clear_decoded_text(self):
        """Reset the _decoded_text buffer."""
        self._decoded_text = ''
        self._decoded_text_offset = 0
        self._snapshot = None

    def _emit_decoded_text(self, n=None):
        """Advance into the _decoded_text buffer."""
        offset = self._decoded_text_offset
        if n is None:
            text = self._decoded_text[offset:]
        else:
            text = self._decoded_text[offset:offset + n]
        self._decoded_text_offset += len(text)
        return text

    def _unemit_decoded_text(self, n):
        """Rewind the _decoded_text buffer."""
        if self._decoded_text_offset < n:
            raise AssertionError("unemit out of bounds")
        self._decoded_text_offset -= n

    def read(self, n=None):
        if n is None:
            n = -1
        decoder = self._decoder or self._get_decoder()
        if n < 0:
            # Read everything.
            result = (self._emit_decoded_text() +
                      decoder.decode(self.buffer.read(), final=True))
            self._clear_decoded_text()
            return result
        else:
            # Keep reading chunks until we have n characters to return.
            eof = False
            result = self._emit_decoded_text(n)
            while len(result) < n and not eof:
                eof = not self._read_chunk()
                result += self._emit_decoded_text(n - len(result))
            return result

    def __next__(self):
        self._telling = False
        line = self.readline()
        if not line:
            self._snapshot = None
            self._telling = self._seekable
            raise StopIteration
        return line

    def readline(self, limit=None):
        if limit is None:
            limit = -1

        # Grab all the decoded text (we will rewind any extra bits later).
        line = self._emit_decoded_text()

        start = 0
        decoder = self._decoder or self._get_decoder()

        pos = endpos = None
        while True:
            if self._readtranslate:
                # Newlines are already translated, only search for \n
                pos = line.find('\n', start)
                if pos >= 0:
                    endpos = pos + 1
                    break
                else:
                    start = len(line)

            elif self._readuniversal:
                # Universal newline search. Find any of \r, \r\n, \n
                # The decoder ensures that \r\n are not split in two pieces

                # In C we'd look for these in parallel of course.
                nlpos = line.find("\n", start)
                crpos = line.find("\r", start)
                if crpos == -1:
                    if nlpos == -1:
                        # Nothing found
                        start = len(line)
                    else:
                        # Found \n
                        endpos = nlpos + 1
                        break
                elif nlpos == -1:
                    # Found lone \r
                    endpos = crpos + 1
                    break
                elif nlpos < crpos:
                    # Found \n
                    endpos = nlpos + 1
                    break
                elif nlpos == crpos + 1:
                    # Found \r\n
                    endpos = crpos + 2
                    break
                else:
                    # Found \r
                    endpos = crpos + 1
                    break
            else:
                # non-universal
                pos = line.find(self._readnl)
                if pos >= 0:
                    endpos = pos + len(self._readnl)
                    break

            if limit >= 0 and len(line) >= limit:
                endpos = limit  # reached length limit
                break

            # No line ending seen yet - get more data
            more_line = ''
            while self._read_chunk():
                if self._decoded_text:
                    break
            if self._decoded_text:
                line += self._emit_decoded_text()
            else:
                # end of file
                self._clear_decoded_text()
                return line

        if limit >= 0 and endpos > limit:
            endpos = limit  # don't exceed limit

        # Rewind _decoded_text to just after the line ending we found.
        self._unemit_decoded_text(len(line) - endpos)
        return line[:endpos]

    @property
    def newlines(self):
        return self._decoder.newlines if self._decoder else None

class StringIO(TextIOWrapper):

    # XXX This is really slow, but fully functional

    def __init__(self, initial_value="", encoding="utf-8",
                 errors="strict", newline="\n"):
        super(StringIO, self).__init__(BytesIO(),
                                       encoding=encoding,
                                       errors=errors,
                                       newline=newline)
        if initial_value:
            if not isinstance(initial_value, str):
                initial_value = str(initial_value)
            self.write(initial_value)
            self.seek(0)

    def getvalue(self):
        self.flush()
        return self.buffer.getvalue().decode(self._encoding, self._errors)
