import json
import os
import platform
import subprocess
import sys
import sysconfig
import unittest

from test import support
from test.support import import_helper


_testinternalcapi = import_helper.import_module("_testinternalcapi")


if not support.has_subprocess_support:
    raise unittest.SkipTest("test requires subprocess support")


def _frame_pointers_expected(machine):
    cflags = " ".join(
        value for value in (
            sysconfig.get_config_var("PY_CORE_CFLAGS"),
            sysconfig.get_config_var("CFLAGS"),
        )
        if value
    )
    if "no-omit-frame-pointer" in cflags:
        return True
    if "omit-frame-pointer" in cflags:
        return False
    if sys.platform == "darwin":
        # macOS x86_64/ARM64 always have frame pointer by default.
        return True
    if sys.platform == "linux":
        if machine in {"aarch64", "arm64"}:
            # 32-bit Linux is not supported
            if sys.maxsize < 2**32:
                return None
            return True
        if machine == "x86_64":
            return False
    if sys.platform == "win32":
        # MSVC ignores /Oy and /Oy- on x64/ARM64.
        if machine == "arm64":
            # Windows ARM64 guidelines recommend frame pointers (x29) for stack walking.
            return True
        elif machine == "x86_64":
            # Windows x64 uses unwind metadata; frame pointers are not required.
            return None
    return None


def _build_stack_and_unwind():
    import operator

    def build_stack(n, unwinder, warming_up_caller=False):
        if warming_up_caller:
            return
        if n == 0:
            return unwinder()
        warming_up = True
        while warming_up:
            # Can't branch on JIT state inside JITted code, so compute here.
            warming_up = (
                hasattr(sys, "_jit")
                and sys._jit.is_enabled()
                and not sys._jit.is_active()
            )
            result = operator.call(build_stack, n - 1, unwinder, warming_up)
        return result

    stack = build_stack(10, _testinternalcapi.manual_frame_pointer_unwind)
    return stack


def _classify_stack(stack, jit_enabled):
    labels = _testinternalcapi.classify_stack_addresses(stack, jit_enabled)

    annotated = []
    jit_frames = 0
    python_frames = 0
    other_frames = 0
    for idx, (frame, tag) in enumerate(zip(stack, labels)):
        addr = int(frame)
        if tag == "jit":
            jit_frames += 1
        elif tag == "python":
            python_frames += 1
        else:
            other_frames += 1
        annotated.append((idx, addr, tag))
    return annotated, python_frames, jit_frames, other_frames


def _annotate_unwind():
    stack = _build_stack_and_unwind()
    jit_enabled = hasattr(sys, "_jit") and sys._jit.is_enabled()
    jit_backend = _testinternalcapi.get_jit_backend()
    ranges = _testinternalcapi.get_jit_code_ranges() if jit_enabled else []
    if jit_enabled and ranges:
        print("JIT ranges:")
        for start, end in ranges:
            print(f"  {int(start):#x}-{int(end):#x}")
    annotated, python_frames, jit_frames, other_frames = _classify_stack(
        stack, jit_enabled
    )
    for idx, addr, tag in annotated:
        print(f"#{idx:02d} {addr:#x} -> {tag}")
    return json.dumps({
        "length": len(stack),
        "python_frames": python_frames,
        "jit_frames": jit_frames,
        "other_frames": other_frames,
        "jit_backend": jit_backend,
    })


def _manual_unwind_length(**env):
    code = (
        "from test.test_frame_pointer_unwind import _annotate_unwind; "
        "print(_annotate_unwind());"
    )
    run_env = os.environ.copy()
    run_env.update(env)
    proc = subprocess.run(
        [sys.executable, "-c", code],
        env=run_env,
        capture_output=True,
        text=True,
    )
    # Surface the output for debugging/visibility when running this test
    if proc.stdout:
        print(proc.stdout, end="")
    if proc.returncode:
        raise RuntimeError(
            f"unwind helper failed (rc={proc.returncode}): {proc.stderr or proc.stdout}"
        )
    stdout_lines = proc.stdout.strip().splitlines()
    if not stdout_lines:
        raise RuntimeError("unwind helper produced no output")
    try:
        return json.loads(stdout_lines[-1])
    except ValueError as exc:
        raise RuntimeError(
            f"unexpected output from unwind helper: {proc.stdout!r}"
        ) from exc


@support.requires_gil_enabled("test requires the GIL enabled")
@unittest.skipIf(support.is_wasi, "test not supported on WASI")
class FramePointerUnwindTests(unittest.TestCase):

    def setUp(self):
        super().setUp()
        machine = platform.machine().lower()
        expected = _frame_pointers_expected(machine)
        if expected is None:
            self.skipTest(f"unsupported architecture for frame pointer check: {machine}")
        try:
            _testinternalcapi.manual_frame_pointer_unwind()
        except RuntimeError as exc:
            if "not supported" in str(exc):
                self.skipTest("manual frame pointer unwinding not supported on this platform")
            raise
        self.machine = machine
        self.frame_pointers_expected = expected

    def test_manual_unwind_respects_frame_pointers(self):
        jit_available = hasattr(sys, "_jit") and sys._jit.is_available()
        envs = [({"PYTHON_JIT": "0"}, False)]
        if jit_available:
            envs.append(({"PYTHON_JIT": "1"}, True))

        for env, using_jit in envs:
            with self.subTest(env=env):
                result = _manual_unwind_length(**env)
                jit_frames = result["jit_frames"]
                python_frames = result.get("python_frames", 0)
                jit_backend = result.get("jit_backend")
                if self.frame_pointers_expected:
                    self.assertGreater(
                        python_frames,
                        0,
                        f"expected to find Python frames on {self.machine} with env {env}",
                    )
                    if using_jit:
                        if jit_backend == "jit":
                            self.assertGreater(
                                jit_frames,
                                0,
                                f"expected to find JIT frames on {self.machine} with env {env}",
                            )
                        else:
                            # jit_backend is "interpreter" or not present
                            self.assertEqual(
                                jit_frames,
                                0,
                                f"unexpected JIT frames counted on {self.machine} with env {env}",
                            )
                    else:
                        self.assertEqual(
                            jit_frames,
                            0,
                            f"unexpected JIT frames counted on {self.machine} with env {env}",
                        )
                else:
                    self.assertLessEqual(
                        python_frames,
                        1,
                        f"unexpected Python frames counted on {self.machine} with env {env}",
                    )
                    self.assertEqual(
                        jit_frames,
                        0,
                        f"unexpected JIT frames counted on {self.machine} with env {env}",
                    )


if __name__ == "__main__":
    unittest.main()
