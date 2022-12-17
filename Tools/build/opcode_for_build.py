"""
Parts of our build process (looking at you, deepfreeze) need the opcode module
for the Python *being built*, not the Python *doing the building*.

This basically just loads ../../Lib/opcode.py:

>>> import opcode_for_build
>>> opcode = opcode_for_build.import_opcode()
"""

import os
import types

_OPCODE_PATH = os.path.realpath(
    os.path.join(
        os.path.dirname(__file__), os.pardir, os.pardir, "Lib", "opcode.py"
    )
)

def import_opcode() -> types.ModuleType:
    """Import the current version of the opcode module (from Lib)."""
    opcode_module = types.ModuleType("opcode")
    opcode_module.__file__ = os.path.realpath(_OPCODE_PATH)
    with open(_OPCODE_PATH, encoding="utf-8") as opcode_file:
        # Don't try this at home, kids:
        exec(opcode_file.read(), opcode_module.__dict__)
    return opcode_module
