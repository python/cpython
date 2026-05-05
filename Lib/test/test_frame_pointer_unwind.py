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


STACK_DEPTH = 10


def _frame_pointers_expected(machine):
    cflags = " ".join(
        value for value in (
            sysconfig.get_config_var("PY_CORE_CFLAGS"),
            sysconfig.get_config_var("CFLAGS"),
        )
        if value
    )

    if "no-omit-frame-pointer" in cflags:
        # For example, configure adds -fno-omit-frame-pointer by default on
        # supported GCC-compatible builds.
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
            final_opt = ""
            for opt in cflags.split():
                if opt.startswith('-O'):
                    final_opt = opt
            if final_opt in ("-O0", "-Og", "-O1"):
                # Unwinding works if the optimization level is low
                return True

            Py_ENABLE_SHARED = int(sysconfig.get_config_var('Py_ENABLE_SHARED') or '0')
            if Py_ENABLE_SHARED:
                # Unwinding does crash using gcc -O2 or gcc -O3
                # when Python is built with --enable-shared
                return "crash"
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


def _build_stack_and_unwind(unwinder):
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

    stack = build_stack(STACK_DEPTH, unwinder)
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


def _summarize_unwind(stack, unwinder_name):
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
    return {
        "length": len(stack),
        "python_frames": python_frames,
        "jit_frames": jit_frames,
        "other_frames": other_frames,
        "jit_backend": jit_backend,
        "unwinder": unwinder_name,
    }


def _annotate_unwind(unwinder_name="manual_frame_pointer_unwind"):
    unwinder = getattr(_testinternalcapi, unwinder_name)
    stack = _build_stack_and_unwind(unwinder)
    return json.dumps(_summarize_unwind(stack, unwinder_name))


def _annotate_unwind_after_executor_free(unwinder_name="gnu_backtrace_unwind"):
    # The first unwind runs at the bottom of _build_stack_and_unwind(), while
    # the recursive helper may be executing in JIT code. After it returns, this
    # helper is back in normal test code; clearing executor caches should remove
    # the old JIT ranges, so the second unwind must not report stale JIT frames.
    live = json.loads(_annotate_unwind(unwinder_name))

    sys._clear_internal_caches()
    _testinternalcapi.clear_executor_deletion_list()

    unwinder = getattr(_testinternalcapi, unwinder_name)
    after_free = _summarize_unwind(unwinder(), unwinder_name)
    return json.dumps({
        "live": live,
        "after_free": after_free,
    })


def _run_unwind_helper(helper_name, unwinder_name, **env):
    code = (
        f"from test.test_frame_pointer_unwind import {helper_name}; "
        f"print({helper_name}({unwinder_name!r}));"
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


def _unwind_result(unwinder_name, **env):
    return _run_unwind_helper("_annotate_unwind", unwinder_name, **env)


def _unwind_after_executor_free_result(unwinder_name, **env):
    return _run_unwind_helper(
        "_annotate_unwind_after_executor_free", unwinder_name, **env)


@support.requires_gil_enabled("test requires the GIL enabled")
@unittest.skipIf(support.is_wasi, "test not supported on WASI")
class FramePointerUnwindTests(unittest.TestCase):

    def setUp(self):
        super().setUp()

        machine = platform.machine().lower()
        expected = _frame_pointers_expected(machine)
        if expected is None:
            self.skipTest(f"unsupported architecture for frame pointer check: {machine}")
        if expected == "crash":
            self.skipTest(f"test does crash on {machine}")

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
                result = _unwind_result("manual_frame_pointer_unwind", **env)
                jit_frames = result["jit_frames"]
                python_frames = result.get("python_frames", 0)
                jit_backend = result.get("jit_backend")
                if self.frame_pointers_expected:
                    self.assertGreaterEqual(
                        python_frames,
                        STACK_DEPTH,
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


@support.requires_gil_enabled("test requires the GIL enabled")
@unittest.skipIf(support.is_wasi, "test not supported on WASI")
@unittest.skipUnless(sys.platform == "linux", "GNU backtrace unwinding test requires Linux")
class GnuBacktraceUnwindTests(unittest.TestCase):

    def setUp(self):
        super().setUp()
        try:
            _testinternalcapi.gnu_backtrace_unwind()
        except RuntimeError as exc:
            if "not supported" in str(exc):
                self.skipTest("gnu backtrace unwinding not supported on this platform")
            raise

    def test_gnu_backtrace_unwinds_through_jit_frames(self):
        jit_available = hasattr(sys, "_jit") and sys._jit.is_available()
        envs = [({"PYTHON_JIT": "0"}, False)]
        if jit_available:
            envs.append(({"PYTHON_JIT": "1"}, True))

        for env, using_jit in envs:
            with self.subTest(env=env):
                result = _unwind_result("gnu_backtrace_unwind", **env)
                python_frames = result.get("python_frames", 0)
                jit_frames = result.get("jit_frames", 0)
                jit_backend = result.get("jit_backend")

                self.assertGreaterEqual(
                    python_frames,
                    STACK_DEPTH,
                    f"expected to find Python frames in GNU backtrace with env {env}",
                )
                if using_jit and jit_backend == "jit":
                    self.assertGreater(
                        jit_frames,
                        0,
                        f"expected GNU backtrace to include JIT frames with env {env}",
                    )
                else:
                    self.assertEqual(
                        jit_frames,
                        0,
                        f"unexpected JIT frames counted in GNU backtrace with env {env}",
                    )

    def test_gnu_backtrace_jit_frames_disappear_after_executor_free(self):
        if not (hasattr(sys, "_jit") and sys._jit.is_available()):
            self.skipTest("JIT is not available")

        result = _unwind_after_executor_free_result(
            "gnu_backtrace_unwind", PYTHON_JIT="1")
        live = result["live"]
        if live.get("jit_backend") != "jit":
            self.skipTest("JIT backend is not active")

        self.assertGreaterEqual(
            live.get("python_frames", 0),
            STACK_DEPTH,
            "expected live GNU backtrace to include recursive Python frames",
        )
        self.assertGreater(
            live.get("jit_frames", 0),
            0,
            "expected live GNU backtrace to include JIT frames",
        )

        after_free = result["after_free"]
        self.assertGreater(
            after_free.get("python_frames", 0),
            0,
            "expected GNU backtrace after executor free to include Python frames",
        )
        self.assertEqual(
            after_free.get("jit_frames", 0),
            0,
            "unexpected JIT frames in GNU backtrace after executor free",
        )


if __name__ == "__main__":
    unittest.main()
