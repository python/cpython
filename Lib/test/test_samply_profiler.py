import unittest
import subprocess
import sys
import sysconfig
import os
import pathlib
from test import support
from test.support.script_helper import (
    make_script,
)
from test.support.os_helper import temp_dir


if not support.has_subprocess_support:
    raise unittest.SkipTest("test module requires subprocess")

if support.check_sanitizer(address=True, memory=True, ub=True, function=True):
    # gh-109580: Skip the test because it does crash randomly if Python is
    # built with ASAN.
    raise unittest.SkipTest("test crash randomly on ASAN/MSAN/UBSAN build")


def supports_trampoline_profiling():
    perf_trampoline = sysconfig.get_config_var("PY_HAVE_PERF_TRAMPOLINE")
    if not perf_trampoline:
        return False
    return int(perf_trampoline) == 1


if not supports_trampoline_profiling():
    raise unittest.SkipTest("perf trampoline profiling not supported")


def samply_command_works():
    try:
        cmd = ["samply", "--help"]
    except (subprocess.SubprocessError, OSError):
        return False

    # Check that we can run a simple samply run
    with temp_dir() as script_dir:
        try:
            output_file = script_dir + "/profile.json.gz"
            cmd = (
                "samply",
                "record",
                "--save-only",
                "--output",
                output_file,
                sys.executable,
                "-c",
                'print("hello")',
            )
            env = {**os.environ, "PYTHON_JIT": "0"}
            stdout = subprocess.check_output(
                cmd, cwd=script_dir, text=True, stderr=subprocess.STDOUT, env=env
            )
        except (subprocess.SubprocessError, OSError):
            return False

        if "hello" not in stdout:
            return False

    return True


def run_samply(cwd, *args, **env_vars):
    env = os.environ.copy()
    if env_vars:
        env.update(env_vars)
    env["PYTHON_JIT"] = "0"
    output_file = cwd + "/profile.json.gz"
    base_cmd = (
        "samply",
        "record",
        "--save-only",
        "-o", output_file,
    )
    proc = subprocess.run(
        base_cmd + args,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=env,
    )
    if proc.returncode:
        print(proc.stderr, file=sys.stderr)
        raise ValueError(f"Samply failed with return code {proc.returncode}")

    import gzip
    with gzip.open(output_file, mode="rt", encoding="utf-8") as f:
        return f.read()


@unittest.skipUnless(samply_command_works(), "samply command doesn't work")
class TestSamplyProfilerMixin:
    def run_samply(self, script_dir, perf_mode, script):
        raise NotImplementedError()

    def test_python_calls_appear_in_the_stack_if_perf_activated(self):
        with temp_dir() as script_dir:
            code = """if 1:
                def foo(n):
                    x = 0
                    for i in range(n):
                        x += i

                def bar(n):
                    foo(n)

                def baz(n):
                    bar(n)

                baz(10000000)
                """
            script = make_script(script_dir, "perftest", code)
            output = self.run_samply(script_dir, script)

            self.assertIn(f"py::foo:{script}", output)
            self.assertIn(f"py::bar:{script}", output)
            self.assertIn(f"py::baz:{script}", output)

    def test_python_calls_do_not_appear_in_the_stack_if_perf_deactivated(self):
        with temp_dir() as script_dir:
            code = """if 1:
                def foo(n):
                    x = 0
                    for i in range(n):
                        x += i

                def bar(n):
                    foo(n)

                def baz(n):
                    bar(n)

                baz(10000000)
                """
            script = make_script(script_dir, "perftest", code)
            output = self.run_samply(
                script_dir, script, activate_trampoline=False
            )

            self.assertNotIn(f"py::foo:{script}", output)
            self.assertNotIn(f"py::bar:{script}", output)
            self.assertNotIn(f"py::baz:{script}", output)


@unittest.skipUnless(samply_command_works(), "samply command doesn't work")
class TestSamplyProfiler(unittest.TestCase, TestSamplyProfilerMixin):
    def run_samply(self, script_dir, script, activate_trampoline=True):
        if activate_trampoline:
            return run_samply(script_dir, sys.executable, "-Xperf", script)
        return run_samply(script_dir, sys.executable, script)

    def setUp(self):
        super().setUp()
        self.perf_files = set(pathlib.Path("/tmp/").glob("perf-*.map"))

    def tearDown(self) -> None:
        super().tearDown()
        files_to_delete = (
            set(pathlib.Path("/tmp/").glob("perf-*.map")) - self.perf_files
        )
        for file in files_to_delete:
            file.unlink()

    def test_pre_fork_compile(self):
        code = """if 1:
                import sys
                import os
                import sysconfig
                from _testinternalcapi import (
                    compile_perf_trampoline_entry,
                    perf_trampoline_set_persist_after_fork,
                )

                def foo_fork():
                    pass

                def bar_fork():
                    foo_fork()

                def foo():
                    import time; time.sleep(1)

                def bar():
                    foo()

                def compile_trampolines_for_all_functions():
                    perf_trampoline_set_persist_after_fork(1)
                    for _, obj in globals().items():
                        if callable(obj) and hasattr(obj, '__code__'):
                            compile_perf_trampoline_entry(obj.__code__)

                if __name__ == "__main__":
                    compile_trampolines_for_all_functions()
                    pid = os.fork()
                    if pid == 0:
                        print(os.getpid())
                        bar_fork()
                    else:
                        bar()
                """

        with temp_dir() as script_dir:
            script = make_script(script_dir, "perftest", code)
            env = {**os.environ, "PYTHON_JIT": "0"}
            with subprocess.Popen(
                [sys.executable, "-Xperf", script],
                universal_newlines=True,
                stderr=subprocess.PIPE,
                stdout=subprocess.PIPE,
                env=env,
            ) as process:
                stdout, stderr = process.communicate()

        self.assertEqual(process.returncode, 0)
        self.assertNotIn("Error:", stderr)
        child_pid = int(stdout.strip())
        perf_file = pathlib.Path(f"/tmp/perf-{process.pid}.map")
        perf_child_file = pathlib.Path(f"/tmp/perf-{child_pid}.map")
        self.assertTrue(perf_file.exists())
        self.assertTrue(perf_child_file.exists())

        perf_file_contents = perf_file.read_text()
        self.assertIn(f"py::foo:{script}", perf_file_contents)
        self.assertIn(f"py::bar:{script}", perf_file_contents)
        self.assertIn(f"py::foo_fork:{script}", perf_file_contents)
        self.assertIn(f"py::bar_fork:{script}", perf_file_contents)

        child_perf_file_contents = perf_child_file.read_text()
        self.assertIn(f"py::foo_fork:{script}", child_perf_file_contents)
        self.assertIn(f"py::bar_fork:{script}", child_perf_file_contents)

        # Pre-compiled perf-map entries of a forked process must be
        # identical in both the parent and child perf-map files.
        perf_file_lines = perf_file_contents.split("\n")
        for line in perf_file_lines:
            if f"py::foo_fork:{script}" in line or f"py::bar_fork:{script}" in line:
                self.assertIn(line, child_perf_file_contents)


if __name__ == "__main__":
    unittest.main()
