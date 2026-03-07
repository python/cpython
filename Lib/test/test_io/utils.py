import array
import threading
import time
import unittest

import io  # C implementation of io
import _pyio as pyio # Python implementation of io


try:
    import ctypes
except ImportError:
    def byteslike(*pos, **kw):
        return array.array("b", bytes(*pos, **kw))
else:
    class EmptyStruct(ctypes.Structure):
        pass

    def byteslike(*pos, **kw):
        """Create a bytes-like object having no string or sequence methods"""
        data = bytes(*pos, **kw)
        obj = EmptyStruct()
        ctypes.resize(obj, len(data))
        memoryview(obj).cast("B")[:] = data
        return obj


class MockRawIOWithoutRead:
    """A RawIO implementation without read(), so as to exercise the default
    RawIO.read() which calls readinto()."""

    def __init__(self, read_stack=()):
        self._read_stack = list(read_stack)
        self._write_stack = []
        self._reads = 0
        self._extraneous_reads = 0

    def write(self, b):
        self._write_stack.append(bytes(b))
        return len(b)

    def writable(self):
        return True

    def fileno(self):
        return 42

    def readable(self):
        return True

    def seekable(self):
        return True

    def seek(self, pos, whence):
        return 0   # wrong but we gotta return something

    def tell(self):
        return 0   # same comment as above

    def readinto(self, buf):
        self._reads += 1
        max_len = len(buf)
        try:
            data = self._read_stack[0]
        except IndexError:
            self._extraneous_reads += 1
            return 0
        if data is None:
            del self._read_stack[0]
            return None
        n = len(data)
        if len(data) <= max_len:
            del self._read_stack[0]
            buf[:n] = data
            return n
        else:
            buf[:] = data[:max_len]
            self._read_stack[0] = data[max_len:]
            return max_len

    def truncate(self, pos=None):
        return pos

class CMockRawIOWithoutRead(MockRawIOWithoutRead, io.RawIOBase):
    pass

class PyMockRawIOWithoutRead(MockRawIOWithoutRead, pyio.RawIOBase):
    pass


class MockRawIO(MockRawIOWithoutRead):

    def read(self, n=None):
        self._reads += 1
        try:
            return self._read_stack.pop(0)
        except:
            self._extraneous_reads += 1
            return b""

class CMockRawIO(MockRawIO, io.RawIOBase):
    pass

class PyMockRawIO(MockRawIO, pyio.RawIOBase):
    pass


class MisbehavedRawIO(MockRawIO):
    def write(self, b):
        return super().write(b) * 2

    def read(self, n=None):
        return super().read(n) * 2

    def seek(self, pos, whence):
        return -123

    def tell(self):
        return -456

    def readinto(self, buf):
        super().readinto(buf)
        return len(buf) * 5

class CMisbehavedRawIO(MisbehavedRawIO, io.RawIOBase):
    pass

class PyMisbehavedRawIO(MisbehavedRawIO, pyio.RawIOBase):
    pass


class SlowFlushRawIO(MockRawIO):
    def __init__(self):
        super().__init__()
        self.in_flush = threading.Event()

    def flush(self):
        self.in_flush.set()
        time.sleep(0.25)

class CSlowFlushRawIO(SlowFlushRawIO, io.RawIOBase):
    pass

class PySlowFlushRawIO(SlowFlushRawIO, pyio.RawIOBase):
    pass


class CloseFailureIO(MockRawIO):
    closed = 0

    def close(self):
        if not self.closed:
            self.closed = 1
            raise OSError

class CCloseFailureIO(CloseFailureIO, io.RawIOBase):
    pass

class PyCloseFailureIO(CloseFailureIO, pyio.RawIOBase):
    pass


class MockFileIO:

    def __init__(self, data):
        self.read_history = []
        super().__init__(data)

    def read(self, n=None):
        res = super().read(n)
        self.read_history.append(None if res is None else len(res))
        return res

    def readinto(self, b):
        res = super().readinto(b)
        self.read_history.append(res)
        return res

class CMockFileIO(MockFileIO, io.BytesIO):
    pass

class PyMockFileIO(MockFileIO, pyio.BytesIO):
    pass


class MockUnseekableIO:
    def seekable(self):
        return False

    def seek(self, *args):
        raise self.UnsupportedOperation("not seekable")

    def tell(self, *args):
        raise self.UnsupportedOperation("not seekable")

    def truncate(self, *args):
        raise self.UnsupportedOperation("not seekable")

class CMockUnseekableIO(MockUnseekableIO, io.BytesIO):
    UnsupportedOperation = io.UnsupportedOperation

class PyMockUnseekableIO(MockUnseekableIO, pyio.BytesIO):
    UnsupportedOperation = pyio.UnsupportedOperation


class MockCharPseudoDevFileIO(MockFileIO):
    # GH-95782
    # ftruncate() does not work on these special files (and CPython then raises
    # appropriate exceptions), so truncate() does not have to be accounted for
    # here.
    def __init__(self, data):
        super().__init__(data)

    def seek(self, *args):
        return 0

    def tell(self, *args):
        return 0

class CMockCharPseudoDevFileIO(MockCharPseudoDevFileIO, io.BytesIO):
    pass

class PyMockCharPseudoDevFileIO(MockCharPseudoDevFileIO, pyio.BytesIO):
    pass


class MockNonBlockWriterIO:

    def __init__(self):
        self._write_stack = []
        self._blocker_char = None

    def pop_written(self):
        s = b"".join(self._write_stack)
        self._write_stack[:] = []
        return s

    def block_on(self, char):
        """Block when a given char is encountered."""
        self._blocker_char = char

    def readable(self):
        return True

    def seekable(self):
        return True

    def seek(self, pos, whence=0):
        # naive implementation, enough for tests
        return 0

    def writable(self):
        return True

    def write(self, b):
        b = bytes(b)
        n = -1
        if self._blocker_char:
            try:
                n = b.index(self._blocker_char)
            except ValueError:
                pass
            else:
                if n > 0:
                    # write data up to the first blocker
                    self._write_stack.append(b[:n])
                    return n
                else:
                    # cancel blocker and indicate would block
                    self._blocker_char = None
                    return None
        self._write_stack.append(b)
        return len(b)

class CMockNonBlockWriterIO(MockNonBlockWriterIO, io.RawIOBase):
    BlockingIOError = io.BlockingIOError

class PyMockNonBlockWriterIO(MockNonBlockWriterIO, pyio.RawIOBase):
    BlockingIOError = pyio.BlockingIOError


# Build classes which point to all the right mocks per io implementation
class CTestCase(unittest.TestCase):
    io = io
    is_C = True

    MockRawIO = CMockRawIO
    MisbehavedRawIO = CMisbehavedRawIO
    MockFileIO = CMockFileIO
    CloseFailureIO = CCloseFailureIO
    MockNonBlockWriterIO = CMockNonBlockWriterIO
    MockUnseekableIO = CMockUnseekableIO
    MockRawIOWithoutRead = CMockRawIOWithoutRead
    SlowFlushRawIO = CSlowFlushRawIO
    MockCharPseudoDevFileIO = CMockCharPseudoDevFileIO

    # Use the class as a proxy to the io module members.
    def __getattr__(self, name):
        return getattr(io, name)


class PyTestCase(unittest.TestCase):
    io = pyio
    is_C = False

    MockRawIO = PyMockRawIO
    MisbehavedRawIO = PyMisbehavedRawIO
    MockFileIO = PyMockFileIO
    CloseFailureIO = PyCloseFailureIO
    MockNonBlockWriterIO = PyMockNonBlockWriterIO
    MockUnseekableIO = PyMockUnseekableIO
    MockRawIOWithoutRead = PyMockRawIOWithoutRead
    SlowFlushRawIO = PySlowFlushRawIO
    MockCharPseudoDevFileIO = PyMockCharPseudoDevFileIO

    # Use the class as a proxy to the _pyio module members.
    def __getattr__(self, name):
        return getattr(pyio, name)
