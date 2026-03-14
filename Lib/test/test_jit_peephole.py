"""Wrapper to run the JIT peephole optimizer tests via 'make test'.

The actual tests live in Tools/jit/test_peephole.py.  This module
adds Tools/jit to sys.path and imports the test cases so they are
discovered by the standard test runner.
"""

import os
import sys
import unittest

# Tools/jit is not on the default path — add it so test_peephole can
# import _asm_to_dasc.
_jit_tools_dir = os.path.join(
    os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))),
    "Tools", "jit",
)

# Skip entirely if Tools/jit doesn't exist (e.g. minimal install).
if not os.path.isfile(os.path.join(_jit_tools_dir, "test_peephole.py")):
    raise unittest.SkipTest("Tools/jit/test_peephole.py not found")

_saved_path = sys.path[:]
try:
    if _jit_tools_dir not in sys.path:
        sys.path.insert(0, _jit_tools_dir)
    # Import all test classes from the real test module.
    from test_peephole import *  # noqa: F401,F403
finally:
    sys.path[:] = _saved_path

if __name__ == "__main__":
    unittest.main()
