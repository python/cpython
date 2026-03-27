import os
import re
import sys
import unittest

from .util import setup_module, DebuggerTests


JIT_SAMPLE_SCRIPT = os.path.join(os.path.dirname(__file__), "gdb_jit_sample.py")
# In batch GDB, break in builtin_id() while it is running under JIT,
# then repeatedly "finish" until the selected frame is the executor.
# That gives a deterministic backtrace starting with py::jit_executor:<jit>.
#
# builtin_id() sits only a few helper frames above the executor on this path.
# This bound is just a generous upper limit so the test fails clearly if the
# expected stack shape changes.
MAX_FINISH_STEPS = 20
# After landing on the executor frame, single-step a little further into the
# blob so the backtrace is taken from executor code itself rather than the
# immediate helper-return site.
EXECUTOR_SINGLE_STEPS = 2

FINISH_TO_JIT_EXECUTOR = (
    "python exec(\"import gdb\\n"
    "target = 'py::jit_executor:<jit>'\\n"
    f"for _ in range({MAX_FINISH_STEPS}):\\n"
    "    frame = gdb.selected_frame()\\n"
    "    if frame is not None and frame.name() == target:\\n"
    "        break\\n"
    "    gdb.execute('finish')\\n"
    "else:\\n"
    "    raise RuntimeError('did not reach %s' % target)\\n\")"
)


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
        self.assertRegex(
            gdb_output,
            re.compile(
                r"py::jit_executor:<jit>.*py::jit_shim:<jit>.*"
                r"(_PyEval_EvalFrameDefault|_PyEval_Vector)",
                re.DOTALL,
            ),
        )

    def test_bt_unwinds_from_inside_jit_executor(self):
        gdb_output = self.get_stack_trace(
            script=JIT_SAMPLE_SCRIPT,
            cmds_after_breakpoint=[
                FINISH_TO_JIT_EXECUTOR,
                *(["si"] * EXECUTOR_SINGLE_STEPS),
                "bt",
            ],
            PYTHON_JIT="1",
        )
        self.assertRegex(
            gdb_output,
            re.compile(
                r"#0\s+py::jit_executor:<jit>.*#1\s+py::jit_shim:<jit>.*"
                r"(_PyEval_EvalFrameDefault|_PyEval_Vector)",
                re.DOTALL,
            ),
        )
