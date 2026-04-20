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
# After landing on the JIT entry frame, single-step a bounded number of
# instructions further into the blob so the backtrace is taken from JIT code
# itself rather than the immediate helper-return site. The exact number of
# steps is not significant: each step is cross-checked against the selected
# frame's symbol so the test fails loudly if stepping escapes the registered
# JIT region, instead of asserting against a misleading backtrace.
MAX_JIT_ENTRY_STEPS = 4
EVAL_FRAME_RE = r"(_PyEval_EvalFrameDefault|_PyEval_Vector)"
BACKTRACE_FRAME_RE = re.compile(r"^#\d+\s+.*$", re.MULTILINE)

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
STEP_INSIDE_JIT_ENTRY = (
    "python exec(\"import gdb\\n"
    "target = 'py::jit_entry:<jit>'\\n"
    f"for _ in range({MAX_JIT_ENTRY_STEPS}):\\n"
    "    frame = gdb.selected_frame()\\n"
    "    if frame is None or frame.name() != target:\\n"
    "        raise RuntimeError('left JIT region during stepping: '\\n"
    "                           + repr(frame and frame.name()))\\n"
    "    gdb.execute('si')\\n"
    "frame = gdb.selected_frame()\\n"
    "if frame is None or frame.name() != target:\\n"
    "    raise RuntimeError('stepped out of JIT region after si')\\n\")"
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
    def get_stack_trace(self, **kwargs):
        # These tests validate the JIT-relevant part of the backtrace via
        # _assert_jit_backtrace_shape, so an unrelated "?? ()" frame below
        # the JIT/eval segment (e.g. libc without debug info) is tolerable.
        kwargs.setdefault("skip_on_truncation", False)
        return super().get_stack_trace(**kwargs)

    def _extract_backtrace_frames(self, gdb_output):
        frames = BACKTRACE_FRAME_RE.findall(gdb_output)
        self.assertGreater(
            len(frames), 0,
            f"expected at least one GDB backtrace frame in output:\n{gdb_output}",
        )
        return frames

    def _assert_jit_backtrace_shape(self, gdb_output, *, anchor_at_top):
        # Shape assertions applied to every JIT backtrace we produce:
        #   1. The synthetic JIT symbol appears exactly once. A second
        #      py::jit_entry:<jit> frame would mean the unwinder is
        #      materializing two native frames for a single logical JIT
        #      region, or failing to unwind out of the region entirely.
        #   2. At least one _PyEval_EvalFrameDefault / _PyEval_Vector
        #      frame appears after the JIT frame, proving the unwinder
        #      climbs back out of the JIT region into the eval loop.
        #      Helper frames from inside the JITted region may still
        #      appear above the synthetic JIT frame in the backtrace.
        #   4. For tests that assert a specific entry PC, the JIT frame
        #      is also at #0.
        frames = self._extract_backtrace_frames(gdb_output)
        backtrace = "\n".join(frames)

        jit_frames = [frame for frame in frames if "py::jit_entry:<jit>" in frame]
        jit_count = len(jit_frames)
        self.assertEqual(
            jit_count, 1,
            f"expected exactly 1 py::jit_entry:<jit> frame, got {jit_count}\n"
            f"backtrace:\n{backtrace}",
        )
        eval_frames = [frame for frame in frames if re.search(EVAL_FRAME_RE, frame)]
        eval_count = len(eval_frames)
        self.assertGreaterEqual(
            eval_count, 1,
            f"expected at least one _PyEval_* frame, got {eval_count}\n"
            f"backtrace:\n{backtrace}",
        )
        jit_frame_index = next(
            i for i, frame in enumerate(frames) if "py::jit_entry:<jit>" in frame
        )
        eval_after_jit = any(
            re.search(EVAL_FRAME_RE, frame)
            for frame in frames[jit_frame_index + 1:]
        )
        self.assertTrue(
            eval_after_jit,
            f"expected an eval frame after the JIT frame\n"
            f"backtrace:\n{backtrace}",
        )
        relevant_end = max(
            i
            for i, frame in enumerate(frames)
            if "py::jit_entry:<jit>" in frame or re.search(EVAL_FRAME_RE, frame)
        )
        truncated_frames = [
            frame for frame in frames[: relevant_end + 1]
            if " ?? ()" in frame
        ]
        self.assertFalse(
            truncated_frames,
            "unexpected truncated frame before the validated JIT/eval segment\n"
            f"backtrace:\n{backtrace}",
        )
        if anchor_at_top:
            self.assertRegex(
                frames[0],
                re.compile(r"^#0\s+py::jit_entry:<jit>"),
            )

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
        # the same synthetic symbol name; breaking at the entry PC pins the
        # JIT frame at #0.
        self._assert_jit_backtrace_shape(gdb_output, anchor_at_top=True)

    def test_bt_unwinds_through_jit_frames(self):
        gdb_output = self.get_stack_trace(
            script=JIT_SAMPLE_SCRIPT,
            cmds_after_breakpoint=["bt"],
            PYTHON_JIT="1",
        )
        # The executor should appear as a named JIT frame and unwind back into
        # the eval loop. Whether GDB also materializes a separate shim frame is
        # an implementation detail of the synthetic executor CFI.
        self._assert_jit_backtrace_shape(gdb_output, anchor_at_top=False)

    def test_bt_unwinds_from_inside_jit_entry(self):
        gdb_output = self.get_stack_trace(
            script=JIT_SAMPLE_SCRIPT,
            cmds_after_breakpoint=[
                FINISH_TO_JIT_ENTRY,
                STEP_INSIDE_JIT_ENTRY,
                "bt",
            ],
            PYTHON_JIT="1",
        )
        # Once the selected PC is inside the JIT entry, we require that GDB
        # identifies the JIT frame at #0 and keeps unwinding into _PyEval_*.
        self._assert_jit_backtrace_shape(gdb_output, anchor_at_top=True)
