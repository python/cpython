import unittest
import subprocess
import re
import sys
import sysconfig
from test.support.script_helper import make_script
from test.support.os_helper import temp_dir
from test.support import check_sanitizer


def get_perf_version():
    try:
        cmd = ["perf", "--version"]
        proc = subprocess.Popen(
            cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True
        )
        with proc:
            version, stderr = proc.communicate()

        if proc.returncode:
            raise Exception(
                f"Command {' '.join(cmd)!r} failed "
                f"with exit code {proc.returncode}: "
                f"stdout={version!r} stderr={stderr!r}"
            )
    except OSError:
        raise unittest.SkipTest("Couldn't find perf on the path")

    match = re.search(r"^perf version\s+(.*)", version)
    if match is None:
        raise Exception("unable to parse perf version: %r" % version)
    return (version, match.group(1))


_, version = get_perf_version()

if not version:
    raise unittest.SkipTest("Could not find valid perf tool")

if "no-omit-frame-pointe" not in sysconfig.get_config_var("CFLAGS"):
    raise unittest.SkipTest("Unwinding without frame pointer is unreliable")

if check_sanitizer(address=True, memory=True, ub=True):
    raise unittest.SkipTest("Perf unwinding doesn't work with sanitizers")


def run_perf(cwd, *args, **env_vars):
    if env_vars:
        env = os.environ.copy()
        env.update(env_vars)
    else:
        env = None
    # -nx: Do not execute commands from any .gdbinit initialization files
    #      (issue #22188)
    output_file = cwd + "/perf_output.perf"
    base_cmd = ("perf", "record", "-g", "--call-graph=fp", "-o", output_file, "--")
    proc = subprocess.Popen(
        base_cmd + args,
        # Redirect stdin to prevent GDB from messing with
        # the terminal settings
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=env,
    )
    with proc:
        out, err = proc.communicate()
    base_cmd = ("perf", "script")
    proc = subprocess.Popen(
        ("perf", "script", "-i", output_file),
        # Redirect stdin to prevent GDB from messing with
        # the terminal settings
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=env,
    )
    with proc:
        out, err = proc.communicate()
    return out.decode("utf-8", "replace"), err.decode("utf-8", "replace")


class TestPerfProfiler(unittest.TestCase):
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
