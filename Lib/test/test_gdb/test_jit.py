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
# Break directly on the lazy shim entry in the binary, then single-step just
# enough to let it install the compiled shim and set a temporary breakpoint on
# the resulting JIT entry address.
MAX_SHIM_SETUP_STEPS = 20
# After landing on the executor frame, single-step a little further into the
# blob so the backtrace is taken from executor code itself rather than the
# immediate helper-return site.
EXECUTOR_SINGLE_STEPS = 2
EVAL_FRAME_RE = r"(_PyEval_EvalFrameDefault|_PyEval_Vector)"

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
BREAK_IN_COMPILED_SHIM = (
    "python exec(\"import gdb\\n"
    "lazy = int(gdb.parse_and_eval('(void*)_Py_LazyJitShim'))\\n"
    f"for _ in range({MAX_SHIM_SETUP_STEPS}):\\n"
    "    entry = int(gdb.parse_and_eval('(void*)_Py_jit_entry'))\\n"
    "    if entry != lazy:\\n"
    "        gdb.execute('tbreak *0x%x' % entry)\\n"
    "        break\\n"
    "    gdb.execute('next')\\n"
    "else:\\n"
    "    raise RuntimeError('compiled shim was not installed')\\n\")"
)


def setUpModule():
    setup_module()


@unittest.skipUnless(
    hasattr(sys, "_jit") and sys._jit.is_available(),
    "requires a JIT-enabled build",
)
class JitBacktraceTests(DebuggerTests):
    def test_bt_shows_compiled_jit_shim(self):
        gdb_output = self.get_stack_trace(
            script=JIT_SAMPLE_SCRIPT,
            breakpoint="_Py_LazyJitShim",
            cmds_after_breakpoint=[
                BREAK_IN_COMPILED_SHIM,
                "continue",
                "bt",
            ],
            PYTHON_JIT="1",
        )
        self.assertRegex(
            gdb_output,
            re.compile(
                rf"#0\s+py::jit_shim:<jit>.*{EVAL_FRAME_RE}",
                re.DOTALL,
            ),
        )

    def test_bt_unwinds_through_jit_frames(self):
        gdb_output = self.get_stack_trace(
            script=JIT_SAMPLE_SCRIPT,
            cmds_after_breakpoint=["bt"],
            PYTHON_JIT="1",
        )
        # The executor should appear as a named JIT frame and unwind back into
        # the eval loop. Whether GDB also materializes a separate shim frame is
        # an implementation detail of the synthetic executor CFI.
        self.assertRegex(
            gdb_output,
            re.compile(
                rf"py::jit_executor:<jit>.*{EVAL_FRAME_RE}",
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
        # Once the selected PC is inside the executor, we only require that
        # GDB can identify the JIT frame and keep unwinding into _PyEval_*.
        self.assertRegex(
            gdb_output,
            re.compile(
                rf"#0\s+py::jit_executor:<jit>.*{EVAL_FRAME_RE}",
                re.DOTALL,
            ),
        )
