import unittest
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
            with subprocess.Popen(
                [sys.executable, "-Xperf", script],
                text=True,
                stderr=subprocess.PIPE,
                stdout=subprocess.PIPE,
            ) as process:
                stdout, stderr = process.communicate()

        self.assertEqual(stderr, "")
        self.assertEqual(stdout, "")

        perf_file = pathlib.Path(f"/tmp/perf-{process.pid}.map")
        self.assertTrue(perf_file.exists())
        perf_file_contents = perf_file.read_text()
        self.assertIn(f"py::foo:{script}", perf_file_contents)
        self.assertIn(f"py::bar:{script}", perf_file_contents)
        self.assertIn(f"py::baz:{script}", perf_file_contents)

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
            with subprocess.Popen(
                [sys.executable, "-Xperf", script],
                universal_newlines=True,
                stderr=subprocess.PIPE,
                stdout=subprocess.PIPE,
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
            with subprocess.Popen(
                [sys.executable, script],
                universal_newlines=True,
                stderr=subprocess.PIPE,
                stdout=subprocess.PIPE,
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
        assert_python_ok("-c", code)

    def test_sys_api_with_invalid_trampoline(self):
        code = """if 1:
                import sys
                sys.activate_stack_trampoline("invalid")
                """
        rc, out, err = assert_python_failure("-c", code)
        self.assertIn("invalid backend: invalid", err.decode())

    def test_sys_api_get_status(self):
        code = """if 1:
                import sys
                sys.activate_stack_trampoline("perf")
                assert sys.is_stack_trampoline_active() is True
                sys.deactivate_stack_trampoline()
                assert sys.is_stack_trampoline_active() is False
                """
        assert_python_ok("-c", code)


def is_unwinding_reliable():
    cflags = sysconfig.get_config_var("PY_CORE_CFLAGS")
    if not cflags:
        return False
    return "no-omit-frame-pointer" in cflags


def perf_command_works():
    try:
        cmd = ["perf", "--help"]
        stdout = subprocess.check_output(cmd, universal_newlines=True)
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
                "-g",
                "--call-graph=fp",
                "-o",
                output_file,
                "--",
                sys.executable,
                "-c",
                'print("hello")',
            )
            stdout = subprocess.check_output(
                cmd, cwd=script_dir, universal_newlines=True, stderr=subprocess.STDOUT
            )
        except (subprocess.SubprocessError, OSError):
            return False

        if "hello" not in stdout:
            return False

    return True


def run_perf(cwd, *args, **env_vars):
    if env_vars:
        env = os.environ.copy()
        env.update(env_vars)
    else:
        env = None
    output_file = cwd + "/perf_output.perf"
    base_cmd = ("perf", "record", "-g", "--call-graph=fp", "-o", output_file, "--")
    proc = subprocess.run(
        base_cmd + args,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=env,
    )
    if proc.returncode:
        print(proc.stderr)
        raise ValueError(f"Perf failed with return code {proc.returncode}")

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


@unittest.skipUnless(perf_command_works(), "perf command doesn't work")
@unittest.skipUnless(is_unwinding_reliable(), "Unwinding is unreliable")
@support.skip_if_sanitizer(address=True, memory=True, ub=True)
class TestPerfProfiler(unittest.TestCase):
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
            stdout, stderr = run_perf(script_dir, sys.executable, "-Xperf", script)
            self.assertEqual(stderr, "")

            self.assertIn(f"py::foo:{script}", stdout)
            self.assertIn(f"py::bar:{script}", stdout)
            self.assertIn(f"py::baz:{script}", stdout)

    def test_python_calls_do_not_appear_in_the_stack_if_perf_activated(self):
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
            stdout, stderr = run_perf(script_dir, sys.executable, script)
            self.assertEqual(stderr, "")

            self.assertNotIn(f"py::foo:{script}", stdout)
            self.assertNotIn(f"py::bar:{script}", stdout)
            self.assertNotIn(f"py::baz:{script}", stdout)


if __name__ == "__main__":
    unittest.main()
