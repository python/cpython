import unittest
import subprocess
import re
import sys
import sysconfig
import os
from test.support.script_helper import make_script
from test.support.os_helper import temp_dir
from test.support import check_sanitizer


def get_perf_version():
    try:
        cmd = ["perf", "--version"]
        proc = subprocess.run(
            cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True
        )
    except subprocess.SubprocessError:
        raise unittest.SkipTest("Couldn't find perf on the path")
    
    version = proc.stdout

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
    output_file = cwd + "/perf_output.perf"
    base_cmd = ("perf", "record", "-g", "--call-graph=fp", "-o", output_file, "--")
    proc = subprocess.run(
        base_cmd + args,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=env,
    )
    base_cmd = ("perf", "script")
    proc = subprocess.run(
        ("perf", "script", "-i", output_file),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=env,
    )
    return proc.stdout.decode("utf-8", "replace"), proc.stderr.decode("utf-8", "replace")


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
