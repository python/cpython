"Provides shared memory for direct access across processes."


__all__ = [ 'SharedMemory', 'PosixSharedMemory', 'WindowsNamedSharedMemory',
            'ShareableList', 'shareable_wrap',
            'SharedMemoryServer', 'SharedMemoryManager', 'SharedMemoryTracker' ]


from functools import reduce
import mmap
from .managers import dispatch, BaseManager, Server, State, ProcessError, \
                      BarrierProxy, AcquirerProxy, ConditionProxy, EventProxy
from . import util
import os
import struct
import sys
import threading
import secrets
try:
    import  _posixshmem
    from os import O_CREAT, O_EXCL, O_TRUNC
except ImportError as ie:
    O_CREAT, O_EXCL, O_TRUNC = 64, 128, 512

O_CREX = O_CREAT | O_EXCL

if os.name == "nt":
    import ctypes
    from ctypes import wintypes

    kernel32 = ctypes.windll.kernel32

    class MEMORY_BASIC_INFORMATION(ctypes.Structure):
        _fields_ = (
            ('BaseAddress',       ctypes.c_void_p),
            ('AllocationBase',    ctypes.c_void_p),
            ('AllocationProtect', wintypes.DWORD),
            ('RegionSize',        ctypes.c_size_t),
            ('State',             wintypes.DWORD),
            ('Protect',           wintypes.DWORD),
            ('Type',              wintypes.DWORD)
        )

    PMEMORY_BASIC_INFORMATION = ctypes.POINTER(MEMORY_BASIC_INFORMATION)
    FILE_MAP_READ = 0x0004

    def _errcheck_bool(result, func, args):
        if not result:
            raise ctypes.WinError(ctypes.get_last_error())
        return args

    kernel32.VirtualQuery.errcheck = _errcheck_bool
    kernel32.VirtualQuery.restype = ctypes.c_size_t
    kernel32.VirtualQuery.argtypes = (
        wintypes.LPCVOID,
        PMEMORY_BASIC_INFORMATION,
        ctypes.c_size_t
    )

    kernel32.OpenFileMappingW.errcheck = _errcheck_bool
    kernel32.OpenFileMappingW.restype = wintypes.HANDLE
    kernel32.OpenFileMappingW.argtypes = (
        wintypes.DWORD,
        wintypes.BOOL,
        wintypes.LPCWSTR
    )

    kernel32.MapViewOfFile.errcheck = _errcheck_bool
    kernel32.MapViewOfFile.restype = wintypes.LPVOID
    kernel32.MapViewOfFile.argtypes = (
        wintypes.HANDLE,
        wintypes.DWORD,
        wintypes.DWORD,
        wintypes.DWORD,
        ctypes.c_size_t
    )

    kernel32.CloseHandle.errcheck = _errcheck_bool
    kernel32.CloseHandle.argtypes = (wintypes.HANDLE,)


class WindowsNamedSharedMemory:

    def __init__(self, name, flags=None, mode=384, size=0, read_only=False):
        if name is None:
            name = _make_filename()

        if size == 0:
            # Attempt to dynamically determine the existing named shared
            # memory block's size which is likely a multiple of mmap.PAGESIZE.
            try:
                h_map = kernel32.OpenFileMappingW(FILE_MAP_READ, False, name)
            except OSError:
                raise FileNotFoundError(name)
            try:
                p_buf = kernel32.MapViewOfFile(h_map, FILE_MAP_READ, 0, 0, 0)
            finally:
                kernel32.CloseHandle(h_map)
            mbi = MEMORY_BASIC_INFORMATION()
            kernel32.VirtualQuery(p_buf, ctypes.byref(mbi), mmap.PAGESIZE)
            size = mbi.RegionSize

        if flags == O_CREX:
            # Verify no named shared memory block already exists by this name.
            try:
                h_map = kernel32.OpenFileMappingW(FILE_MAP_READ, False, name)
                kernel32.CloseHandle(h_map)
                name_collision = True
            except OSError as ose:
                name_collision = False
            if name_collision:
                raise FileExistsError(name)

        self._mmap = mmap.mmap(-1, size, tagname=name)
        self.buf = memoryview(self._mmap)
        self.name = name
        self.mode = mode
        self.size = size

    def __repr__(self):
        return f'{self.__class__.__name__}({self.name!r}, size={self.size})'

    def close(self):
        self.buf.release()
        self._mmap.close()

    def unlink(self):
        """Windows ensures that destruction of the last reference to this
        named shared memory block will result in the release of this memory."""
        pass


# FreeBSD (and perhaps other BSDs) limit names to 14 characters.
_SHM_SAFE_NAME_LENGTH = 14

# shared object name prefix
if os.name == "nt":
    _SHM_NAME_PREFIX = 'wnsm_'
else:
    _SHM_NAME_PREFIX = '/psm_'


def _make_filename():
    """Create a random filename for the shared memory object.
    """
    # number of random bytes to use for name
    nbytes = (_SHM_SAFE_NAME_LENGTH - len(_SHM_NAME_PREFIX)) // 2
    assert nbytes >= 2, '_SHM_NAME_PREFIX too long'
    name = _SHM_NAME_PREFIX + secrets.token_hex(nbytes)
    assert len(name) <= _SHM_SAFE_NAME_LENGTH
    return name


class PosixSharedMemory:

    # defaults so close() and unlink() can run without errors
    fd = -1
    name = None
    _mmap = None
    buf = None

    def __init__(self, name, flags=None, mode=384, size=0, read_only=False):
        if name and (flags is None):
            flags = 0
        else:
            flags = O_CREX if flags is None else flags
        if flags & O_EXCL and not flags & O_CREAT:
            raise ValueError("O_EXCL must be combined with O_CREAT")
        if name is None and not flags & O_EXCL:
            raise ValueError("'name' can only be None if O_EXCL is set")
        flags |= os.O_RDONLY if read_only else os.O_RDWR
        self.flags = flags
        self.mode = mode
        if not size >= 0:
            raise ValueError("'size' must be a positive integer")
        self.size = size
        if name is None:
            self._open_retry()
        else:
            self.name = name
            self.fd = _posixshmem.shm_open(name, flags, mode=mode)
        if self.size:
            try:
                os.ftruncate(self.fd, self.size)
            except OSError:
                self.unlink()
                raise
        self._mmap = mmap.mmap(self.fd, self.size)
        self.buf = memoryview(self._mmap)

    def _open_retry(self):
        # generate a random name, open, retry if it exists
        while True:
            name = _make_filename()
            try:
                self.fd = _posixshmem.shm_open(name, self.flags,
                                               mode=self.mode)
            except FileExistsError:
                continue
            self.name = name
            break

    def __repr__(self):
        return f'{self.__class__.__name__}({self.name!r}, size={self.size})'

    def unlink(self):
        if self.name:
            _posixshmem.shm_unlink(self.name)

    def close(self):
        if self.buf is not None:
            self.buf.release()
            self.buf = None
        if self._mmap is not None:
            self._mmap.close()
            self._mmap = None
        if self.fd >= 0:
            os.close(self.fd)
            self.fd = -1

    def __del__(self):
        try:
            self.close()
        except OSError:
            pass


class SharedMemory:

    def __new__(cls, *args, **kwargs):
        if os.name == 'nt':
            cls = WindowsNamedSharedMemory
        else:
            cls = PosixSharedMemory
        return cls(*args, **kwargs)


def shareable_wrap(
    existing_obj=None,
    shmem_name=None,
    cls=None,
    shape=(0,),
    strides=None,
    dtype=None,
    format=None,
    **kwargs
):
    """Provides a fast, convenient way to encapsulate objects that support
    the buffer protocol as both producer and consumer, duplicating the
    original object's data in shared memory and returning a new wrapped
    object that when serialized via pickle does not serialize its data.

    The function has been written in a general way to potentially work with
    any object supporting the buffer protocol as producer and consumer.  It
    is known to work well with NumPy ndarrays.  Among the Python core data
    types and standard library, there are a number of objects supporting
    the buffer protocol as a producer but not as a consumer.

    Without an example of a producer+consumer of the buffer protocol in
    the Python core to demonstrate the use of this function, this function
    should likely be removed from this module and potentially be made
    available instead via a pip-installable package."""

    augmented_kwargs = dict(kwargs)
    extras = dict(shape=shape, strides=strides, dtype=dtype, format=format)
    for key, value in extras.items():
        if value is not None:
            augmented_kwargs[key] = value

    if existing_obj is not None:
        existing_type = getattr(
            existing_obj,
            "_proxied_type",
            type(existing_obj)
        )

        #agg = existing_obj.itemsize
        #size = [ agg := i * agg for i in existing_obj.shape ][-1]
        # TODO: replace use of reduce below with above 2 lines once available
        size = reduce(
            lambda x, y: x * y,
            existing_obj.shape,
            existing_obj.itemsize
        )

    else:
        assert shmem_name is not None
        existing_type = cls
        size = 1

    shm = SharedMemory(shmem_name, size=size)

    class CustomShareableProxy(existing_type):

        def __init__(self, *args, buffer=None, **kwargs):
            # If copy method called, prevent recursion from replacing _shm.
            if not hasattr(self, "_shm"):
                self._shm = shm
                self._proxied_type = existing_type
            else:
                # _proxied_type only used in pickling.
                assert hasattr(self, "_proxied_type")
            try:
                existing_type.__init__(self, *args, **kwargs)
            except Exception:
                pass

        def __repr__(self):
            if not hasattr(self, "_shm"):
                return existing_type.__repr__(self)
            formatted_pairs = (
                "%s=%r" % kv for kv in self._build_state(self).items()
            )
            return f"{self.__class__.__name__}({', '.join(formatted_pairs)})"

        #def __getstate__(self):
        #    if not hasattr(self, "_shm"):
        #        return existing_type.__getstate__(self)
        #    state = self._build_state(self)
        #    return state

        #def __setstate__(self, state):
        #    self.__init__(**state)

        def __reduce__(self):
            return (
                shareable_wrap,
                (
                    None,
                    self._shm.name,
                    self._proxied_type,
                    self.shape,
                    self.strides,
                    self.dtype.str if hasattr(self, "dtype") else None,
                    getattr(self, "format", None),
                ),
            )

        def copy(self):
            dupe = existing_type.copy(self)
            if not hasattr(dupe, "_shm"):
                dupe = shareable_wrap(dupe)
            return dupe

        @staticmethod
        def _build_state(existing_obj, generics_only=False):
            state = {
                "shape": existing_obj.shape,
                "strides": existing_obj.strides,
            }
            try:
                state["dtype"] = existing_obj.dtype
            except AttributeError:
                try:
                    state["format"] = existing_obj.format
                except AttributeError:
                    pass
            if not generics_only:
                try:
                    state["shmem_name"] = existing_obj._shm.name
                    state["cls"] = existing_type
                except AttributeError:
                    pass
            return state

    proxy_type = type(
        f"{existing_type.__name__}Shareable",
        CustomShareableProxy.__bases__,
        dict(CustomShareableProxy.__dict__),
    )

    if existing_obj is not None:
        try:
            proxy_obj = proxy_type(
                buffer=shm.buf,
                **proxy_type._build_state(existing_obj)
            )
        except Exception:
            proxy_obj = proxy_type(
                buffer=shm.buf,
                **proxy_type._build_state(existing_obj, True)
            )

        mveo = memoryview(existing_obj)
        proxy_obj._shm.buf[:mveo.nbytes] = mveo.tobytes()

    else:
        proxy_obj = proxy_type(buffer=shm.buf, **augmented_kwargs)

    return proxy_obj


encoding = "utf8"

class ShareableList:
    """Pattern for a mutable list-like object shareable via a shared
    memory block.  It differs from the built-in list type in that these
    lists can not change their overall length (i.e. no append, insert,
    etc.)

    Because values are packed into a memoryview as bytes, the struct
    packing format for any storable value must require no more than 8
    characters to describe its format."""

    _types_mapping = {
        int: "q",
        float: "d",
        bool: "xxxxxxx?",
        str: "%ds",
        bytes: "%ds",
        None.__class__: "xxxxxx?x",
    }
    _alignment = 8
    _back_transforms_mapping = {
        0: lambda value: value,                   # int, float, bool
        1: lambda value: value.rstrip(b'\x00').decode(encoding),  # str
        2: lambda value: value.rstrip(b'\x00'),   # bytes
        3: lambda _value: None,                   # None
    }

    @staticmethod
    def _extract_recreation_code(value):
        """Used in concert with _back_transforms_mapping to convert values
        into the appropriate Python objects when retrieving them from
        the list as well as when storing them."""
        if not isinstance(value, (str, bytes, None.__class__)):
            return 0
        elif isinstance(value, str):
            return 1
        elif isinstance(value, bytes):
            return 2
        else:
            return 3  # NoneType

    def __init__(self, sequence=None, *, name=None):
        if sequence is not None:
            _formats = [
                self._types_mapping[type(item)]
                    if not isinstance(item, (str, bytes))
                    else self._types_mapping[type(item)] % (
                        self._alignment * (len(item) // self._alignment + 1),
                    )
                for item in sequence
            ]
            self._list_len = len(_formats)
            assert sum(len(fmt) <= 8 for fmt in _formats) == self._list_len
            self._allocated_bytes = tuple(
                    self._alignment if fmt[-1] != "s" else int(fmt[:-1])
                    for fmt in _formats
            )
            _recreation_codes = [
                self._extract_recreation_code(item) for item in sequence
            ]
            requested_size = struct.calcsize(
                "q" + self._format_size_metainfo +
                "".join(_formats) +
                self._format_packing_metainfo +
                self._format_back_transform_codes
            )

        else:
            requested_size = 8  # Some platforms require > 0.

        if name is not None and sequence is None:
            self.shm = SharedMemory(name)
        else:
            self.shm = SharedMemory(name, flags=O_CREX, size=requested_size)

        if sequence is not None:
            _enc = encoding
            struct.pack_into(
                "q" + self._format_size_metainfo,
                self.shm.buf,
                0,
                self._list_len,
                *(self._allocated_bytes)
            )
            struct.pack_into(
                "".join(_formats),
                self.shm.buf,
                self._offset_data_start,
                *(v.encode(_enc) if isinstance(v, str) else v for v in sequence)
            )
            struct.pack_into(
                self._format_packing_metainfo,
                self.shm.buf,
                self._offset_packing_formats,
                *(v.encode(_enc) for v in _formats)
            )
            struct.pack_into(
                self._format_back_transform_codes,
                self.shm.buf,
                self._offset_back_transform_codes,
                *(_recreation_codes)
            )

        else:
            self._list_len = len(self)  # Obtains size from offset 0 in buffer.
            self._allocated_bytes = struct.unpack_from(
                self._format_size_metainfo,
                self.shm.buf,
                1 * 8
            )

    def _get_packing_format(self, position):
        "Gets the packing format for a single value stored in the list."
        position = position if position >= 0 else position + self._list_len
        if (position >= self._list_len) or (self._list_len < 0):
            raise IndexError("Requested position out of range.")

        v = struct.unpack_from(
            "8s",
            self.shm.buf,
            self._offset_packing_formats + position * 8
        )[0]
        fmt = v.rstrip(b'\x00')
        fmt_as_str = fmt.decode(encoding)

        return fmt_as_str

    def _get_back_transform(self, position):
        "Gets the back transformation function for a single value."

        position = position if position >= 0 else position + self._list_len
        if (position >= self._list_len) or (self._list_len < 0):
            raise IndexError("Requested position out of range.")

        transform_code = struct.unpack_from(
            "b",
            self.shm.buf,
            self._offset_back_transform_codes + position
        )[0]
        transform_function = self._back_transforms_mapping[transform_code]

        return transform_function

    def _set_packing_format_and_transform(self, position, fmt_as_str, value):
        """Sets the packing format and back transformation code for a
        single value in the list at the specified position."""

        position = position if position >= 0 else position + self._list_len
        if (position >= self._list_len) or (self._list_len < 0):
            raise IndexError("Requested position out of range.")

        struct.pack_into(
            "8s",
            self.shm.buf,
            self._offset_packing_formats + position * 8,
            fmt_as_str.encode(encoding)
        )

        transform_code = self._extract_recreation_code(value)
        struct.pack_into(
            "b",
            self.shm.buf,
            self._offset_back_transform_codes + position,
            transform_code
        )

    def __getitem__(self, position):
        try:
            offset = self._offset_data_start \
                     + sum(self._allocated_bytes[:position])
            (v,) = struct.unpack_from(
                self._get_packing_format(position),
                self.shm.buf,
                offset
            )
        except IndexError:
            raise IndexError("index out of range")

        back_transform = self._get_back_transform(position)
        v = back_transform(v)

        return v

    def __setitem__(self, position, value):
        try:
            offset = self._offset_data_start \
                     + sum(self._allocated_bytes[:position])
            current_format = self._get_packing_format(position)
        except IndexError:
            raise IndexError("assignment index out of range")

        if not isinstance(value, (str, bytes)):
            new_format = self._types_mapping[type(value)]
        else:
            if len(value) > self._allocated_bytes[position]:
                raise ValueError("exceeds available storage for existing str")
            if current_format[-1] == "s":
                new_format = current_format
            else:
                new_format = self._types_mapping[str] % (
                    self._allocated_bytes[position],
                )

        self._set_packing_format_and_transform(
            position,
            new_format,
            value
        )
        value = value.encode(encoding) if isinstance(value, str) else value
        struct.pack_into(new_format, self.shm.buf, offset, value)

    def __len__(self):
        return struct.unpack_from("q", self.shm.buf, 0)[0]

    def __repr__(self):
        return f'{self.__class__.__name__}({list(self)}, name={self.shm.name!r})'

    @property
    def format(self):
        "The struct packing format used by all currently stored values."
        return "".join(
            self._get_packing_format(i) for i in range(self._list_len)
        )

    @property
    def _format_size_metainfo(self):
        "The struct packing format used for metainfo on storage sizes."
        return f"{self._list_len}q"

    @property
    def _format_packing_metainfo(self):
        "The struct packing format used for the values' packing formats."
        return "8s" * self._list_len

    @property
    def _format_back_transform_codes(self):
        "The struct packing format used for the values' back transforms."
        return "b" * self._list_len

    @property
    def _offset_data_start(self):
        return (self._list_len + 1) * 8  # 8 bytes per "q"

    @property
    def _offset_packing_formats(self):
        return self._offset_data_start + sum(self._allocated_bytes)

    @property
    def _offset_back_transform_codes(self):
        return self._offset_packing_formats + self._list_len * 8

    def copy(self, *, name=None):
        "L.copy() -> ShareableList -- a shallow copy of L."

        if name is None:
            return self.__class__(self)
        else:
            return self.__class__(self, name=name)

    def count(self, value):
        "L.count(value) -> integer -- return number of occurrences of value."

        return sum(value == entry for entry in self)

    def index(self, value):
        """L.index(value) -> integer -- return first index of value.
        Raises ValueError if the value is not present."""

        for position, entry in enumerate(self):
            if value == entry:
                return position
        else:
            raise ValueError(f"{value!r} not in this container")


class SharedMemoryTracker:
    "Manages one or more shared memory segments."

    def __init__(self, name, segment_names=[]):
        self.shared_memory_context_name = name
        self.segment_names = segment_names

    def register_segment(self, segment_name):
        "Adds the supplied shared memory block name to tracker."
        util.debug(f"Registering segment {segment_name!r} in pid {os.getpid()}")
        self.segment_names.append(segment_name)

    def destroy_segment(self, segment_name):
        """Calls unlink() on the shared memory block with the supplied name
        and removes it from the list of blocks being tracked."""
        util.debug(f"Destroying segment {segment_name!r} in pid {os.getpid()}")
        self.segment_names.remove(segment_name)
        segment = SharedMemory(segment_name)
        segment.close()
        segment.unlink()

    def unlink(self):
        "Calls destroy_segment() on all currently tracked shared memory blocks."
        for segment_name in self.segment_names[:]:
            self.destroy_segment(segment_name)

    def __del__(self):
        util.debug(f"Called {self.__class__.__name__}.__del__ in {os.getpid()}")
        self.unlink()

    def __getstate__(self):
        return (self.shared_memory_context_name, self.segment_names)

    def __setstate__(self, state):
        self.__init__(*state)

    def wrap(self, obj_exposing_buffer_protocol):
        wrapped_obj = shareable_wrap(obj_exposing_buffer_protocol)
        self.register_segment(wrapped_obj._shm.name)
        return wrapped_obj


class SharedMemoryServer(Server):

    public = Server.public + \
             ['track_segment', 'release_segment', 'list_segments']

    def __init__(self, *args, **kwargs):
        Server.__init__(self, *args, **kwargs)
        self.shared_memory_context = \
            SharedMemoryTracker(f"shmm_{self.address}_{os.getpid()}")
        util.debug(f"SharedMemoryServer started by pid {os.getpid()}")

    def create(self, c, typeid, *args, **kwargs):
        """Create a new distributed-shared object (not backed by a shared
        memory block) and return its id to be used in a Proxy Object."""
        # Unless set up as a shared proxy, don't make shared_memory_context
        # a standard part of kwargs.  This makes things easier for supplying
        # simple functions.
        if hasattr(self.registry[typeid][-1], "_shared_memory_proxy"):
            kwargs['shared_memory_context'] = self.shared_memory_context
        return Server.create(self, c, typeid, *args, **kwargs)

    def shutdown(self, c):
        "Call unlink() on all tracked shared memory then terminate the Server."
        self.shared_memory_context.unlink()
        return Server.shutdown(self, c)

    def track_segment(self, c, segment_name):
        "Adds the supplied shared memory block name to Server's tracker."
        self.shared_memory_context.register_segment(segment_name)

    def release_segment(self, c, segment_name):
        """Calls unlink() on the shared memory block with the supplied name
        and removes it from the tracker instance inside the Server."""
        self.shared_memory_context.destroy_segment(segment_name)

    def list_segments(self, c):
        """Returns a list of names of shared memory blocks that the Server
        is currently tracking."""
        return self.shared_memory_context.segment_names


class SharedMemoryManager(BaseManager):
    """Like SyncManager but uses SharedMemoryServer instead of Server.

    It provides methods for creating and returning SharedMemory instances
    and for creating a list-like object (ShareableList) backed by shared
    memory.  It also provides methods that create and return Proxy Objects
    that support synchronization across processes (i.e. multi-process-safe
    locks and semaphores).
    """

    _Server = SharedMemoryServer

    def __init__(self, *args, **kwargs):
        BaseManager.__init__(self, *args, **kwargs)
        util.debug(f"{self.__class__.__name__} created by pid {os.getpid()}")

    def __del__(self):
        util.debug(f"{self.__class__.__name__} told die by pid {os.getpid()}")
        pass

    def get_server(self):
        'Better than monkeypatching for now; merge into Server ultimately'
        if self._state.value != State.INITIAL:
            if self._state.value == State.STARTED:
                raise ProcessError("Already started SharedMemoryServer")
            elif self._state.value == State.SHUTDOWN:
                raise ProcessError("SharedMemoryManager has shut down")
            else:
                raise ProcessError(
                    "Unknown state {!r}".format(self._state.value))
        return self._Server(self._registry, self._address,
                            self._authkey, self._serializer)

    def SharedMemory(self, size):
        """Returns a new SharedMemory instance with the specified size in
        bytes, to be tracked by the manager."""
        with self._Client(self._address, authkey=self._authkey) as conn:
            sms = SharedMemory(None, flags=O_CREX, size=size)
            try:
                dispatch(conn, None, 'track_segment', (sms.name,))
            except BaseException as e:
                sms.unlink()
                raise e
        return sms

    def ShareableList(self, sequence):
        """Returns a new ShareableList instance populated with the values
        from the input sequence, to be tracked by the manager."""
        with self._Client(self._address, authkey=self._authkey) as conn:
            sl = ShareableList(sequence)
            try:
                dispatch(conn, None, 'track_segment', (sl.shm.name,))
            except BaseException as e:
                sl.shm.unlink()
                raise e
        return sl

SharedMemoryManager.register('Barrier', threading.Barrier, BarrierProxy)
SharedMemoryManager.register(
    'BoundedSemaphore',
    threading.BoundedSemaphore,
    AcquirerProxy
)
SharedMemoryManager.register('Condition', threading.Condition, ConditionProxy)
SharedMemoryManager.register('Event', threading.Event, EventProxy)
SharedMemoryManager.register('Lock', threading.Lock, AcquirerProxy)
SharedMemoryManager.register('RLock', threading.RLock, AcquirerProxy)
SharedMemoryManager.register('Semaphore', threading.Semaphore, AcquirerProxy)
