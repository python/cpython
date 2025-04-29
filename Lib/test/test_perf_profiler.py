import unittest
import string
import subprocess
import sys
import sysconfig
import os
import pathlib
from test import support
from test.support.script_helper import (
    make_script,
    assert_python_failure,
    assert_python_ok,
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


class TestPerfTrampoline(unittest.TestCase):
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

    @unittest.skipIf(support.check_bolt_optimized(), "fails on BOLT instrumented binaries")
    def test_trampoline_works(self):
        code = """if 1:
                def foo():
                    pass

                def bar():
                    foo()

                def baz():
                    bar()

                baz()
                """
        with temp_dir() as script_dir:
            script = make_script(script_dir, "perftest", code)
            env = {**os.environ, "PYTHON_JIT": "0"}
            with subprocess.Popen(
                [sys.executable, "-Xperf", script],
                text=True,
                stderr=subprocess.PIPE,
                stdout=subprocess.PIPE,
                env=env,
            ) as process:
                stdout, stderr = process.communicate()

        self.assertEqual(stderr, "")
        self.assertEqual(stdout, "")

        perf_file = pathlib.Path(f"/tmp/perf-{process.pid}.map")
        self.assertTrue(perf_file.exists())
        perf_file_contents = perf_file.read_text()
        perf_lines = perf_file_contents.splitlines()
        expected_symbols = [
            f"py::foo:{script}",
            f"py::bar:{script}",
            f"py::baz:{script}",
        ]
        for expected_symbol in expected_symbols:
            perf_line = next(
                (line for line in perf_lines if expected_symbol in line), None
            )
            self.assertIsNotNone(
                perf_line, f"Could not find {expected_symbol} in perf file"
            )
            perf_addr = perf_line.split(" ")[0]
            self.assertFalse(
                perf_addr.startswith("0x"), "Address should not be prefixed with 0x"
            )
            self.assertTrue(
                set(perf_addr).issubset(string.hexdigits),
                "Address should contain only hex characters",
            )

    @unittest.skipIf(support.check_bolt_optimized(), "fails on BOLT instrumented binaries")
    def test_trampoline_works_with_forks(self):
        code = """if 1:
                import os, sys

                def foo_fork():
                    pass

                def bar_fork():
                    foo_fork()

                def baz_fork():
                    bar_fork()

                def foo():
                    pid = os.fork()
                    if pid == 0:
                        print(os.getpid())
                        baz_fork()
                    else:
                        _, status = os.waitpid(-1, 0)
                        sys.exit(status)

                def bar():
                    foo()

                def baz():
                    bar()

                baz()
                """
        with temp_dir() as script_dir:
            script = make_script(script_dir, "perftest", code)
            env = {**os.environ, "PYTHON_JIT": "0"}
            with subprocess.Popen(
                [sys.executable, "-Xperf", script],
                text=True,
                stderr=subprocess.PIPE,
                stdout=subprocess.PIPE,
                env=env,
            ) as process:
                stdout, stderr = process.communicate()

        self.assertEqual(process.returncode, 0)
        self.assertEqual(stderr, "")
        child_pid = int(stdout.strip())
        perf_file = pathlib.Path(f"/tmp/perf-{process.pid}.map")
        perf_child_file = pathlib.Path(f"/tmp/perf-{child_pid}.map")
        self.assertTrue(perf_file.exists())
        self.assertTrue(perf_child_file.exists())

        perf_file_contents = perf_file.read_text()
        self.assertIn(f"py::foo:{script}", perf_file_contents)
        self.assertIn(f"py::bar:{script}", perf_file_contents)
        self.assertIn(f"py::baz:{script}", perf_file_contents)

        child_perf_file_contents = perf_child_file.read_text()
        self.assertIn(f"py::foo_fork:{script}", child_perf_file_contents)
        self.assertIn(f"py::bar_fork:{script}", child_perf_file_contents)
        self.assertIn(f"py::baz_fork:{script}", child_perf_file_contents)

    @unittest.skipIf(support.check_bolt_optimized(), "fails on BOLT instrumented binaries")
    def test_sys_api(self):
        code = """if 1:
                import sys
                def foo():
                    pass

                def spam():
                    pass

                def bar():
                    sys.deactivate_stack_trampoline()
                    foo()
                    sys.activate_stack_trampoline("perf")
                    spam()

                def baz():
                    bar()

                sys.activate_stack_trampoline("perf")
                baz()
                """
        with temp_dir() as script_dir:
            script = make_script(script_dir, "perftest", code)
            env = {**os.environ, "PYTHON_JIT": "0"}
            with subprocess.Popen(
                [sys.executable, script],
                text=True,
                stderr=subprocess.PIPE,
                stdout=subprocess.PIPE,
                env=env,
            ) as process:
                stdout, stderr = process.communicate()

        self.assertEqual(stderr, "")
        self.assertEqual(stdout, "")

        perf_file = pathlib.Path(f"/tmp/perf-{process.pid}.map")
        self.assertTrue(perf_file.exists())
        perf_file_contents = perf_file.read_text()
        self.assertNotIn(f"py::foo:{script}", perf_file_contents)
        self.assertIn(f"py::spam:{script}", perf_file_contents)
        self.assertIn(f"py::bar:{script}", perf_file_contents)
        self.assertIn(f"py::baz:{script}", perf_file_contents)

    def test_sys_api_with_existing_trampoline(self):
        code = """if 1:
                import sys
                sys.activate_stack_trampoline("perf")
                sys.activate_stack_trampoline("perf")
                """
        assert_python_ok("-c", code, PYTHON_JIT="0")

    def test_sys_api_with_invalid_trampoline(self):
        code = """if 1:
                import sys
                sys.activate_stack_trampoline("invalid")
                """
        rc, out, err = assert_python_failure("-c", code, PYTHON_JIT="0")
        self.assertIn("invalid backend: invalid", err.decode())

    def test_sys_api_get_status(self):
        code = """if 1:
                import sys
                sys.activate_stack_trampoline("perf")
                assert sys.is_stack_trampoline_active() is True
                sys.deactivate_stack_trampoline()
                assert sys.is_stack_trampoline_active() is False
                """
        assert_python_ok("-c", code, PYTHON_JIT="0")


def is_unwinding_reliable_with_frame_pointers():
    cflags = sysconfig.get_config_var("PY_CORE_CFLAGS")
    if not cflags:
        return False
    return "no-omit-frame-pointer" in cflags


def perf_command_works():
    try:
        cmd = ["perf", "--help"]
        stdout = subprocess.check_output(cmd, text=True)
    except (subprocess.SubprocessError, OSError):
        return False

    # perf version does not return a version number on Fedora. Use presence
    # of "perf.data" in help as indicator that it's perf from Linux tools.
    if "perf.data" not in stdout:
        return False

    # Check that we can run a simple perf run
    with temp_dir() as script_dir:
        try:
            output_file = script_dir + "/perf_output.perf"
            cmd = (
                "perf",
                "record",
                "--no-buildid",
                "--no-buildid-cache",
                "-g",
                "--call-graph=fp",
                "-o",
                output_file,
                "--",
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


def run_perf(cwd, *args, use_jit=False, **env_vars):
    env = os.environ.copy()
    if env_vars:
        env.update(env_vars)
    env["PYTHON_JIT"] = "0"
    output_file = cwd + "/perf_output.perf"
    if not use_jit:
        base_cmd = (
            "perf",
            "record",
            "--no-buildid",
            "--no-buildid-cache",
            "-g",
            "--call-graph=fp",
            "-o", output_file,
            "--"
        )
    else:
        base_cmd = (
            "perf",
            "record",
            "--no-buildid",
            "--no-buildid-cache",
            "-g",
            "--call-graph=dwarf,65528",
            "-F99",
            "-k1",
            "-o",
            output_file,
            "--",
        )
    proc = subprocess.run(
        base_cmd + args,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=env,
    )
    if proc.returncode:
        print(proc.stderr, file=sys.stderr)
        raise ValueError(f"Perf failed with return code {proc.returncode}")

    if use_jit:
        jit_output_file = cwd + "/jit_output.dump"
        command = ("perf", "inject", "-j", "-i", output_file, "-o", jit_output_file)
        proc = subprocess.run(
            command, stderr=subprocess.PIPE, stdout=subprocess.PIPE, env=env
        )
        if proc.returncode:
            print(proc.stderr)
            raise ValueError(f"Perf failed with return code {proc.returncode}")
        # Copy the jit_output_file to the output_file
        os.rename(jit_output_file, output_file)

    base_cmd = ("perf", "script")
    proc = subprocess.run(
        ("perf", "script", "-i", output_file),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=env,
        check=True,
    )
    return proc.stdout.decode("utf-8", "replace"), proc.stderr.decode(
        "utf-8", "replace"
    )


class TestPerfProfilerMixin:
    def run_perf(self, script_dir, perf_mode, script):
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
            stdout, stderr = self.run_perf(script_dir, script)
            self.assertEqual(stderr, "")

            self.assertIn(f"py::foo:{script}", stdout)
            self.assertIn(f"py::bar:{script}", stdout)
            self.assertIn(f"py::baz:{script}", stdout)

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
            stdout, stderr = self.run_perf(
                script_dir, script, activate_trampoline=False
            )
            self.assertEqual(stderr, "")

            self.assertNotIn(f"py::foo:{script}", stdout)
            self.assertNotIn(f"py::bar:{script}", stdout)
            self.assertNotIn(f"py::baz:{script}", stdout)


@unittest.skipUnless(perf_command_works(), "perf command doesn't work")
@unittest.skipUnless(
    is_unwinding_reliable_with_frame_pointers(),
    "Unwinding is unreliable with frame pointers",
)
class TestPerfProfiler(unittest.TestCase, TestPerfProfilerMixin):
    def run_perf(self, script_dir, script, activate_trampoline=True):
        if activate_trampoline:
            return run_perf(script_dir, sys.executable, "-Xperf", script)
        return run_perf(script_dir, sys.executable, script)

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


def _is_perf_version_at_least(major, minor):
    # The output of perf --version looks like "perf version 6.7-3" but
    # it can also be perf version "perf version 5.15.143", or even include
    # a commit hash in the version string, like "6.12.9.g242e6068fd5c"
    try:
        output = subprocess.check_output(["perf", "--version"], text=True)
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False
    version = output.split()[2]
    version = version.split("-")[0]
    version = version.split(".")
    version = tuple(map(int, version[:2]))
    return version >= (major, minor)


@unittest.skipUnless(perf_command_works(), "perf command doesn't work")
@unittest.skipUnless(
    _is_perf_version_at_least(6, 6), "perf command may not work due to a perf bug"
)
class TestPerfProfilerWithDwarf(unittest.TestCase, TestPerfProfilerMixin):
    def run_perf(self, script_dir, script, activate_trampoline=True):
        if activate_trampoline:
            return run_perf(
                script_dir, sys.executable, "-Xperf_jit", script, use_jit=True
            )
        return run_perf(script_dir, sys.executable, script, use_jit=True)

    def setUp(self):
        super().setUp()
        self.perf_files = set(pathlib.Path("/tmp/").glob("jit*.dump"))
        self.perf_files |= set(pathlib.Path("/tmp/").glob("jitted-*.so"))

    def tearDown(self) -> None:
        super().tearDown()
        files_to_delete = set(pathlib.Path("/tmp/").glob("jit*.dump"))
        files_to_delete |= set(pathlib.Path("/tmp/").glob("jitted-*.so"))
        files_to_delete = files_to_delete - self.perf_files
        for file in files_to_delete:
            file.unlink()


if __name__ == "__main__":
    unittest.main()
