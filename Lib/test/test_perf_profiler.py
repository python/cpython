import glob
import struct
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
from test.support import import_helper
from test.support.os_helper import temp_dir
from test import test_tools


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


def _perf_env(**env_vars):
    env = os.environ.copy()
    # Keep perf's output stable regardless of the builder's perf config.
    env.update(
        {
            "DEBUGINFOD_URLS": "",
            "PERF_CONFIG": os.devnull,
        }
    )
    if env_vars:
        env.update(env_vars)
    env["PYTHON_JIT"] = "0"
    return env


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
            with subprocess.Popen(
                [sys.executable, "-Xperf", script],
                text=True,
                stderr=subprocess.PIPE,
                stdout=subprocess.PIPE,
                env=_perf_env(),
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
            self.assertNotStartsWith(perf_addr, "0x")
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
            with subprocess.Popen(
                [sys.executable, "-Xperf", script],
                text=True,
                stderr=subprocess.PIPE,
                stdout=subprocess.PIPE,
                env=_perf_env(),
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

        # The parent's map should not contain the child's symbols.
        self.assertNotIn(f"py::foo_fork:{script}", perf_file_contents)
        self.assertNotIn(f"py::bar_fork:{script}", perf_file_contents)
        self.assertNotIn(f"py::baz_fork:{script}", perf_file_contents)

        # The child's map should not contain the parent's symbols.
        self.assertNotIn(f"py::foo:{script}", child_perf_file_contents)
        self.assertNotIn(f"py::bar:{script}", child_perf_file_contents)
        self.assertNotIn(f"py::baz:{script}", child_perf_file_contents)

    @unittest.skipIf(support.check_bolt_optimized(), "fails on BOLT instrumented binaries")
    def test_trampoline_works_after_fork_with_many_code_objects(self):
        code = """if 1:
                import gc, os, sys, signal

                # Create many code objects so trampoline_refcount > 1
                for i in range(50):
                    exec(compile(f"def _dummy_{i}(): pass", f"<test{i}>", "exec"))

                pid = os.fork()
                if pid == 0:
                    # Child: create and destroy new code objects,
                    # then collect garbage. If the old code watcher
                    # survived the fork, the double-decrement of
                    # trampoline_refcount will cause a SIGSEGV.
                    for i in range(50):
                        exec(compile(f"def _child_{i}(): pass", f"<child{i}>", "exec"))
                    gc.collect()
                    os._exit(0)
                else:
                    _, status = os.waitpid(pid, 0)
                    if os.WIFSIGNALED(status):
                        print(f"FAIL: child killed by signal {os.WTERMSIG(status)}", file=sys.stderr)
                        sys.exit(1)
                    sys.exit(os.WEXITSTATUS(status))
                """
        with temp_dir() as script_dir:
            script = make_script(script_dir, "perftest", code)
            with subprocess.Popen(
                [sys.executable, "-Xperf", script],
                text=True,
                stderr=subprocess.PIPE,
                stdout=subprocess.PIPE,
                env=_perf_env(),
            ) as process:
                stdout, stderr = process.communicate()

        self.assertEqual(process.returncode, 0, stderr)
        self.assertEqual(stderr, "")

    @unittest.skipIf(support.check_bolt_optimized(), "fails on BOLT instrumented binaries")
    def test_sys_api(self):
        for define_eval_hook in (False, True):
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
            if define_eval_hook:
                set_eval_hook = """if 1:
                                import _testinternalcapi
                                _testinternalcapi.set_eval_frame_record([])
"""
                code = set_eval_hook + code
            with temp_dir() as script_dir:
                script = make_script(script_dir, "perftest", code)
                with subprocess.Popen(
                    [sys.executable, script],
                    text=True,
                    stderr=subprocess.PIPE,
                    stdout=subprocess.PIPE,
                    env=_perf_env(),
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

    def test_sys_api_perf_jit_backend(self):
        code = """if 1:
                import sys
                sys.activate_stack_trampoline("perf_jit")
                assert sys.is_stack_trampoline_active() is True
                sys.deactivate_stack_trampoline()
                assert sys.is_stack_trampoline_active() is False
                """
        assert_python_ok("-c", code, PYTHON_JIT="0")

    def test_sys_api_with_existing_perf_jit_trampoline(self):
        code = """if 1:
                import sys
                sys.activate_stack_trampoline("perf_jit")
                sys.activate_stack_trampoline("perf_jit")
                """
        assert_python_ok("-c", code, PYTHON_JIT="0")

    @unittest.skipUnless(
        "-D_Py_JIT" in (sysconfig.get_config_var("PY_CORE_CFLAGS") or "").split(),
        "requires a real JIT (_Py_JIT)",
    )
    def test_sys_api_perf_jit_backend_in_subinterpreter(self):
        # gh-157247: a subinterpreter must not bypass the JIT/perf exclusion.
        code = """if 1:
                import sys
                from contextlib import closing
                from concurrent import interpreters

                assert sys._jit.is_enabled(), "expected the JIT to be enabled"

                with closing(interpreters.create()) as interp:
                    interp.exec(
                        "import sys; sys.activate_stack_trampoline('perf_jit')")
                """
        rc, out, err = assert_python_failure("-c", code, PYTHON_JIT="1")
        self.assertIn(
            b"Cannot activate the perf trampoline if the JIT is active", err)


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
            stdout = subprocess.check_output(
                cmd,
                cwd=script_dir,
                text=True,
                stderr=subprocess.STDOUT,
                env=_perf_env(),
            )
        except (subprocess.SubprocessError, OSError):
            return False

        if "hello" not in stdout:
            return False

    return True


def run_perf(cwd, *args, use_jit=False, **env_vars):
    env = _perf_env(**env_vars)
    output_file = cwd + "/perf_output.perf"
    base_cmd = [
        "perf",
        "record",
        "--no-buildid",
        "--no-buildid-cache",
        "-g",
        "--call-graph=dwarf,65528" if use_jit else "--call-graph=fp",
    ]
    if use_jit:
        perf_commands = []
        # Some builders have low perf_event_mlock_kb limits.
        mmap_sizes = ("4M", "2M", "1M", "512K", "256K", "128K", None)
        for mmap_size in mmap_sizes:
            command = base_cmd.copy()
            if mmap_size is not None:
                command += ["-F99", "-k1", "-m", mmap_size]
            else:
                command += ["-F99", "-k1"]
            command += ["-o", output_file, "--"]
            perf_commands.append(command)
    else:
        perf_commands = [base_cmd + ["-o", output_file, "--"]]

    mmap_pages_error = "try again with a smaller value of -m/--mmap_pages"
    for index, base_cmd in enumerate(perf_commands):
        proc = subprocess.run(
            base_cmd + list(args),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            env=env,
            text=True,
        )
        if (
            proc.returncode
            and use_jit
            and index != len(perf_commands) - 1
            and mmap_pages_error in proc.stderr
        ):
            continue
        break

    if proc.returncode:
        print(proc.stderr, file=sys.stderr)
        raise ValueError(f"Perf failed with return code {proc.returncode}")

    if use_jit:
        jit_output_file = cwd + "/jit_output.dump"
        command = ("perf", "inject", "-j", "-i", output_file, "-o", jit_output_file)
        proc = subprocess.run(
            command, stderr=subprocess.PIPE, stdout=subprocess.PIPE, env=env, text=True
        )
        if proc.returncode:
            print(proc.stderr, file=sys.stderr)
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
        text=True,
    )
    return proc.stdout, proc.stderr


class TestPerfProfilerMixin:
    PERF_CAPTURE_ATTEMPTS = 3

    def run_perf(self, script_dir, script, activate_trampoline=True):
        raise NotImplementedError()

    def run_perf_with_retries(
        self, script_dir, script, expected_symbols=(), activate_trampoline=True
    ):
        stdout = stderr = ""
        for _ in range(self.PERF_CAPTURE_ATTEMPTS):
            stdout, stderr = self.run_perf(
                script_dir, script, activate_trampoline=activate_trampoline
            )
            if activate_trampoline and any(
                symbol not in stdout for symbol in expected_symbols
            ):
                continue
            break
        return stdout, stderr

    def test_python_calls_appear_in_the_stack_if_perf_activated(self):
        with temp_dir() as script_dir:
            code = """if 1:
                from itertools import repeat

                def foo(n):
                    for _ in repeat(None, n):
                        pass

                def bar(n):
                    foo(n)

                def baz(n):
                    bar(n)

                baz(40000000)
                """
            script = make_script(script_dir, "perftest", code)
            expected_symbols = [
                f"py::foo:{script}",
                f"py::bar:{script}",
                f"py::baz:{script}",
            ]
            stdout, _ = self.run_perf_with_retries(
                script_dir, script, expected_symbols
            )

            for expected_symbol in expected_symbols:
                self.assertIn(expected_symbol, stdout)

    def test_python_calls_do_not_appear_in_the_stack_if_perf_deactivated(self):
        with temp_dir() as script_dir:
            code = """if 1:
                from itertools import repeat

                def foo(n):
                    for _ in repeat(None, n):
                        pass

                def bar(n):
                    foo(n)

                def baz(n):
                    bar(n)

                baz(40000000)
                """
            script = make_script(script_dir, "perftest", code)
            stdout, _ = self.run_perf_with_retries(
                script_dir, script, activate_trampoline=False
            )

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
            with subprocess.Popen(
                [sys.executable, "-Xperf", script],
                universal_newlines=True,
                stderr=subprocess.PIPE,
                stdout=subprocess.PIPE,
                env=_perf_env(),
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
    #
    # PermissionError is raised if perf does not exist on the Windows Subsystem
    # for Linux, see #134987
    try:
        output = subprocess.check_output(["perf", "--version"], text=True)
    except (subprocess.CalledProcessError, FileNotFoundError, PermissionError):
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


JITDUMP_MAGIC = 0x4A695444  # "JiTD"
JITDUMP_VERSION = 1
PERF_LOAD = 0
PERF_UNWINDING_INFO = 4
JITDUMP_ENDIAN = "<" if sys.byteorder == "little" else ">"
# Every jitdump record starts with event(u32), size(u32), timestamp(u64).
JITDUMP_RECORD_HEADER_SIZE = 16
# CodeLoadEvent: record header, pid(u32), tid(u32), vma(u64), code_addr(u64),
# code_size(u64), code_id(u64), then the NUL-terminated name.
CODE_LOAD_CODE_SIZE_OFFSET = JITDUMP_RECORD_HEADER_SIZE + 4 + 4 + 8 + 8
CODE_LOAD_NAME_OFFSET = CODE_LOAD_CODE_SIZE_OFFSET + 8 + 8
# CodeUnwindingInfoEvent: record header, unwind_data_size(u64),
# eh_frame_hdr_size(u64), mapped_size(u64), then the .eh_frame bytes followed
# by perf's 20-byte eh_frame_hdr (EhFrameHeader in perf_jit_trampoline.c),
# whose "from" field is the signed distance back to the code.
UNWIND_DATA_SIZE_OFFSET = JITDUMP_RECORD_HEADER_SIZE
UNWIND_EH_FRAME_OFFSET = JITDUMP_RECORD_HEADER_SIZE + 3 * 8
EH_FRAME_HDR_SIZE = 20
EH_FRAME_HDR_FROM_OFFSET = 12
# DWARF FDE pointer encodings: DW_EH_PE_pcrel | DW_EH_PE_sdata4 (ELF
# assemblers) and DW_EH_PE_pcrel | DW_EH_PE_absptr (Darwin assemblers).
DW_EH_PE_PCREL_SDATA4 = 0x1B
DW_EH_PE_PCREL_ABSPTR = 0x10


def _jitdump_records(data):
    """Yield (event, offset, size) for each record of a jitdump file."""
    header_size = struct.unpack_from(f"{JITDUMP_ENDIAN}I", data, 8)[0]
    pos = header_size
    while pos < len(data):
        if pos + JITDUMP_RECORD_HEADER_SIZE > len(data):
            raise ValueError(f"truncated record header at offset {pos}")
        event, size = struct.unpack_from(f"{JITDUMP_ENDIAN}II", data, pos)
        if size < JITDUMP_RECORD_HEADER_SIZE or pos + size > len(data):
            raise ValueError(f"record at offset {pos} has a bad size {size}")
        yield event, pos, size
        pos += size


def _fde_pointer_encoding(eh_frame):
    """Return the FDE pointer encoding byte of a version 1 "zR" CIE."""
    cie_length = struct.unpack_from(f"{JITDUMP_ENDIAN}I", eh_frame, 0)[0]
    cie_total = 4 + cie_length
    pos = 12  # past length, CIE_id, version and "zR\0"
    for _ in range(2):  # code and data alignment factors (LEB128)
        while eh_frame[pos] & 0x80:
            pos += 1
        pos += 1
    pos += 1  # return address column
    while eh_frame[pos] & 0x80:  # augmentation data length (LEB128)
        pos += 1
    pos += 1
    if pos >= cie_total:
        raise ValueError("truncated CIE augmentation data")
    return eh_frame[pos]


class TestJitdumpFileFormat(unittest.TestCase):
    """Validate the jitdump written by -Xperf_jit without requiring perf."""

    def _run_and_get_jitdump(self, code):
        # The child prints its pid so we open exactly its own jitdump file
        # rather than whatever another test worker left in /tmp.
        code = "import os, sys\nsys.stdout.write(str(os.getpid()))\n" + code
        rc, out, err = assert_python_ok("-Xperf_jit", "-c", code, PYTHON_JIT="0")
        path = f"/tmp/jit-{int(out)}.dump"
        if not os.path.exists(path):
            # perf_map_jit_init() gives up silently when it cannot create
            # the file (for example an unwritable /tmp).
            self.skipTest("jitdump file was not created")
        self.addCleanup(os.unlink, path)
        with open(path, "rb") as f:
            data = f.read()
        if not data:
            # The file is created before the executable mapping of the
            # jitdump; if that mapping fails (for example a noexec /tmp) the
            # backend gives up silently and never writes the header.
            self.skipTest("jitdump could not be initialized")
        return data

    def _check_unwinding_records(self, data):
        """Check every unwinding record against the code load record it
        describes; return {name: code_size} for the regions seen."""
        records = list(_jitdump_records(data))
        regions = {}
        for index, (event, pos, size) in enumerate(records):
            if event != PERF_UNWINDING_INFO:
                continue
            # The unwinding info record is immediately followed by the
            # code load record it describes.
            self.assertLess(index + 1, len(records))
            load_event, load_pos, load_size = records[index + 1]
            self.assertEqual(load_event, PERF_LOAD)
            code_size = struct.unpack_from(
                f"{JITDUMP_ENDIAN}Q", data, load_pos + CODE_LOAD_CODE_SIZE_OFFSET)[0]
            name_start = load_pos + CODE_LOAD_NAME_OFFSET
            name_end = data.find(b"\x00", name_start, load_pos + load_size)
            self.assertGreater(name_end, 0)
            name = data[name_start:name_end].decode("utf-8", errors="replace")
            # The machine code follows the name inside the load record.
            self.assertLessEqual(name_end + 1 + code_size, load_pos + load_size, name)
            unwind_data_size, eh_frame_hdr_size = struct.unpack_from(
                f"{JITDUMP_ENDIAN}QQ", data, pos + UNWIND_DATA_SIZE_OFFSET)
            self.assertEqual(eh_frame_hdr_size, EH_FRAME_HDR_SIZE)
            self.assertLessEqual(UNWIND_EH_FRAME_OFFSET + unwind_data_size, size)
            eh_frame_size = unwind_data_size - eh_frame_hdr_size
            self.assertGreater(eh_frame_size, 0)
            start = pos + UNWIND_EH_FRAME_OFFSET
            eh_frame = data[start:start + eh_frame_size]

            cie_length, cie_id = struct.unpack_from(f"{JITDUMP_ENDIAN}II", eh_frame, 0)
            self.assertEqual(cie_id, 0, "first entry must be a CIE")
            self.assertEqual(eh_frame[8], 1, "CIE version must be 1")
            self.assertEqual(eh_frame[9:12], b"zR\x00")
            encoding = _fde_pointer_encoding(eh_frame)
            if encoding == DW_EH_PE_PCREL_SDATA4:
                fields = "iI"
            elif encoding == DW_EH_PE_PCREL_ABSPTR:
                fields = "qQ"
            else:
                self.fail(f"unexpected FDE pointer encoding {encoding:#x}")
            # jit_unwind.c patches initial_location and address_range for
            # perf's DSO layout, where the .eh_frame follows the code at
            # code_size rounded up to 8 bytes.
            pc_offset = 4 + cie_length + 8
            initial_location, address_range = struct.unpack_from(
                f"{JITDUMP_ENDIAN}{fields}", eh_frame, pc_offset)
            self.assertEqual(address_range, code_size, name)
            rounded_code_size = (code_size + 7) & ~7
            self.assertEqual(initial_location, -(rounded_code_size + pc_offset), name)
            # perf's eh_frame_hdr must point back at the code with the same
            # rounding as the FDE.
            hdr_from = struct.unpack_from(
                f"{JITDUMP_ENDIAN}i", data,
                start + eh_frame_size + EH_FRAME_HDR_FROM_OFFSET)[0]
            self.assertEqual(hdr_from, -(rounded_code_size + eh_frame_size), name)
            regions[name] = code_size
        self.assertTrue(regions, "no CodeUnwindingInfoEvent found")
        return regions

    def test_jitdump_unwinding_info(self):
        """Each region's .eh_frame is patched for that region's size."""
        data = self._run_and_get_jitdump("def my_test_func(): pass\nmy_test_func()")
        magic, version = struct.unpack_from(f"{JITDUMP_ENDIAN}II", data, 0)
        self.assertEqual((magic, version), (JITDUMP_MAGIC, JITDUMP_VERSION))
        regions = self._check_unwinding_records(data)
        self.assertTrue(any("my_test_func" in name for name in regions))


try:
    with test_tools.imports_under_tool("jit"):
        import _trampoline_ehframe
except ImportError:
    # Installed Python without the Tools directory.
    _trampoline_ehframe = None


def _fake_cie(*, version=1, augmentation=b"zR", ra_column=16,
              encoding=DW_EH_PE_PCREL_SDATA4, cie_id=0):
    """A CIE like the assembler's: code align 1, data align -8, one
    DW_CFA_def_cfa instruction, padded with DW_CFA_nop to 8 bytes."""
    body = bytes([version]) + augmentation + b"\x00"
    body += bytes([1, 0x78, ra_column, 1, encoding])
    body += bytes([0x0C, 7, 8])  # DW_CFA_def_cfa: r7 (rsp) ofs 8
    body += b"\x00" * (-(8 + len(body)) % 8)
    return struct.pack("<II", 4 + len(body), cie_id) + body


def _fake_fde(cie_total, *, field_size=4, address_range=8,
              instructions=b"\x41\x0e\x10\x86\x02"):
    """An FDE right after a CIE of cie_total bytes, padded to 8 bytes."""
    body = struct.pack("<I", cie_total + 4)  # CIE pointer, relative to itself
    # initial_location as an assembler would leave it, the parser zeroes it.
    body += (-40).to_bytes(field_size, "little", signed=True)
    body += address_range.to_bytes(field_size, "little")
    body += b"\x00"  # augmentation data length
    body += instructions
    body += b"\x00" * (-(4 + len(body)) % 8)
    return struct.pack("<I", len(body)) + body


@unittest.skipIf(_trampoline_ehframe is None,
                 "Tools/jit/_trampoline_ehframe.py not found")
class TestTrampolineEhframeScript(unittest.TestCase):
    """Tests for Tools/jit/_trampoline_ehframe.py."""

    ehframe = _trampoline_ehframe

    def parse(self, data, text_size=8):
        return self.ehframe.parse_ehframe(bytes(data), "<", text_size)

    def test_parse(self):
        """Both FDE pointer encodings: ELF sdata4 and Darwin absptr."""
        cases = [(DW_EH_PE_PCREL_SDATA4, 4, 16, 8), (DW_EH_PE_PCREL_ABSPTR, 8, 30, 20)]
        for encoding, field_size, ra_column, text_size in cases:
            with self.subTest(encoding=hex(encoding)):
                cie = _fake_cie(encoding=encoding, ra_column=ra_column)
                fde = _fake_fde(len(cie), field_size=field_size,
                                address_range=text_size)
                result = self.parse(cie + fde, text_size)
                self.assertEqual(result.field_size, field_size)
                self.assertEqual(result.fde_pc_offset, len(cie) + 8)
                self.assertEqual(result.fde_range_offset, len(cie) + 8 + field_size)
                # Both patchable fields zeroed, everything else untouched.
                expected = bytearray(cie + fde)
                expected[len(cie) + 8:len(cie) + 8 + 2 * field_size] = bytes(2 * field_size)
                self.assertEqual(result.data, bytes(expected))

    def test_parse_rejects_malformed(self):
        cie = _fake_cie()
        fde = _fake_fde(len(cie))
        cases = [
            ("version", _fake_cie(version=3) + fde, 8),
            ("augmentation", _fake_cie(augmentation=b"zPLR") + fde, 8),
            ("encoding", _fake_cie(encoding=0x1A) + fde, 8),
            ("exactly one FDE", cie + fde + fde, 8),
            ("address_range", cie + fde, 12),
            ("no FDE", cie, 8),
        ]
        for message, data, text_size in cases:
            with self.subTest(message):
                with self.assertRaisesRegex(ValueError, message):
                    self.parse(data, text_size)

    def _build_trampoline_objects(self):
        """The object(s) the Makefile fed to the generator."""
        builddir = sysconfig.get_config_var("abs_builddir") or "."
        universal2 = os.path.join(builddir, "Python", "asm_trampoline_universal2.o")
        if os.path.exists(universal2):
            return [universal2]
        return sorted(
            path for path in glob.glob(
                os.path.join(builddir, "Python", "asm_trampoline_*.o"))
            if "apple-darwin" not in os.path.basename(path))

    def test_macho_thin_and_fat(self):
        """Mach-O objects and fat containers are parsed with no external tools."""
        E = self.ehframe

        def macho(cputype, text, eh_frame):
            # A minimal MH_OBJECT: one __TEXT segment with __text and
            # __eh_frame sections, section data right after the load command.
            segment_size = 72 + 2 * 80
            text_offset = 32 + segment_size
            eh_offset = text_offset + len(text)
            sections = b""
            for name, size, offset in (("__text", len(text), text_offset),
                                       ("__eh_frame", len(eh_frame), eh_offset)):
                sections += struct.pack("<16s16sQQIIIIIIII", name.encode(),
                                        b"__TEXT", 0, size, offset,
                                        0, 0, 0, 0, 0, 0, 0)
            segment = struct.pack("<II16sQQQQIIII", E._LC_SEGMENT_64,
                                  segment_size, b"__TEXT", 0,
                                  len(text) + len(eh_frame), text_offset,
                                  len(text) + len(eh_frame), 7, 5, 2, 0)
            header = struct.pack("<IIIIIIII", E._MH_MAGIC_64, cputype, 0,
                                 1, 1, segment_size, 0, 0)
            return header + segment + sections + text + eh_frame

        x86 = macho(E._CPU_TYPE_X86_64, b"\x55\xc3", b"x86 eh_frame")
        arm = macho(E._CPU_TYPE_ARM64, b"\xc0\x03\x5f\xd6", b"arm64 eh_frame")
        # The fat header and its fat_arch entries are big-endian.
        blobs = [(E._CPU_TYPE_X86_64, x86), (E._CPU_TYPE_ARM64, arm)]
        offset = 8 + 20 * len(blobs)
        entries = b""
        body = b""
        for cputype, blob in blobs:
            entries += struct.pack(">IIIII", cputype, 0, offset + len(body),
                                   len(blob), 0)
            body += blob
        fat = struct.pack(">II", E._FAT_MAGIC, len(blobs)) + entries + body

        with temp_dir() as tmp:
            thin_path = os.path.join(tmp, "thin.o")
            fat_path = os.path.join(tmp, "fat.o")
            with open(thin_path, "wb") as f:
                f.write(arm)
            with open(fat_path, "wb") as f:
                f.write(fat)
            (thin,) = E.load_object(thin_path)
            fat_slices = E.load_object(fat_path)

        self.assertEqual(thin.arch_macro, "__aarch64__")
        self.assertEqual(thin.sections[".text"], b"\xc0\x03\x5f\xd6")
        self.assertEqual(thin.sections[".eh_frame"], b"arm64 eh_frame")
        self.assertEqual([s.arch_macro for s in fat_slices],
                         ["__x86_64__", "__aarch64__"])
        self.assertEqual(fat_slices[0].sections[".eh_frame"], b"x86 eh_frame")
        self.assertEqual(fat_slices[1].sections[".text"], b"\xc0\x03\x5f\xd6")

    def test_generated_header_is_current(self):
        """The header in the build directory matches a fresh generation."""
        objects = self._build_trampoline_objects()
        builddir = sysconfig.get_config_var("abs_builddir") or "."
        header = os.path.join(builddir, "trampoline_ehframe.h")
        if not objects or not os.path.exists(header):
            self.skipTest("trampoline object or generated header not found")
        with open(header) as f:
            current = f.read()
        with temp_dir() as tmp:
            fresh_path = os.path.join(tmp, "trampoline_ehframe.h")
            self.ehframe.generate(objects, fresh_path)
            with open(fresh_path) as f:
                fresh = f.read()
        self.assertEqual(current, fresh)


class TestTrampolineEhframeHeader(unittest.TestCase):
    """Structural checks on the generated trampoline_ehframe.h data."""

    def test_generated_header_structure(self):
        _testinternalcapi = import_helper.import_module("_testinternalcapi")
        check = getattr(_testinternalcapi, "test_trampoline_ehframe", None)
        if check is None:
            self.skipTest("_testinternalcapi built without the perf trampoline")
        # Raises AssertionError describing the first failed check.
        check()


if __name__ == "__main__":
    unittest.main()
