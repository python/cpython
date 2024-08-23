import sys
import warnings
import struct

from _ctypes import CField
import ctypes

class _BaseLayout:
    def __init__(self, cls, fields, is_struct, base, **kwargs):
        if kwargs:
            warnings.warn(f'Unknown keyword arguments: {list(kwargs.keys())}')
        self.cls = cls
        self.fields = fields
        self.is_struct = is_struct

        self.size = 0
        self.offset = 0
        if base:
            if is_struct:
                self.size = self.offset = ctypes.sizeof(base)
            base_align = ctypes.alignment(base)
        else:
            self.offset = 0
            base_align = 1

        self.align = getattr(cls, '_align_', 1)
        if self.align < 0:
            raise ValueError('_align_ must be a non-negative integer')
        elif self.align == 0:
            # Setting `_align_ = 0` amounts to using the default alignment
            self.align == 1

        self.swapped_bytes = hasattr(cls, '_swappedbytes_')

        self.total_align = max(self.align, base_align)

        self._pack_ = getattr(cls, '_pack_', None)

    def __iter__(self):
        self.gave_up = False
        for i, field in enumerate(self.fields):
            field = tuple(field)
            try:
                name, ftype, bit_size = field
            except ValueError:
                name, ftype = field
                bit_size = None
            size = ctypes.sizeof(ftype)
            offset = self.offset

            if bit_size:
                self.gave_up = True
            if size != ctypes.alignment(ftype):
                self.gave_up = True
            if self._pack_:
                self.gave_up = True

            ################################## State check (remove this)
            if isinstance(self, WindowsLayout):
                state_field_size = size * 8
            else:
                state_field_size = 0
            state_bitofs = -1
            state_offset = -1
            state_size = -1
            state_align = -1

            if not self.gave_up:
                if isinstance(self, GCCSysVLayout):
                    # We don't use packstate->offset here, so clear it, if it has been set.
                    state_offset = 0
                else:
                    state_offset = self.offset
                state_align = ctypes.alignment(ftype)

            state_to_check = struct.pack(
                "nnnnn",
                state_field_size, state_bitofs, state_offset, state_size, state_align
            )
            ##################################

            yield CField(
                name=name,
                type=ftype,
                size=size,
                offset=offset,
                bit_size=bit_size,
                swapped_bytes=self.swapped_bytes,
                pack=self._pack_,
                index=i,
                state_to_check=state_to_check,
                **self._field_args(),
            )
            if self.is_struct:
                self.offset += self.size

    def _field_args(self):
        return {}


class WindowsLayout(_BaseLayout):
    def _field_args(self):
        return {'_ms': True}

class GCCSysVLayout(_BaseLayout):
    def __init__(self, cls, *args, **kwargs):
        if getattr(cls, '_pack_', None):
            raise ValueError('_pack_ is not compatible with gcc-sysv layout')
        return super().__init__(cls, *args, **kwargs)

if sys.platform == 'win32':
    NativeLayout = WindowsLayout
    DefaultLayout = WindowsLayout
else:
    NativeLayout = GCCSysVLayout

def default_layout(cls, *args, **kwargs):
    layout = getattr(cls, '_layout_', None)
    if layout is None:
        if sys.platform == 'win32' or getattr(cls, '_pack_', None):
            return WindowsLayout(cls, *args, **kwargs)
        return GCCSysVLayout(cls, *args, **kwargs)
    elif layout == 'ms':
        return WindowsLayout(cls, *args, **kwargs)
    elif layout == 'gcc-sysv':
        return GCCSysVLayout(cls, *args, **kwargs)
    else:
        raise ValueError(f'unknown _layout_: {layout!r}')
