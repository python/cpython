import os
import platform
import re
import sys
import unittest

from .util import setup_module, DebuggerTests


JIT_SAMPLE_SCRIPT = os.path.join(os.path.dirname(__file__), "gdb_jit_sample.py")
# In batch GDB, break in builtin_id() while it is running under JIT,
# then repeatedly "finish" until the selected frame is the JIT entry.
# That gives a deterministic backtrace starting with py::jit_entry:<jit>.
#
# builtin_id() sits only a few helper frames above the JIT entry on this path.
# This bound is just a generous upper limit so the test fails clearly if the
# expected stack shape changes.
MAX_FINISH_STEPS = 20
# Break directly on the lazy shim entry in the binary, then single-step just
# enough to let it install the compiled JIT entry and set a temporary
# breakpoint on the resulting address.
MAX_ENTRY_SETUP_STEPS = 20
# After landing on the JIT entry frame, single-step a little further into the
# blob so the backtrace is taken from JIT code itself rather than the
# immediate helper-return site.
JIT_ENTRY_SINGLE_STEPS = 2
EVAL_FRAME_RE = r"(_PyEval_EvalFrameDefault|_PyEval_Vector)"

FINISH_TO_JIT_ENTRY = (
    "python exec(\"import gdb\\n"
    "target = 'py::jit_entry:<jit>'\\n"
    f"for _ in range({MAX_FINISH_STEPS}):\\n"
    "    frame = gdb.selected_frame()\\n"
    "    if frame is not None and frame.name() == target:\\n"
    "        break\\n"
    "    gdb.execute('finish')\\n"
    "else:\\n"
    "    raise RuntimeError('did not reach %s' % target)\\n\")"
)
BREAK_IN_COMPILED_JIT_ENTRY = (
    "python exec(\"import gdb\\n"
    "lazy = int(gdb.parse_and_eval('(void*)_Py_LazyJitShim'))\\n"
    f"for _ in range({MAX_ENTRY_SETUP_STEPS}):\\n"
    "    entry = int(gdb.parse_and_eval('(void*)_Py_jit_entry'))\\n"
    "    if entry != lazy:\\n"
    "        gdb.execute('tbreak *0x%x' % entry)\\n"
    "        break\\n"
    "    gdb.execute('next')\\n"
    "else:\\n"
    "    raise RuntimeError('compiled JIT entry was not installed')\\n\")"
)


def setUpModule():
    setup_module()


# The GDB JIT interface registration is gated on __linux__ && __ELF__ in
# Python/jit_unwind.c, and the synthetic EH-frame is only implemented for
# x86_64 and AArch64 (a #error fires otherwise). Skip cleanly on other
# platforms or architectures instead of producing timeouts / empty backtraces.
# is_enabled() implies is_available() and also implies that the runtime has
# JIT execution active; interpreter-only tier 2 builds don't hit this path.
@unittest.skipUnless(sys.platform == "linux",
                     "GDB JIT interface is only implemented for Linux + ELF")
@unittest.skipUnless(platform.machine() in ("x86_64", "aarch64"),
                     "GDB JIT CFI emitter only supports x86_64 and AArch64")
@unittest.skipUnless(hasattr(sys, "_jit") and sys._jit.is_enabled(),
                     "requires a JIT-enabled build with JIT execution active")
class JitBacktraceTests(DebuggerTests):
    def test_bt_shows_compiled_jit_entry(self):
        gdb_output = self.get_stack_trace(
            script=JIT_SAMPLE_SCRIPT,
            breakpoint="_Py_LazyJitShim",
            cmds_after_breakpoint=[
                BREAK_IN_COMPILED_JIT_ENTRY,
                "continue",
                "bt",
            ],
            PYTHON_JIT="1",
        )
        # GDB registers the compiled JIT entry and per-trace JIT regions under
        # the same synthetic symbol name.
        self.assertRegex(
            gdb_output,
            re.compile(
                rf"#0\s+py::jit_entry:<jit>.*{EVAL_FRAME_RE}",
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
                rf"py::jit_entry:<jit>.*{EVAL_FRAME_RE}",
                re.DOTALL,
            ),
        )

    def test_bt_unwinds_from_inside_jit_entry(self):
        gdb_output = self.get_stack_trace(
            script=JIT_SAMPLE_SCRIPT,
            cmds_after_breakpoint=[
                FINISH_TO_JIT_ENTRY,
                *(["si"] * JIT_ENTRY_SINGLE_STEPS),
                "bt",
            ],
            PYTHON_JIT="1",
        )
        # Once the selected PC is inside the JIT entry, we only require that
        # GDB can identify the JIT frame and keep unwinding into _PyEval_*.
        self.assertRegex(
            gdb_output,
            re.compile(
                rf"#0\s+py::jit_entry:<jit>.*{EVAL_FRAME_RE}",
                re.DOTALL,
            ),
        )
