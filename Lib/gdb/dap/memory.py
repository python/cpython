# Copyright 2023-2024 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import base64

import gdb

from .server import capability, request
from .startup import DAPException


@request("readMemory")
@capability("supportsReadMemoryRequest")
def read_memory(*, memoryReference: str, offset: int = 0, count: int, **extra):
    addr = int(memoryReference, 0) + offset
    try:
        buf = gdb.selected_inferior().read_memory(addr, count)
    except MemoryError as e:
        raise DAPException("Out of memory") from e
    return {
        "address": hex(addr),
        "data": base64.b64encode(buf).decode("ASCII"),
    }


@request("writeMemory")
@capability("supportsWriteMemoryRequest")
def write_memory(*, memoryReference: str, offset: int = 0, data: str, **extra):
    addr = int(memoryReference, 0) + offset
    buf = base64.b64decode(data)
    gdb.selected_inferior().write_memory(addr, buf)
