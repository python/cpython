"""
Parts of our build process (looking at you, deepfreeze) need the opcode module
for the Python *being built*, not the Python *doing the building*.

This basically just loads ../../Lib/opcode.py and re-exports everything:

>>> import opcode_for_build as opcode
"""

import os

_opcode_path = os.path.join(
    os.path.dirname(__file__), os.pardir, os.pardir, "Lib", "opcode.py"
)
with open(_opcode_path, encoding="utf-8") as _opcode_file:
    # Don't try this at home, kids:
    exec(_opcode_file.read())
