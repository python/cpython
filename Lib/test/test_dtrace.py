import dis
import os.path
import re
import shlex
import signal
import subprocess
import sys
import sysconfig
import tempfile
import types
import unittest

from test import support
from test.support import findfile, MS_WINDOWS
from test.support import os_helper


if not support.has_subprocess_support:
    raise unittest.SkipTest("test module requires subprocess")
if not sysconfig.get_config_var('WITH_DTRACE'):
    raise unittest.SkipTest(
        "CPython must be configured with the --with-dtrace option."
    )


def abspath(filename):
    return os.path.abspath(findfile(filename, subdir="dtracedata"))


def get_probe_binary():
    binary = sys.executable
    if sysconfig.get_config_var("Py_ENABLE_SHARED"):
        lib_dir = sysconfig.get_config_var("LIBDIR")
        if not lib_dir or sysconfig.is_python_build():
            lib_dir = os.path.abspath(os.path.dirname(sys.executable))

        lib_names = []
        for name in (
            sysconfig.get_config_var("INSTSONAME"),
            sysconfig.get_config_var("LDLIBRARY"),
        ):
            if name and name not in lib_names:
                lib_names.append(name)

        if lib_dir:
            for name in lib_names:
                libpython_path = os.path.join(lib_dir, name)
                if os.path.exists(libpython_path):
                    binary = libpython_path
                    break

    return binary


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


USE_PROCESS_GROUP = (hasattr(os, "setsid") and hasattr(os, "killpg"))
TERMINATE_TIMEOUT = 10

def create_process_group(*args, **kwargs):
    if USE_PROCESS_GROUP:
        kwargs['start_new_session'] = True
    return subprocess.Popen(*args, **kwargs)

def terminate_process_group(proc):
    if USE_PROCESS_GROUP:
        try:
            os.killpg(proc.pid, signal.SIGTERM)
        except ProcessLookupError:
            pass
    else:
        proc.terminate()

    try:
        proc.communicate(timeout=TERMINATE_TIMEOUT)
    except subprocess.TimeoutExpired:
        if USE_PROCESS_GROUP:
            try:
                os.killpg(proc.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
        else:
            proc.kill()
        proc.communicate(timeout=TERMINATE_TIMEOUT)  # Clean up


def run_command(cmd):
    # Force the C locale to disable localization.
    env = dict(os.environ, LC_ALL="C")
    try:
        proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            env=env,
        )
    except OSError:
        raise unittest.SkipTest(f"Couldn't find {cmd[0]!r} on the path")

    with proc:
        stdout, stderr = proc.communicate()

    if proc.returncode:
        raise AssertionError(
            f"Command {shlex.join(cmd)!r} failed "
            f"with exit code {proc.returncode}: "
            f"stdout={stdout!r} stderr={stderr!r}"
        )

    return stdout


class TraceBackend:
    EXTENSION = None
    COMMAND = None
    COMMAND_ARGS = []
    USABILITY_TIMEOUT = 10

    def run_case(self, name, optimize_python=None):
        try:
            actual_output = normalize_trace_output(self.trace_python(
                script_file=abspath(name + self.EXTENSION),
                python_file=abspath(name + ".py"),
                optimize_python=optimize_python))
        except subprocess.TimeoutExpired:
            raise AssertionError(f"{self.COMMAND[0]} timed out")

        with open(abspath(name + self.EXTENSION + ".expected")) as f:
            expected_output = f.read().rstrip()

        return (expected_output, actual_output)

    def generate_trace_command(self, script_file, subcommand=None):
        command = self.COMMAND + [script_file]
        if subcommand:
            command += ["-c", subcommand]
        return command

    def trace(self, script_file, subcommand=None, *, timeout=None,
              check_returncode=False):
        command = self.generate_trace_command(script_file, subcommand)
        proc = create_process_group(command,
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT,
                                    universal_newlines=True)
        try:
            stdout, _ = proc.communicate(timeout=timeout)
        except subprocess.TimeoutExpired:
            terminate_process_group(proc)
            raise
        if check_returncode and proc.returncode:
            raise AssertionError(
                f"Command {shlex.join(command)!r} failed "
                f"with exit code {proc.returncode}: output={stdout!r}"
            )
        return stdout

    def trace_python(self, script_file, python_file, optimize_python=None):
        python_flags = []
        if optimize_python:
            python_flags.extend(["-O"] * optimize_python)
        subcommand = shlex.join([sys.executable] + python_flags + [python_file])
        return self.trace(script_file, subcommand, timeout=60,
                          check_returncode=True)

    def assert_usable(self):
        try:
            output = self.trace(abspath("assert_usable" + self.EXTENSION),
                                timeout=self.USABILITY_TIMEOUT)
            output = output.strip()
        except subprocess.TimeoutExpired:
            raise unittest.SkipTest(
                f"{self.COMMAND[0]} timed out during usability check"
            )
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
    PROBE_PLACEHOLDER = "@PYTHON_SYSTEMTAP_PROBE@"
    USABILITY_TIMEOUT = 60

    @staticmethod
    def quote_systemtap_string(value):
        return value.replace("\\", "\\\\").replace('"', '\\"')

    def python_probe(self):
        executable = self.quote_systemtap_string(sys.executable)
        probe_binary = get_probe_binary()
        if probe_binary == sys.executable:
            return f'process("{executable}").mark'

        # Python built with --enable-shared
        probe_binary = self.quote_systemtap_string(probe_binary)
        return f'process("{executable}").library("{probe_binary}").mark'

    def render_script(self, filename):
        with open(filename) as fp:
            script = fp.read()

        return script.replace(self.PROBE_PLACEHOLDER, self.python_probe())

    def generate_trace_command(self, script_file, subcommand=None):
        probe_binary = get_probe_binary()
        if subcommand and probe_binary != sys.executable:
            library_path = os.path.dirname(probe_binary)
            if existing_path := os.environ.get("LD_LIBRARY_PATH"):
                library_path = os.pathsep.join((library_path, existing_path))
            env = shlex.join(["env", f"LD_LIBRARY_PATH={library_path}"])
            subcommand = f"{env} {subcommand}"

        return super().generate_trace_command(script_file, subcommand)

    def trace(self, script_file, subcommand=None, *, timeout=None,
              check_returncode=False):
        with tempfile.NamedTemporaryFile(
            mode="w", encoding="utf-8", suffix=self.EXTENSION, delete=False
        ) as script:
            script.write(self.render_script(script_file))
            generated_script_file = script.name

        try:
            return super().trace(
                generated_script_file,
                subcommand,
                timeout=timeout,
                check_returncode=check_returncode,
            )
        finally:
            os_helper.unlink(generated_script_file)


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
        if sys.platform == "sunos5":
            # Solaris does not expose available probes through readable ELF
            # section; it uses DTrace itself instead.
            DTraceBackend().assert_usable()
            return

        readelf_major_version, readelf_minor_version = cls.get_readelf_version()
        if support.verbose:
            print(f"readelf version: {readelf_major_version}.{readelf_minor_version}")


    @staticmethod
    def get_readelf_version():
        version = run_command(["readelf", "--version"])

        # Regex to parse:
        # 'GNU readelf (GNU Binutils) 2.40.0\n' -> 2.40
        match = re.search(r"^(?:GNU) readelf.*?\b(\d+)\.(\d+)", version)
        if match is None:
            raise unittest.SkipTest(f"Unable to parse readelf version: {version}")

        return int(match.group(1)), int(match.group(2))

    def get_probe_output(self):
        if sys.platform == "sunos5":
            # Get probes available in this process.
            return run_command(
                ["dtrace", "-l", "-P", f"python{os.getpid()}"]
            )

        return run_command(["readelf", "-n", get_probe_binary()])

    def assert_probe_present(self, probe_name, output):
        if sys.platform == "sunos5":
            self.assertIn(probe_name.replace("__", "-"), output)
        else:
            self.assertIn(f"Name: {probe_name}", output)

    def test_check_probes(self):
        probe_output = self.get_probe_output()

        available_probe_names = [
            "import__find__load__done",
            "import__find__load__start",
            "audit",
            "gc__start",
            "gc__done",
        ]

        for probe_name in available_probe_names:
            with self.subTest(probe_name=probe_name):
                self.assert_probe_present(probe_name, probe_output)

    @unittest.expectedFailure
    def test_missing_probes(self):
        probe_output = self.get_probe_output()

        # Missing probes will be added in the future.
        missing_probe_names = [
            "function__entry",
            "function__return",
            "line",
        ]

        for probe_name in missing_probe_names:
            with self.subTest(probe_name=probe_name):
                self.assert_probe_present(probe_name, probe_output)


if __name__ == '__main__':
    unittest.main()
