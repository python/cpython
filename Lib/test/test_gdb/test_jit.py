import os
import re
import sys
import unittest

from .util import setup_module, DebuggerTests


JIT_SAMPLE_SCRIPT = os.path.join(os.path.dirname(__file__), "gdb_jit_sample.py")


def setUpModule():
    setup_module()


@unittest.skipUnless(
    hasattr(sys, "_jit") and sys._jit.is_available(),
    "requires a JIT-enabled build",
)
class JitBacktraceTests(DebuggerTests):
    def test_bt_unwinds_through_jit_frames(self):
        gdb_output = self.get_stack_trace(
            script=JIT_SAMPLE_SCRIPT,
            cmds_after_breakpoint=["bt"],
            PYTHON_JIT="1",
        )
        self.assertIn("py::jit_executor:<jit>", gdb_output)
        self.assertIn("py::jit_shim:<jit>", gdb_output)
        self.assertRegex(
            gdb_output,
            re.compile(
                r"py::jit_executor:<jit>.*(_PyEval_EvalFrameDefault|_PyEval_Vector)",
                re.DOTALL,
            ),
        )
