Issue #24731: Fixed crash on converting objects with special methods
__bytes__, __trunc__, and __float__ returning instances of subclasses of
bytes, int, and float to subclasses of bytes, int, and float correspondingly.