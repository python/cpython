# Copyright 2006 Google, Inc. All Rights Reserved.
# Licensed to PSF under a Contributor Agreement.

"""New I/O library.

See PEP XXX; for now: http://docs.google.com/Doc?id=dfksfvqd_1cn5g5m
"""

__author__ = "Guido van Rossum <guido@python.org>"

__all__ = ["open", "RawIOBase", "FileIO", "SocketIO", "BytesIO"]

import os

def open(filename, mode="r", buffering=None, *, encoding=None):
    """Replacement for the built-in open function, with encoding parameter."""
    assert isinstance(filename, str)
    assert isinstance(mode, str)
    assert buffering is None or isinstance(buffering, int)
    assert encoding is None or isinstance(encoding, str)
    modes = set(mode)
    if modes - set("arwb+t") or len(mode) > len(modes):
        raise ValueError("invalid mode: %r" % mode)
    reading = "r" in modes
    writing = "w" in modes or "a" in modes
    binary = "b" in modes
    appending = "a" in modes
    updating = "+" in modes
    text = "t" in modes or not binary
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
        buffering = 8*1024  # International standard buffer size
    if buffering < 0:
        raise ValueError("invalid buffering size")
    if buffering == 0:
        if binary:
            return raw
        raise ValueError("can't have unbuffered text I/O")
    if updating:
        buffer = BufferedRandom(raw, buffering)
    elif writing:
        buffer = BufferedWriter(raw, buffering)
    else:
        assert reading
        buffer = BufferedReader(raw, buffering)
    if binary:
        return buffer
    assert text
    textio = TextIOWrapper(buffer)  # Universal newlines default to on
    return textio


class RawIOBase:

    """Base class for raw binary I/O."""

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

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()

    def fileno(self):
        return self._fd

    
class SocketIO(RawIOBase):

    """Raw I/O implementation for stream sockets."""

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


class BytesIO(RawIOBase):

    """Raw I/O implementation for bytes, like StringIO."""

    def __init__(self, inital_bytes=None):
        self._buffer = b""
        self._pos = 0
        if inital_bytes is not None:
            self._buffer += inital_bytes

    def getvalue(self):
        return self._buffer

    def read(self, n):
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
