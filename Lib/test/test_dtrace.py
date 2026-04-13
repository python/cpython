import dis
import os.path
import re
import subprocess
import sys
import sysconfig
import types
import unittest

from test import support
from test.support import findfile, MS_WINDOWS


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
            if row and not row.startswith('#')
        ]
        result.sort(key=lambda row: int(row[0]))
        result = [row[1] for row in result]
        return "\n".join(result)
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


@unittest.skipIf(MS_WINDOWS, "Tests not compliant with trace on Windows.")
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

        opcodes = set(["CALL_FUNCTION", "CALL_FUNCTION_EX", "CALL_FUNCTION_KW"])

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

    def test_line(self):
        self.run_case("line")


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
        ]

        for probe_name in available_probe_names:
            with self.subTest(probe_name=probe_name):
                self.assertIn(probe_name, readelf_output)

    @unittest.expectedFailure
    def test_missing_probes(self):
        readelf_output = self.get_readelf_output()

        # Missing probes will be added in the future.
        missing_probe_names = [
            "Name: function__entry",
            "Name: function__return",
            "Name: line",
        ]

        for probe_name in missing_probe_names:
            with self.subTest(probe_name=probe_name):
                self.assertIn(probe_name, readelf_output)


if __name__ == '__main__':
    unittest.main()
