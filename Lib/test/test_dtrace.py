import dis
import os.path
import re
import subprocess
import sys
import sysconfig
import types
import unittest

from test import support
from test.support import findfile


if not support.has_subprocess_support:
    raise unittest.SkipTest("test module requires subprocess")


def abspath(filename):
    return os.path.abspath(findfile(filename, subdir="dtracedata"))


def normalize_trace_output(output):
    """Normalize DTrace output for comparison.

    DTrace keeps a per-CPU buffer, and when showing the fired probes, buffers
    are concatenated. So if the operating system moves our thread around, the
    straight result can be "non-causal". So we add timestamps to the probe
    firing, sort by that field, then strip it from the output"""

    # When compiling with '--with-pydebug', strip '[# refs]' debug output.
    output = re.sub(r"\[[0-9]+ refs\]", "", output)
    try:
        result = [
            row.split("\t")
            for row in output.splitlines()
            if row and not row.startswith('#') and not row.startswith('@')
        ]
        result.sort(key=lambda row: int(row[0]))
        result = [row[1] for row in result]
        # Normalize paths to basenames (bpftrace outputs full paths)
        normalized = []
        for line in result:
            # Replace full paths with just the filename
            line = re.sub(r'/[^:]+/([^/:]+\.py)', r'\1', line)
            normalized.append(line)
        return "\n".join(normalized)
    except (IndexError, ValueError):
        raise AssertionError(
            "tracer produced unparsable output:\n{}".format(output)
        )


class TraceBackend:
    EXTENSION = None
    COMMAND = None
    COMMAND_ARGS = []

    def run_case(self, name, optimize_python=None):
        actual_output = normalize_trace_output(self.trace_python(
            script_file=abspath(name + self.EXTENSION),
            python_file=abspath(name + ".py"),
            optimize_python=optimize_python))

        with open(abspath(name + self.EXTENSION + ".expected")) as f:
            expected_output = f.read().rstrip()

        return (expected_output, actual_output)

    def generate_trace_command(self, script_file, subcommand=None):
        command = self.COMMAND + [script_file]
        if subcommand:
            command += ["-c", subcommand]
        return command

    def trace(self, script_file, subcommand=None):
        command = self.generate_trace_command(script_file, subcommand)
        stdout, _ = subprocess.Popen(command,
                                     stdout=subprocess.PIPE,
                                     stderr=subprocess.STDOUT,
                                     universal_newlines=True).communicate()
        return stdout

    def trace_python(self, script_file, python_file, optimize_python=None):
        python_flags = []
        if optimize_python:
            python_flags.extend(["-O"] * optimize_python)
        subcommand = " ".join([sys.executable] + python_flags + [python_file])
        return self.trace(script_file, subcommand)

    def assert_usable(self):
        try:
            output = self.trace(abspath("assert_usable" + self.EXTENSION))
            output = output.strip()
        except (FileNotFoundError, NotADirectoryError, PermissionError) as fnfe:
            output = str(fnfe)
        if output != "probe: success":
            raise unittest.SkipTest(
                "{}(1) failed: {}".format(self.COMMAND[0], output)
            )


class DTraceBackend(TraceBackend):
    EXTENSION = ".d"
    COMMAND = ["dtrace", "-q", "-s"]


class SystemTapBackend(TraceBackend):
    EXTENSION = ".stp"
    COMMAND = ["stap", "-g"]


class BPFTraceBackend(TraceBackend):
    EXTENSION = ".bt"
    COMMAND = ["bpftrace"]

    # Inline bpftrace programs for each test case
    PROGRAMS = {
        "call_stack": """
            usdt:{python}:python:function__entry {{
                printf("%lld\\tfunction__entry:%s:%s:%d\\n",
                       nsecs, str(arg0), str(arg1), arg2);
            }}
            usdt:{python}:python:function__return {{
                printf("%lld\\tfunction__return:%s:%s:%d\\n",
                       nsecs, str(arg0), str(arg1), arg2);
            }}
        """,
        "gc": """
            usdt:{python}:python:function__entry {{
                if (str(arg1) == "start") {{ @tracing = 1; }}
            }}
            usdt:{python}:python:function__return {{
                if (str(arg1) == "start") {{ @tracing = 0; }}
            }}
            usdt:{python}:python:gc__start {{
                if (@tracing) {{
                    printf("%lld\\tgc__start:%d\\n", nsecs, arg0);
                }}
            }}
            usdt:{python}:python:gc__done {{
                if (@tracing) {{
                    printf("%lld\\tgc__done:%lld\\n", nsecs, arg0);
                }}
            }}
            END {{ clear(@tracing); }}
        """,
    }

    # Which test scripts to filter by filename (None = use @tracing flag)
    FILTER_BY_FILENAME = {"call_stack": "call_stack.py"}

    # Expected outputs for each test case
    # Note: bpftrace captures <module> entry/return and may have slight timing
    # differences compared to SystemTap due to probe firing order
    EXPECTED = {
        "call_stack": """function__entry:call_stack.py:<module>:0
function__entry:call_stack.py:start:23
function__entry:call_stack.py:function_1:1
function__entry:call_stack.py:function_3:9
function__return:call_stack.py:function_3:10
function__return:call_stack.py:function_1:2
function__entry:call_stack.py:function_2:5
function__entry:call_stack.py:function_1:1
function__return:call_stack.py:function_3:10
function__return:call_stack.py:function_1:2
function__return:call_stack.py:function_2:6
function__entry:call_stack.py:function_3:9
function__return:call_stack.py:function_3:10
function__entry:call_stack.py:function_4:13
function__return:call_stack.py:function_4:14
function__entry:call_stack.py:function_5:18
function__return:call_stack.py:function_5:21
function__return:call_stack.py:start:28
function__return:call_stack.py:<module>:30""",
        "gc": """gc__start:0
gc__done:0
gc__start:1
gc__done:0
gc__start:2
gc__done:0
gc__start:2
gc__done:1""",
    }

    def run_case(self, name, optimize_python=None):
        if name not in self.PROGRAMS:
            raise unittest.SkipTest(f"No bpftrace program for {name}")

        python_file = abspath(name + ".py")
        python_flags = []
        if optimize_python:
            python_flags.extend(["-O"] * optimize_python)

        subcommand = [sys.executable] + python_flags + [python_file]
        program = self.PROGRAMS[name].format(python=sys.executable)

        try:
            proc = subprocess.Popen(
                ["bpftrace", "-e", program, "-c", " ".join(subcommand)],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                universal_newlines=True,
            )
            stdout, stderr = proc.communicate(timeout=60)
        except subprocess.TimeoutExpired:
            proc.kill()
            raise AssertionError("bpftrace timed out")
        except (FileNotFoundError, PermissionError) as e:
            raise unittest.SkipTest(f"bpftrace not available: {e}")

        if proc.returncode != 0:
            raise AssertionError(
                f"bpftrace failed with code {proc.returncode}:\n{stderr}"
            )

        # Filter output by filename if specified (bpftrace captures everything)
        if name in self.FILTER_BY_FILENAME:
            filter_filename = self.FILTER_BY_FILENAME[name]
            filtered_lines = [
                line for line in stdout.splitlines()
                if filter_filename in line
            ]
            stdout = "\n".join(filtered_lines)

        actual_output = normalize_trace_output(stdout)
        expected_output = self.EXPECTED[name].strip()

        return (expected_output, actual_output)

    def assert_usable(self):
        # Check if bpftrace is available and can attach to USDT probes
        program = f'usdt:{sys.executable}:python:function__entry {{ printf("probe: success\\n"); exit(); }}'
        try:
            proc = subprocess.Popen(
                ["bpftrace", "-e", program, "-c", f"{sys.executable} -c pass"],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                universal_newlines=True,
            )
            stdout, stderr = proc.communicate(timeout=10)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.communicate()  # Clean up
            raise unittest.SkipTest("bpftrace timed out during usability check")
        except OSError as e:
            raise unittest.SkipTest(f"bpftrace not available: {e}")

        # Check for permission errors (bpftrace usually requires root)
        if proc.returncode != 0:
            raise unittest.SkipTest(
                f"bpftrace(1) failed with code {proc.returncode}: {stderr}"
            )

        if "probe: success" not in stdout:
            raise unittest.SkipTest(
                f"bpftrace(1) failed: stdout={stdout!r} stderr={stderr!r}"
            )




class TraceTests:
    # unittest.TestCase options
    maxDiff = None

    # TraceTests options
    backend = None
    optimize_python = 0

    @classmethod
    def setUpClass(self):
        self.backend.assert_usable()

    def run_case(self, name):
        actual_output, expected_output = self.backend.run_case(
            name, optimize_python=self.optimize_python)
        self.assertEqual(actual_output, expected_output)

    def test_function_entry_return(self):
        self.run_case("call_stack")

    def test_verify_call_opcodes(self):
        """Ensure our call stack test hits all function call opcodes"""

        # Modern Python uses CALL, CALL_KW, and CALL_FUNCTION_EX
        opcodes = set(["CALL", "CALL_FUNCTION_EX", "CALL_KW"])

        with open(abspath("call_stack.py")) as f:
            code_string = f.read()

        def get_function_instructions(funcname):
            # Recompile with appropriate optimization setting
            code = compile(source=code_string,
                           filename="<string>",
                           mode="exec",
                           optimize=self.optimize_python)

            for c in code.co_consts:
                if isinstance(c, types.CodeType) and c.co_name == funcname:
                    return dis.get_instructions(c)
            return []

        for instruction in get_function_instructions('start'):
            opcodes.discard(instruction.opname)

        self.assertEqual(set(), opcodes)

    def test_gc(self):
        self.run_case("gc")


class DTraceNormalTests(TraceTests, unittest.TestCase):
    backend = DTraceBackend()
    optimize_python = 0


class DTraceOptimizedTests(TraceTests, unittest.TestCase):
    backend = DTraceBackend()
    optimize_python = 2


class SystemTapNormalTests(TraceTests, unittest.TestCase):
    backend = SystemTapBackend()
    optimize_python = 0


class SystemTapOptimizedTests(TraceTests, unittest.TestCase):
    backend = SystemTapBackend()
    optimize_python = 2


class BPFTraceNormalTests(TraceTests, unittest.TestCase):
    backend = BPFTraceBackend()
    optimize_python = 0


class BPFTraceOptimizedTests(TraceTests, unittest.TestCase):
    backend = BPFTraceBackend()
    optimize_python = 2


class CheckDtraceProbes(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if sysconfig.get_config_var('WITH_DTRACE'):
            readelf_major_version, readelf_minor_version = cls.get_readelf_version()
            if support.verbose:
                print(f"readelf version: {readelf_major_version}.{readelf_minor_version}")
        else:
            raise unittest.SkipTest("CPython must be configured with the --with-dtrace option.")


    @staticmethod
    def get_readelf_version():
        try:
            cmd = ["readelf", "--version"]
            proc = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                universal_newlines=True,
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
            raise unittest.SkipTest("Couldn't find readelf on the path")

        # Regex to parse:
        # 'GNU readelf (GNU Binutils) 2.40.0\n' -> 2.40
        match = re.search(r"^(?:GNU) readelf.*?\b(\d+)\.(\d+)", version)
        if match is None:
            raise unittest.SkipTest(f"Unable to parse readelf version: {version}")

        return int(match.group(1)), int(match.group(2))

    def get_readelf_output(self):
        command = ["readelf", "-n", sys.executable]
        stdout, _ = subprocess.Popen(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            universal_newlines=True,
        ).communicate()
        return stdout

    def test_check_probes(self):
        readelf_output = self.get_readelf_output()

        available_probe_names = [
            "Name: import__find__load__done",
            "Name: import__find__load__start",
            "Name: audit",
            "Name: gc__start",
            "Name: gc__done",
            "Name: function__entry",
            "Name: function__return",
        ]

        for probe_name in available_probe_names:
            with self.subTest(probe_name=probe_name):
                self.assertIn(probe_name, readelf_output)

    @unittest.expectedFailure
    def test_missing_probes(self):
        readelf_output = self.get_readelf_output()

        # Missing probes will be added in the future.
        missing_probe_names = [
            "Name: line",
        ]

        for probe_name in missing_probe_names:
            with self.subTest(probe_name=probe_name):
                self.assertIn(probe_name, readelf_output)


if __name__ == '__main__':
    unittest.main()
