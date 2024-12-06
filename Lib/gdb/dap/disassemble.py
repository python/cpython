# Copyright 2022-2024 Free Software Foundation, Inc.

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

import gdb

from .server import capability, request
from .sources import make_source


# This tracks labels associated with a disassembly request and helps
# with updating individual instructions.
class _BlockTracker:
    def __init__(self):
        # Map from PC to symbol names.  A given PC is assumed to have
        # just one label -- DAP wouldn't let us return multiple labels
        # anyway.
        self.labels = {}
        # List of blocks that have already been handled.  Note that
        # blocks aren't hashable so a set is not used.
        self.blocks = []

    # Add a gdb.Block and its superblocks, ignoring the static and
    # global block.  BLOCK can also be None, which is ignored.
    def add_block(self, block):
        while block is not None:
            if block.is_static or block.is_global or block in self.blocks:
                return
            self.blocks.append(block)
            if block.function is not None:
                self.labels[block.start] = block.function.name
            for sym in block:
                if sym.addr_class == gdb.SYMBOL_LOC_LABEL:
                    self.labels[int(sym.value())] = sym.name
            block = block.superblock

    # Add PC to this tracker.  Update RESULT as appropriate with
    # information about the source and any label.
    def add_pc(self, pc, result):
        self.add_block(gdb.block_for_pc(pc))
        if pc in self.labels:
            result["symbol"] = self.labels[pc]
        sal = gdb.find_pc_line(pc)
        if sal.symtab is not None:
            if sal.line != 0:
                result["line"] = sal.line
            if sal.symtab.filename is not None:
                # The spec says this can be omitted in some
                # situations, but it's a little simpler to just always
                # supply it.
                result["location"] = make_source(sal.symtab.filename)


@request("disassemble")
@capability("supportsDisassembleRequest")
def disassemble(
    *,
    memoryReference: str,
    offset: int = 0,
    instructionOffset: int = 0,
    instructionCount: int,
    **extra
):
    pc = int(memoryReference, 0) + offset
    inf = gdb.selected_inferior()
    try:
        arch = gdb.selected_frame().architecture()
    except gdb.error:
        # Maybe there was no frame.
        arch = inf.architecture()
    tracker = _BlockTracker()
    result = []
    total_count = instructionOffset + instructionCount
    for elt in arch.disassemble(pc, count=total_count)[instructionOffset:]:
        mem = inf.read_memory(elt["addr"], elt["length"])
        insn = {
            "address": hex(elt["addr"]),
            "instruction": elt["asm"],
            "instructionBytes": mem.hex(),
        }
        tracker.add_pc(elt["addr"], insn)
        result.append(insn)
    return {
        "instructions": result,
    }
