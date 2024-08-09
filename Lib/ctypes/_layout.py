import sys
import warnings

from _ctypes import CField
import ctypes

class _BaseLayout:
    def __init__(self, cls, fields, is_struct, base, **kwargs):
        if kwargs:
            warnings.warn(f'Unknown keyword arguments: {list(kwargs.keys())}')
        self.cls = cls
        self.fields = fields
        self.is_struct = is_struct

        if base:
            self.size = self.offset = ctypes.sizeof(base)
            base_align = ctypes.alignment(base)
        else:
            self.size = 0
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

    def __iter__(self):
        for field in self.fields:
            field = tuple(field)
            try:
                name, ftype, bit_size = field
            except ValueError:
                name, ftype = field
                bit_size = None
            size = ctypes.sizeof(ftype)
            offset = self.offset
            yield CField(
                name=name,
                type=ftype,
                size=size,
                offset=offset,
                bit_size=bit_size,
                swapped_bytes=self.swapped_bytes,
            )
            self.offset += self.size


class WindowsLayout(_BaseLayout):
    pass

class GCCSysVLayout(_BaseLayout):
    pass

if sys.platform == 'win32':
    NativeLayout = WindowsLayout
else:
    NativeLayout = GCCSysVLayout
