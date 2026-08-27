from __future__ import annotations
from ctypes.util import struct
from ctypes import c_int

class TestAnn:
    x: c_int

# Check that "from __future__ import annotations" works as expected
if not isinstance(TestAnn.__annotations__['x'], str):
    raise Exception("annotations must be strings")

@struct
class Point:
    x: c_int
    y: c_int
