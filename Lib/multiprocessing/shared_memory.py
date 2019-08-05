"Provides shared memory for direct access across processes."


__all__ = [ 'SharedMemory', 'PosixSharedMemory', 'WindowsNamedSharedMemory',
            'ShareableList', 'shareable_wrap',
            'SharedMemoryServer', 'SharedMemoryManager', 'SharedMemoryTracker' ]


from functools import reduce
import mmap
from .managers import DictProxy, SyncManager, Server
from . import util
import os
import random
import struct
import sys
try:
    from _posixshmem import _PosixSharedMemory, Error, ExistentialError, O_CREX
except ImportError as ie:
    if os.name != "nt":
        # On Windows, posixshmem is not required to be available.
        raise ie
    else:
        _PosixSharedMemory = object
        class ExistentialError(BaseException): pass
        class Error(BaseException): pass
        O_CREX = -1


class WindowsNamedSharedMemory:

    def __init__(self, name, flags=None, mode=None, size=None, read_only=False):
        if name is None:
            name = f'wnsm_{os.getpid()}_{random.randrange(100000)}'

        self._mmap = mmap.mmap(-1, size, tagname=name)
        self.buf = memoryview(self._mmap)
        self.name = name
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


class PosixSharedMemory(_PosixSharedMemory):

    def __init__(self, name, flags=None, mode=None, size=None, read_only=False):
        if name and (flags is None):
            _PosixSharedMemory.__init__(self, name)
        else:
            if name is None:
                name = f'psm_{os.getpid()}_{random.randrange(100000)}'
            _PosixSharedMemory.__init__(self, name, flags=O_CREX, size=size)

        self._mmap = mmap.mmap(self.fd, self.size)
        self.buf = memoryview(self._mmap)

    def __repr__(self):
        return f'{self.__class__.__name__}({self.name!r}, size={self.size})'

    def close(self):
        self.buf.release()
        self._mmap.close()
        self.close_fd()


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
            except:
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

    # TODO: Adjust for discovered word size of machine.
    types_mapping = {
        int: "q",
        float: "d",
        bool: "xxxxxxx?",
        str: "%ds",
        bytes: "%ds",
        None.__class__: "xxxxxx?x",
    }
    alignment = 8
    back_transform_codes = {
        0: lambda value: value,                   # int, float, bool
        1: lambda value: value.rstrip(b'\x00').decode(encoding),  # str
        2: lambda value: value.rstrip(b'\x00'),   # bytes
        3: lambda _value: None,                   # None
    }

    @staticmethod
    def _extract_recreation_code(value):
        """Used in concert with back_transform_codes to convert values
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

    def __init__(self, iterable=None, name=None):
        if iterable is not None:
            _formats = [
                self.types_mapping[type(item)]
                    if not isinstance(item, (str, bytes))
                    else self.types_mapping[type(item)] % (
                        self.alignment * (len(item) // self.alignment + 1),
                    )
                for item in iterable
            ]
            self._list_len = len(_formats)
            assert sum(len(fmt) <= 8 for fmt in _formats) == self._list_len
            self._allocated_bytes = tuple(
                    self.alignment if fmt[-1] != "s" else int(fmt[:-1])
                    for fmt in _formats
            )
            _back_transform_codes = [
                self._extract_recreation_code(item) for item in iterable
            ]
            requested_size = struct.calcsize(
                "q" + self._format_size_metainfo + "".join(_formats)
            )

        else:
            requested_size = 1  # Some platforms require > 0.

        self.shm = SharedMemory(name, size=requested_size)

        if iterable is not None:
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
                *(v.encode(_enc) if isinstance(v, str) else v for v in iterable)
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
                *(_back_transform_codes)
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
        transform_function = self.back_transform_codes[transform_code]

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
            new_format = self.types_mapping[type(value)]
        else:
            if len(value) > self._allocated_bytes[position]:
                raise ValueError("exceeds available storage for existing str")
            if current_format[-1] == "s":
                new_format = current_format
            else:
                new_format = self.types_mapping[str] % (
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

    @property
    def format(self):
        "The struct packing format used by all currently stored values."
        return "".join(self._get_packing_format(i) for i in range(self._list_len))

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

    @classmethod
    def copy(cls, self):
        "L.copy() -> ShareableList -- a shallow copy of L."

        return cls(self)

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

    def register_segment(self, segment):
        util.debug(f"Registering segment {segment.name!r} in pid {os.getpid()}")
        self.segment_names.append(segment.name)

    def destroy_segment(self, segment_name):
        util.debug(f"Destroying segment {segment_name!r} in pid {os.getpid()}")
        self.segment_names.remove(segment_name)
        segment = SharedMemory(segment_name, size=1)
        segment.close()
        segment.unlink()

    def unlink(self):
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
        self.register_segment(wrapped_obj._shm)
        return wrapped_obj


class SharedMemoryServer(Server):
    def __init__(self, *args, **kwargs):
        Server.__init__(self, *args, **kwargs)
        self.shared_memory_context = \
            SharedMemoryTracker(f"shmm_{self.address}_{os.getpid()}")
        util.debug(f"SharedMemoryServer started by pid {os.getpid()}")

    def create(self, c, typeid, *args, **kwargs):
        # Unless set up as a shared proxy, don't make shared_memory_context
        # a standard part of kwargs.  This makes things easier for supplying
        # simple functions.
        if hasattr(self.registry[typeid][-1], "_shared_memory_proxy"):
            kwargs['shared_memory_context'] = self.shared_memory_context
        return Server.create(self, c, typeid, *args, **kwargs)

    def shutdown(self, c):
        self.shared_memory_context.unlink()
        return Server.shutdown(self, c)


class SharedMemoryManager(SyncManager):
    """Like SyncManager but uses SharedMemoryServer instead of Server.

    TODO: Consider relocate/merge into managers submodule."""

    _Server = SharedMemoryServer

    def __init__(self, *args, **kwargs):
        SyncManager.__init__(self, *args, **kwargs)
        util.debug(f"{self.__class__.__name__} created by pid {os.getpid()}")

    def __del__(self):
        util.debug(f"{self.__class__.__name__} told die by pid {os.getpid()}")
        pass

    def get_server(self):
        'Better than monkeypatching for now; merge into Server ultimately'
        if self._state.value != State.INITIAL:
            if self._state.value == State.STARTED:
                raise ProcessError("Already started server")
            elif self._state.value == State.SHUTDOWN:
                raise ProcessError("Manager has shut down")
            else:
                raise ProcessError(
                    "Unknown state {!r}".format(self._state.value))
        return _Server(self._registry, self._address,
                       self._authkey, self._serializer)
