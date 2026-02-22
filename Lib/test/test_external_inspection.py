import unittest
import os
import textwrap
import contextlib
import importlib
import sys
import socket
import threading
import time
from contextlib import contextmanager
from asyncio import staggered, taskgroups, base_events, tasks
from unittest.mock import ANY
from test.support import (
    os_helper,
    SHORT_TIMEOUT,
    busy_retry,
    requires_gil_enabled,
    requires_remote_subprocess_debugging,
)
from test.support.script_helper import make_script
from test.support.socket_helper import find_unused_port

import subprocess

# Profiling mode constants
PROFILING_MODE_WALL = 0
PROFILING_MODE_CPU = 1
PROFILING_MODE_GIL = 2
PROFILING_MODE_ALL = 3
PROFILING_MODE_EXCEPTION = 4

# Thread status flags
THREAD_STATUS_HAS_GIL = 1 << 0
THREAD_STATUS_ON_CPU = 1 << 1
THREAD_STATUS_UNKNOWN = 1 << 2
THREAD_STATUS_HAS_EXCEPTION = 1 << 4

# Maximum number of retry attempts for operations that may fail transiently
MAX_TRIES = 10
RETRY_DELAY = 0.1

# Exceptions that can occur transiently when reading from a live process
TRANSIENT_ERRORS = (OSError, RuntimeError, UnicodeDecodeError)

try:
    from concurrent import interpreters
except ImportError:
    interpreters = None

PROCESS_VM_READV_SUPPORTED = False

try:
    from _remote_debugging import PROCESS_VM_READV_SUPPORTED
    from _remote_debugging import RemoteUnwinder
    from _remote_debugging import FrameInfo, CoroInfo, TaskInfo
except ImportError:
    raise unittest.SkipTest(
        "Test only runs when _remote_debugging is available"
    )


# ============================================================================
# Module-level helper functions
# ============================================================================


def _make_test_script(script_dir, script_basename, source):
    to_return = make_script(script_dir, script_basename, source)
    importlib.invalidate_caches()
    return to_return


def _create_server_socket(port, backlog=1):
    """Create and configure a server socket for test communication."""
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind(("localhost", port))
    server_socket.settimeout(SHORT_TIMEOUT)
    server_socket.listen(backlog)
    return server_socket


def _wait_for_signal(sock, expected_signals, timeout=SHORT_TIMEOUT):
    """
    Wait for expected signal(s) from a socket with proper timeout and EOF handling.

    Args:
        sock: Connected socket to read from
        expected_signals: Single bytes object or list of bytes objects to wait for
        timeout: Socket timeout in seconds

    Returns:
        bytes: Complete accumulated response buffer

    Raises:
        RuntimeError: If connection closed before signal received or timeout
    """
    if isinstance(expected_signals, bytes):
        expected_signals = [expected_signals]

    sock.settimeout(timeout)
    buffer = b""

    while True:
        # Check if all expected signals are in buffer
        if all(sig in buffer for sig in expected_signals):
            return buffer

        try:
            chunk = sock.recv(4096)
            if not chunk:
                # EOF - connection closed
                raise RuntimeError(
                    f"Connection closed before receiving expected signals. "
                    f"Expected: {expected_signals}, Got: {buffer[-200:]!r}"
                )
            buffer += chunk
        except socket.timeout:
            raise RuntimeError(
                f"Timeout waiting for signals. "
                f"Expected: {expected_signals}, Got: {buffer[-200:]!r}"
            )


def _wait_for_n_signals(sock, signal_pattern, count, timeout=SHORT_TIMEOUT):
    """
    Wait for N occurrences of a signal pattern.

    Args:
        sock: Connected socket to read from
        signal_pattern: bytes pattern to count (e.g., b"ready")
        count: Number of occurrences expected
        timeout: Socket timeout in seconds

    Returns:
        bytes: Complete accumulated response buffer

    Raises:
        RuntimeError: If connection closed or timeout before receiving all signals
    """
    sock.settimeout(timeout)
    buffer = b""
    found_count = 0

    while found_count < count:
        try:
            chunk = sock.recv(4096)
            if not chunk:
                raise RuntimeError(
                    f"Connection closed after {found_count}/{count} signals. "
                    f"Last 200 bytes: {buffer[-200:]!r}"
                )
            buffer += chunk
            # Count occurrences in entire buffer
            found_count = buffer.count(signal_pattern)
        except socket.timeout:
            raise RuntimeError(
                f"Timeout waiting for {count} signals (found {found_count}). "
                f"Last 200 bytes: {buffer[-200:]!r}"
            )

    return buffer


@contextmanager
def _managed_subprocess(args, timeout=SHORT_TIMEOUT):
    """
    Context manager for subprocess lifecycle management.

    Ensures process is properly terminated and cleaned up even on exceptions.
    Uses graceful termination first, then forceful kill if needed.
    """
    p = subprocess.Popen(args)
    try:
        yield p
    finally:
        try:
            p.terminate()
            try:
                p.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                p.kill()
                try:
                    p.wait(timeout=timeout)
                except subprocess.TimeoutExpired:
                    pass  # Process refuses to die, nothing more we can do
        except OSError:
            pass  # Process already dead


def _cleanup_sockets(*sockets):
    """Safely close multiple sockets, ignoring errors."""
    for sock in sockets:
        if sock is not None:
            try:
                sock.close()
            except OSError:
                pass


# ============================================================================
# Decorators and skip conditions
# ============================================================================

skip_if_not_supported = unittest.skipIf(
    (
        sys.platform != "darwin"
        and sys.platform != "linux"
        and sys.platform != "win32"
    ),
    "Test only runs on Linux, Windows and MacOS",
)


def requires_subinterpreters(meth):
    """Decorator to skip a test if subinterpreters are not supported."""
    return unittest.skipIf(interpreters is None, "subinterpreters required")(
        meth
    )


# ============================================================================
# Simple wrapper functions for RemoteUnwinder
# ============================================================================

def get_stack_trace(pid):
    for _ in busy_retry(SHORT_TIMEOUT):
        try:
            unwinder = RemoteUnwinder(pid, all_threads=True, debug=True)
            return unwinder.get_stack_trace()
        except RuntimeError as e:
            continue
    raise RuntimeError("Failed to get stack trace after retries")


def get_async_stack_trace(pid):
    for _ in busy_retry(SHORT_TIMEOUT):
        try:
            unwinder = RemoteUnwinder(pid, debug=True)
            return unwinder.get_async_stack_trace()
        except RuntimeError as e:
            continue
    raise RuntimeError("Failed to get async stack trace after retries")


def get_all_awaited_by(pid):
    for _ in busy_retry(SHORT_TIMEOUT):
        try:
            unwinder = RemoteUnwinder(pid, debug=True)
            return unwinder.get_all_awaited_by()
        except RuntimeError as e:
            continue
    raise RuntimeError("Failed to get all awaited_by after retries")


def _get_stack_trace_with_retry(unwinder, timeout=SHORT_TIMEOUT, condition=None):
    """Get stack trace from an existing unwinder with retry for transient errors.

    This handles the case where we want to reuse an existing RemoteUnwinder
    instance but still handle transient failures like "Failed to parse initial
    frame in chain" that can occur when sampling at an inopportune moment.
    If condition is provided, keeps retrying until condition(traces) is True.
    """
    last_error = None
    for _ in busy_retry(timeout):
        try:
            traces = unwinder.get_stack_trace()
            if condition is None or condition(traces):
                return traces
            # Condition not met yet, keep retrying
        except TRANSIENT_ERRORS as e:
            last_error = e
            continue
    if last_error:
        raise RuntimeError(
            f"Failed to get stack trace after retries: {last_error}"
        )
    raise RuntimeError("Condition never satisfied within timeout")


# ============================================================================
# Base test class with shared infrastructure
# ============================================================================


class RemoteInspectionTestBase(unittest.TestCase):
    """Base class for remote inspection tests with common helpers."""

    maxDiff = None

    def _run_script_and_get_trace(
        self,
        script,
        trace_func,
        wait_for_signals=None,
        port=None,
        backlog=1,
    ):
        """
        Common pattern: run a script, wait for signals, get trace.

        Args:
            script: Script content (will be formatted with port if {port} present)
            trace_func: Function to call with pid to get trace (e.g., get_stack_trace)
            wait_for_signals: Signal(s) to wait for before getting trace
            port: Port to use (auto-selected if None)
            backlog: Socket listen backlog

        Returns:
            tuple: (trace_result, script_name)
        """
        if port is None:
            port = find_unused_port()

        # Format script with port if needed
        if "{port}" in script or "{{port}}" in script:
            script = script.replace("{{port}}", "{port}").format(port=port)

        with os_helper.temp_dir() as work_dir:
            script_dir = os.path.join(work_dir, "script_pkg")
            os.mkdir(script_dir)

            server_socket = _create_server_socket(port, backlog)
            script_name = _make_test_script(script_dir, "script", script)
            client_socket = None

            try:
                with _managed_subprocess([sys.executable, script_name]) as p:
                    client_socket, _ = server_socket.accept()
                    server_socket.close()
                    server_socket = None

                    if wait_for_signals:
                        _wait_for_signal(client_socket, wait_for_signals)

                    trace = trace_func(p.pid)
                    return trace, script_name
            finally:
                _cleanup_sockets(client_socket, server_socket)

    def _find_frame_in_trace(self, stack_trace, predicate):
        """
        Find a frame matching predicate in stack trace.

        Args:
            stack_trace: List of InterpreterInfo objects
            predicate: Function(frame) -> bool

        Returns:
            FrameInfo or None
        """
        for interpreter_info in stack_trace:
            for thread_info in interpreter_info.threads:
                for frame in thread_info.frame_info:
                    if predicate(frame):
                        return frame
        return None

    def _find_thread_by_id(self, stack_trace, thread_id):
        """Find a thread by its native thread ID."""
        for interpreter_info in stack_trace:
            for thread_info in interpreter_info.threads:
                if thread_info.thread_id == thread_id:
                    return thread_info
        return None

    def _find_thread_with_frame(self, stack_trace, frame_predicate):
        """Find a thread containing a frame matching predicate."""
        for interpreter_info in stack_trace:
            for thread_info in interpreter_info.threads:
                for frame in thread_info.frame_info:
                    if frame_predicate(frame):
                        return thread_info
        return None

    def _get_thread_statuses(self, stack_trace):
        """Extract thread_id -> status mapping from stack trace."""
        statuses = {}
        for interpreter_info in stack_trace:
            for thread_info in interpreter_info.threads:
                statuses[thread_info.thread_id] = thread_info.status
        return statuses

    def _get_task_id_map(self, stack_trace):
        """Create task_id -> task mapping from async stack trace."""
        return {task.task_id: task for task in stack_trace[0].awaited_by}

    def _get_awaited_by_relationships(self, stack_trace):
        """Extract task name to awaited_by set mapping."""
        id_to_task = self._get_task_id_map(stack_trace)
        return {
            task.task_name: set(
                id_to_task[awaited.task_name].task_name
                for awaited in task.awaited_by
            )
            for task in stack_trace[0].awaited_by
        }

    def _extract_coroutine_stacks(self, stack_trace):
        """Extract and format coroutine stacks from tasks."""
        return {
            task.task_name: sorted(
                tuple(tuple(frame) for frame in coro.call_stack)
                for coro in task.coroutine_stack
            )
            for task in stack_trace[0].awaited_by
        }

    @staticmethod
    def _frame_to_lineno_tuple(frame):
        """Convert frame to (filename, lineno, funcname, opcode) tuple.

        This extracts just the line number from the location, ignoring column
        offsets which can vary due to sampling timing (e.g., when two statements
        are on the same line, the sample might catch either one).
        """
        filename, location, funcname, opcode = frame
        return (filename, location.lineno, funcname, opcode)

    def _extract_coroutine_stacks_lineno_only(self, stack_trace):
        """Extract coroutine stacks with line numbers only (no column offsets).

        Use this for tests where sampling timing can cause column offset
        variations (e.g., 'expr1; expr2' on the same line).
        """
        return {
            task.task_name: sorted(
                tuple(self._frame_to_lineno_tuple(frame) for frame in coro.call_stack)
                for coro in task.coroutine_stack
            )
            for task in stack_trace[0].awaited_by
        }


# ============================================================================
# Test classes
# ============================================================================


@requires_remote_subprocess_debugging()
class TestGetStackTrace(RemoteInspectionTestBase):
    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    def test_remote_stack_trace(self):
        port = find_unused_port()
        script = textwrap.dedent(
            f"""\
            import time, sys, socket, threading

            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(('localhost', {port}))

            def bar():
                for x in range(100):
                    if x == 50:
                        baz()

            def baz():
                foo()

            def foo():
                sock.sendall(b"ready:thread\\n"); time.sleep(10_000)

            t = threading.Thread(target=bar)
            t.start()
            sock.sendall(b"ready:main\\n"); t.join()
            """
        )

        with os_helper.temp_dir() as work_dir:
            script_dir = os.path.join(work_dir, "script_pkg")
            os.mkdir(script_dir)

            server_socket = _create_server_socket(port)
            script_name = _make_test_script(script_dir, "script", script)
            client_socket = None

            try:
                with _managed_subprocess([sys.executable, script_name]) as p:
                    client_socket, _ = server_socket.accept()
                    server_socket.close()
                    server_socket = None

                    _wait_for_signal(
                        client_socket, [b"ready:main", b"ready:thread"]
                    )

                    stack_trace = get_stack_trace(p.pid)

                    # Find expected thread stack by funcname
                    found_thread = self._find_thread_with_frame(
                        stack_trace,
                        lambda f: f.funcname == "foo" and f.location.lineno == 15,
                    )
                    self.assertIsNotNone(
                        found_thread, "Expected thread stack trace not found"
                    )
                    # Check the funcnames in order
                    funcnames = [f.funcname for f in found_thread.frame_info]
                    self.assertEqual(
                        funcnames[:6],
                        ["foo", "baz", "bar", "Thread.run", "Thread._bootstrap_inner", "Thread._bootstrap"]
                    )

                    # Check main thread
                    found_main = self._find_frame_in_trace(
                        stack_trace,
                        lambda f: f.funcname == "<module>" and f.location.lineno == 19,
                    )
                    self.assertIsNotNone(
                        found_main, "Main thread stack trace not found"
                    )
            finally:
                _cleanup_sockets(client_socket, server_socket)

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    def test_self_trace_after_ctypes_import(self):
        """Test that RemoteUnwinder works on the same process after _ctypes import.

        When _ctypes is imported, it may call dlopen on the libpython shared
        library, creating a duplicate mapping in the process address space.
        The remote debugging code must skip these uninitialized duplicate
        mappings and find the real PyRuntime. See gh-144563.
        """
        # Run the test in a subprocess to avoid side effects
        script = textwrap.dedent("""\
            import os
            import _remote_debugging

            # Should work before _ctypes import
            unwinder = _remote_debugging.RemoteUnwinder(os.getpid())

            import _ctypes

            # Should still work after _ctypes import (gh-144563)
            unwinder = _remote_debugging.RemoteUnwinder(os.getpid())
            """)

        result = subprocess.run(
            [sys.executable, "-c", script],
            capture_output=True,
            text=True,
            timeout=SHORT_TIMEOUT,
        )
        self.assertEqual(
            result.returncode, 0,
            f"stdout: {result.stdout}\nstderr: {result.stderr}"
        )

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    def test_async_remote_stack_trace(self):
        port = find_unused_port()
        script = textwrap.dedent(
            f"""\
            import asyncio
            import time
            import sys
            import socket

            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(('localhost', {port}))

            def c5():
                sock.sendall(b"ready"); time.sleep(10_000)

            async def c4():
                await asyncio.sleep(0)
                c5()

            async def c3():
                await c4()

            async def c2():
                await c3()

            async def c1(task):
                await task

            async def main():
                async with asyncio.TaskGroup() as tg:
                    task = tg.create_task(c2(), name="c2_root")
                    tg.create_task(c1(task), name="sub_main_1")
                    tg.create_task(c1(task), name="sub_main_2")

            def new_eager_loop():
                loop = asyncio.new_event_loop()
                eager_task_factory = asyncio.create_eager_task_factory(
                    asyncio.Task)
                loop.set_task_factory(eager_task_factory)
                return loop

            asyncio.run(main(), loop_factory={{TASK_FACTORY}})
            """
        )

        for task_factory_variant in "asyncio.new_event_loop", "new_eager_loop":
            with (
                self.subTest(task_factory_variant=task_factory_variant),
                os_helper.temp_dir() as work_dir,
            ):
                script_dir = os.path.join(work_dir, "script_pkg")
                os.mkdir(script_dir)

                server_socket = _create_server_socket(port)
                script_name = _make_test_script(
                    script_dir,
                    "script",
                    script.format(TASK_FACTORY=task_factory_variant),
                )
                client_socket = None

                try:
                    with _managed_subprocess(
                        [sys.executable, script_name]
                    ) as p:
                        client_socket, _ = server_socket.accept()
                        server_socket.close()
                        server_socket = None

                        response = _wait_for_signal(client_socket, b"ready")
                        self.assertIn(b"ready", response)

                        stack_trace = get_async_stack_trace(p.pid)

                        # Check all tasks are present
                        tasks_names = [
                            task.task_name
                            for task in stack_trace[0].awaited_by
                        ]
                        for task_name in [
                            "c2_root",
                            "sub_main_1",
                            "sub_main_2",
                        ]:
                            self.assertIn(task_name, tasks_names)

                        # Check awaited_by relationships
                        relationships = self._get_awaited_by_relationships(
                            stack_trace
                        )
                        self.assertEqual(
                            relationships,
                            {
                                "c2_root": {
                                    "Task-1",
                                    "sub_main_1",
                                    "sub_main_2",
                                },
                                "Task-1": set(),
                                "sub_main_1": {"Task-1"},
                                "sub_main_2": {"Task-1"},
                            },
                        )

                        # Check coroutine stacks (using line numbers only to avoid
                        # flakiness from column offset variations when sampling
                        # catches different statements on the same line)
                        coroutine_stacks = self._extract_coroutine_stacks_lineno_only(
                            stack_trace
                        )
                        self.assertEqual(
                            coroutine_stacks,
                            {
                                "Task-1": [
                                    (
                                        (taskgroups.__file__, ANY, "TaskGroup._aexit", None),
                                        (taskgroups.__file__, ANY, "TaskGroup.__aexit__", None),
                                        (script_name, 26, "main", None),
                                    )
                                ],
                                "c2_root": [
                                    (
                                        (script_name, 10, "c5", None),
                                        (script_name, 14, "c4", None),
                                        (script_name, 17, "c3", None),
                                        (script_name, 20, "c2", None),
                                    )
                                ],
                                "sub_main_1": [
                                    ((script_name, 23, "c1", None),)
                                ],
                                "sub_main_2": [
                                    ((script_name, 23, "c1", None),)
                                ],
                            },
                        )

                        # Check awaited_by coroutine stacks (line numbers only)
                        id_to_task = self._get_task_id_map(stack_trace)
                        awaited_by_coroutine_stacks = {
                            task.task_name: sorted(
                                (
                                    id_to_task[coro.task_name].task_name,
                                    tuple(
                                        self._frame_to_lineno_tuple(frame)
                                        for frame in coro.call_stack
                                    ),
                                )
                                for coro in task.awaited_by
                            )
                            for task in stack_trace[0].awaited_by
                        }
                        self.assertEqual(
                            awaited_by_coroutine_stacks,
                            {
                                "Task-1": [],
                                "c2_root": [
                                    (
                                        "Task-1",
                                        (
                                            (taskgroups.__file__, ANY, "TaskGroup._aexit", None),
                                            (taskgroups.__file__, ANY, "TaskGroup.__aexit__", None),
                                            (script_name, 26, "main", None),
                                        ),
                                    ),
                                    (
                                        "sub_main_1",
                                        ((script_name, 23, "c1", None),),
                                    ),
                                    (
                                        "sub_main_2",
                                        ((script_name, 23, "c1", None),),
                                    ),
                                ],
                                "sub_main_1": [
                                    (
                                        "Task-1",
                                        (
                                            (taskgroups.__file__, ANY, "TaskGroup._aexit", None),
                                            (taskgroups.__file__, ANY, "TaskGroup.__aexit__", None),
                                            (script_name, 26, "main", None),
                                        ),
                                    )
                                ],
                                "sub_main_2": [
                                    (
                                        "Task-1",
                                        (
                                            (taskgroups.__file__, ANY, "TaskGroup._aexit", None),
                                            (taskgroups.__file__, ANY, "TaskGroup.__aexit__", None),
                                            (script_name, 26, "main", None),
                                        ),
                                    )
                                ],
                            },
                        )
                finally:
                    _cleanup_sockets(client_socket, server_socket)

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    def test_asyncgen_remote_stack_trace(self):
        port = find_unused_port()
        script = textwrap.dedent(
            f"""\
            import asyncio
            import time
            import sys
            import socket

            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(('localhost', {port}))

            async def gen_nested_call():
                sock.sendall(b"ready"); time.sleep(10_000)

            async def gen():
                for num in range(2):
                    yield num
                    if num == 1:
                        await gen_nested_call()

            async def main():
                async for el in gen():
                    pass

            asyncio.run(main())
            """
        )

        with os_helper.temp_dir() as work_dir:
            script_dir = os.path.join(work_dir, "script_pkg")
            os.mkdir(script_dir)

            server_socket = _create_server_socket(port)
            script_name = _make_test_script(script_dir, "script", script)
            client_socket = None

            try:
                with _managed_subprocess([sys.executable, script_name]) as p:
                    client_socket, _ = server_socket.accept()
                    server_socket.close()
                    server_socket = None

                    response = _wait_for_signal(client_socket, b"ready")
                    self.assertIn(b"ready", response)

                    stack_trace = get_async_stack_trace(p.pid)

                    # For this simple asyncgen test, we only expect one task
                    self.assertEqual(len(stack_trace[0].awaited_by), 1)
                    task = stack_trace[0].awaited_by[0]
                    self.assertEqual(task.task_name, "Task-1")

                    # Check the coroutine stack (using line numbers only to avoid
                    # flakiness from column offset variations when sampling
                    # catches different statements on the same line)
                    coroutine_stack = sorted(
                        tuple(self._frame_to_lineno_tuple(frame) for frame in coro.call_stack)
                        for coro in task.coroutine_stack
                    )
                    self.assertEqual(
                        coroutine_stack,
                        [
                            (
                                (script_name, 10, "gen_nested_call", None),
                                (script_name, 16, "gen", None),
                                (script_name, 19, "main", None),
                            )
                        ],
                    )

                    # No awaited_by relationships expected
                    self.assertEqual(task.awaited_by, [])
            finally:
                _cleanup_sockets(client_socket, server_socket)

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    def test_async_gather_remote_stack_trace(self):
        port = find_unused_port()
        script = textwrap.dedent(
            f"""\
            import asyncio
            import time
            import sys
            import socket

            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(('localhost', {port}))

            async def deep():
                await asyncio.sleep(0)
                sock.sendall(b"ready"); time.sleep(10_000)

            async def c1():
                await asyncio.sleep(0)
                await deep()

            async def c2():
                await asyncio.sleep(0)

            async def main():
                await asyncio.gather(c1(), c2())

            asyncio.run(main())
            """
        )

        with os_helper.temp_dir() as work_dir:
            script_dir = os.path.join(work_dir, "script_pkg")
            os.mkdir(script_dir)

            server_socket = _create_server_socket(port)
            script_name = _make_test_script(script_dir, "script", script)
            client_socket = None

            try:
                with _managed_subprocess([sys.executable, script_name]) as p:
                    client_socket, _ = server_socket.accept()
                    server_socket.close()
                    server_socket = None

                    response = _wait_for_signal(client_socket, b"ready")
                    self.assertIn(b"ready", response)

                    stack_trace = get_async_stack_trace(p.pid)

                    # Check all tasks are present
                    tasks_names = [
                        task.task_name for task in stack_trace[0].awaited_by
                    ]
                    for task_name in ["Task-1", "Task-2"]:
                        self.assertIn(task_name, tasks_names)

                    # Check awaited_by relationships
                    relationships = self._get_awaited_by_relationships(
                        stack_trace
                    )
                    self.assertEqual(
                        relationships,
                        {
                            "Task-1": set(),
                            "Task-2": {"Task-1"},
                        },
                    )

                    # Check coroutine stacks (using line numbers only to avoid
                    # flakiness from column offset variations when sampling
                    # catches different statements on the same line)
                    coroutine_stacks = self._extract_coroutine_stacks_lineno_only(
                        stack_trace
                    )
                    self.assertEqual(
                        coroutine_stacks,
                        {
                            "Task-1": [((script_name, 21, "main", None),)],
                            "Task-2": [
                                (
                                    (script_name, 11, "deep", None),
                                    (script_name, 15, "c1", None),
                                )
                            ],
                        },
                    )

                    # Check awaited_by coroutine stacks (line numbers only)
                    id_to_task = self._get_task_id_map(stack_trace)
                    awaited_by_coroutine_stacks = {
                        task.task_name: sorted(
                            (
                                id_to_task[coro.task_name].task_name,
                                tuple(
                                    self._frame_to_lineno_tuple(frame) for frame in coro.call_stack
                                ),
                            )
                            for coro in task.awaited_by
                        )
                        for task in stack_trace[0].awaited_by
                    }
                    self.assertEqual(
                        awaited_by_coroutine_stacks,
                        {
                            "Task-1": [],
                            "Task-2": [
                                ("Task-1", ((script_name, 21, "main", None),))
                            ],
                        },
                    )
            finally:
                _cleanup_sockets(client_socket, server_socket)

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    def test_async_staggered_race_remote_stack_trace(self):
        port = find_unused_port()
        script = textwrap.dedent(
            f"""\
            import asyncio.staggered
            import time
            import sys
            import socket

            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(('localhost', {port}))

            async def deep():
                await asyncio.sleep(0)
                sock.sendall(b"ready"); time.sleep(10_000)

            async def c1():
                await asyncio.sleep(0)
                await deep()

            async def c2():
                await asyncio.sleep(10_000)

            async def main():
                await asyncio.staggered.staggered_race(
                    [c1, c2],
                    delay=None,
                )

            asyncio.run(main())
            """
        )

        with os_helper.temp_dir() as work_dir:
            script_dir = os.path.join(work_dir, "script_pkg")
            os.mkdir(script_dir)

            server_socket = _create_server_socket(port)
            script_name = _make_test_script(script_dir, "script", script)
            client_socket = None

            try:
                with _managed_subprocess([sys.executable, script_name]) as p:
                    client_socket, _ = server_socket.accept()
                    server_socket.close()
                    server_socket = None

                    response = _wait_for_signal(client_socket, b"ready")
                    self.assertIn(b"ready", response)

                    stack_trace = get_async_stack_trace(p.pid)

                    # Check all tasks are present
                    tasks_names = [
                        task.task_name for task in stack_trace[0].awaited_by
                    ]
                    for task_name in ["Task-1", "Task-2"]:
                        self.assertIn(task_name, tasks_names)

                    # Check awaited_by relationships
                    relationships = self._get_awaited_by_relationships(
                        stack_trace
                    )
                    self.assertEqual(
                        relationships,
                        {
                            "Task-1": set(),
                            "Task-2": {"Task-1"},
                        },
                    )

                    # Check coroutine stacks (using line numbers only to avoid
                    # flakiness from column offset variations when sampling
                    # catches different statements on the same line)
                    coroutine_stacks = self._extract_coroutine_stacks_lineno_only(
                        stack_trace
                    )
                    self.assertEqual(
                        coroutine_stacks,
                        {
                            "Task-1": [
                                (
                                    (staggered.__file__, ANY, "staggered_race", None),
                                    (script_name, 21, "main", None),
                                )
                            ],
                            "Task-2": [
                                (
                                    (script_name, 11, "deep", None),
                                    (script_name, 15, "c1", None),
                                    (staggered.__file__, ANY, "staggered_race.<locals>.run_one_coro", None),
                                )
                            ],
                        },
                    )

                    # Check awaited_by coroutine stacks (line numbers only)
                    id_to_task = self._get_task_id_map(stack_trace)
                    awaited_by_coroutine_stacks = {
                        task.task_name: sorted(
                            (
                                id_to_task[coro.task_name].task_name,
                                tuple(
                                    self._frame_to_lineno_tuple(frame) for frame in coro.call_stack
                                ),
                            )
                            for coro in task.awaited_by
                        )
                        for task in stack_trace[0].awaited_by
                    }
                    self.assertEqual(
                        awaited_by_coroutine_stacks,
                        {
                            "Task-1": [],
                            "Task-2": [
                                (
                                    "Task-1",
                                    (
                                        (staggered.__file__, ANY, "staggered_race", None),
                                        (script_name, 21, "main", None),
                                    ),
                                )
                            ],
                        },
                    )
            finally:
                _cleanup_sockets(client_socket, server_socket)

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    def test_async_global_awaited_by(self):
        # Reduced from 1000 to 100 to avoid file descriptor exhaustion
        # when running tests in parallel (e.g., -j 20)
        NUM_TASKS = 100

        port = find_unused_port()
        script = textwrap.dedent(
            f"""\
            import asyncio
            import os
            import random
            import sys
            import socket
            from string import ascii_lowercase, digits
            from test.support import socket_helper, SHORT_TIMEOUT

            HOST = '127.0.0.1'
            PORT = socket_helper.find_unused_port()
            connections = 0

            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(('localhost', {port}))

            class EchoServerProtocol(asyncio.Protocol):
                def connection_made(self, transport):
                    global connections
                    connections += 1
                    self.transport = transport

                def data_received(self, data):
                    self.transport.write(data)
                    self.transport.close()

            async def echo_client(message):
                reader, writer = await asyncio.open_connection(HOST, PORT)
                writer.write(message.encode())
                await writer.drain()

                data = await reader.read(100)
                assert message == data.decode()
                writer.close()
                await writer.wait_closed()
                sock.sendall(b"ready")
                await asyncio.sleep(SHORT_TIMEOUT)

            async def echo_client_spam(server):
                async with asyncio.TaskGroup() as tg:
                    while connections < {NUM_TASKS}:
                        msg = list(ascii_lowercase + digits)
                        random.shuffle(msg)
                        tg.create_task(echo_client("".join(msg)))
                        await asyncio.sleep(0)
                server.close()
                await server.wait_closed()

            async def main():
                loop = asyncio.get_running_loop()
                server = await loop.create_server(EchoServerProtocol, HOST, PORT)
                async with server:
                    async with asyncio.TaskGroup() as tg:
                        tg.create_task(server.serve_forever(), name="server task")
                        tg.create_task(echo_client_spam(server), name="echo client spam")

            asyncio.run(main())
            """
        )

        with os_helper.temp_dir() as work_dir:
            script_dir = os.path.join(work_dir, "script_pkg")
            os.mkdir(script_dir)

            server_socket = _create_server_socket(port)
            script_name = _make_test_script(script_dir, "script", script)
            client_socket = None

            try:
                with _managed_subprocess([sys.executable, script_name]) as p:
                    client_socket, _ = server_socket.accept()
                    server_socket.close()
                    server_socket = None

                    # Wait for NUM_TASKS "ready" signals
                    try:
                        _wait_for_n_signals(client_socket, b"ready", NUM_TASKS)
                    except RuntimeError as e:
                        self.fail(str(e))

                    all_awaited_by = get_all_awaited_by(p.pid)

                    # Expected: a list of two elements: 1 thread, 1 interp
                    self.assertEqual(len(all_awaited_by), 2)
                    # Expected: a tuple with the thread ID and the awaited_by list
                    self.assertEqual(len(all_awaited_by[0]), 2)
                    # Expected: no tasks in the fallback per-interp task list
                    self.assertEqual(all_awaited_by[1], (0, []))

                    entries = all_awaited_by[0][1]
                    # Expected: at least NUM_TASKS pending tasks
                    self.assertGreaterEqual(len(entries), NUM_TASKS)

                    # Check the main task structure
                    main_stack = [
                        FrameInfo(
                            [taskgroups.__file__, ANY, "TaskGroup._aexit", ANY]
                        ),
                        FrameInfo(
                            [taskgroups.__file__, ANY, "TaskGroup.__aexit__", ANY]
                        ),
                        FrameInfo([script_name, ANY, "main", ANY]),
                    ]
                    self.assertIn(
                        TaskInfo(
                            [ANY, "Task-1", [CoroInfo([main_stack, ANY])], []]
                        ),
                        entries,
                    )
                    self.assertIn(
                        TaskInfo(
                            [
                                ANY,
                                "server task",
                                [
                                    CoroInfo(
                                        [
                                            [
                                                FrameInfo(
                                                    [
                                                        base_events.__file__,
                                                        ANY,
                                                        "Server.serve_forever",
                                                        ANY,
                                                    ]
                                                )
                                            ],
                                            ANY,
                                        ]
                                    )
                                ],
                                [
                                    CoroInfo(
                                        [
                                            [
                                                FrameInfo(
                                                    [
                                                        taskgroups.__file__,
                                                        ANY,
                                                        "TaskGroup._aexit",
                                                        ANY,
                                                    ]
                                                ),
                                                FrameInfo(
                                                    [
                                                        taskgroups.__file__,
                                                        ANY,
                                                        "TaskGroup.__aexit__",
                                                        ANY,
                                                    ]
                                                ),
                                                FrameInfo(
                                                    [script_name, ANY, "main", ANY]
                                                ),
                                            ],
                                            ANY,
                                        ]
                                    )
                                ],
                            ]
                        ),
                        entries,
                    )
                    self.assertIn(
                        TaskInfo(
                            [
                                ANY,
                                "Task-4",
                                [
                                    CoroInfo(
                                        [
                                            [
                                                FrameInfo(
                                                    [
                                                        tasks.__file__,
                                                        ANY,
                                                        "sleep",
                                                        ANY,
                                                    ]
                                                ),
                                                FrameInfo(
                                                    [
                                                        script_name,
                                                        ANY,
                                                        "echo_client",
                                                        ANY,
                                                    ]
                                                ),
                                            ],
                                            ANY,
                                        ]
                                    )
                                ],
                                [
                                    CoroInfo(
                                        [
                                            [
                                                FrameInfo(
                                                    [
                                                        taskgroups.__file__,
                                                        ANY,
                                                        "TaskGroup._aexit",
                                                        ANY,
                                                    ]
                                                ),
                                                FrameInfo(
                                                    [
                                                        taskgroups.__file__,
                                                        ANY,
                                                        "TaskGroup.__aexit__",
                                                        ANY,
                                                    ]
                                                ),
                                                FrameInfo(
                                                    [
                                                        script_name,
                                                        ANY,
                                                        "echo_client_spam",
                                                        ANY,
                                                    ]
                                                ),
                                            ],
                                            ANY,
                                        ]
                                    )
                                ],
                            ]
                        ),
                        entries,
                    )

                    # Find tasks awaited by echo_client_spam via TaskGroup
                    def matches_awaited_by_pattern(task):
                        if len(task.awaited_by) != 1:
                            return False
                        coro = task.awaited_by[0]
                        if len(coro.call_stack) != 3:
                            return False
                        funcnames = [f.funcname for f in coro.call_stack]
                        return funcnames == [
                            "TaskGroup._aexit",
                            "TaskGroup.__aexit__",
                            "echo_client_spam",
                        ]

                    tasks_with_awaited = [
                        task
                        for task in entries
                        if matches_awaited_by_pattern(task)
                    ]
                    self.assertGreaterEqual(len(tasks_with_awaited), NUM_TASKS)

                    # Final task should be from echo client spam (not on Windows)
                    if sys.platform != "win32":
                        self.assertEqual(
                            tasks_with_awaited[-1].awaited_by,
                            entries[-1].awaited_by,
                        )
            finally:
                _cleanup_sockets(client_socket, server_socket)

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    def test_self_trace(self):
        stack_trace = get_stack_trace(os.getpid())

        this_thread_stack = None
        for interpreter_info in stack_trace:
            for thread_info in interpreter_info.threads:
                if thread_info.thread_id == threading.get_native_id():
                    this_thread_stack = thread_info.frame_info
                    break
            if this_thread_stack:
                break

        self.assertIsNotNone(this_thread_stack)
        # Check the top two frames
        self.assertGreaterEqual(len(this_thread_stack), 2)
        self.assertEqual(this_thread_stack[0].funcname, "get_stack_trace")
        self.assertTrue(this_thread_stack[0].filename.endswith("test_external_inspection.py"))
        self.assertEqual(this_thread_stack[1].funcname, "TestGetStackTrace.test_self_trace")
        self.assertTrue(this_thread_stack[1].filename.endswith("test_external_inspection.py"))

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    @requires_subinterpreters
    def test_subinterpreter_stack_trace(self):
        port = find_unused_port()

        import pickle

        subinterp_code = textwrap.dedent(f"""
            import socket
            import time

            def sub_worker():
                def nested_func():
                    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    sock.connect(('localhost', {port}))
                    sock.sendall(b"ready:sub\\n")
                    time.sleep(10_000)
                nested_func()

            sub_worker()
        """).strip()

        pickled_code = pickle.dumps(subinterp_code)

        script = textwrap.dedent(
            f"""
            from concurrent import interpreters
            import time
            import sys
            import socket
            import threading

            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(('localhost', {port}))

            def main_worker():
                sock.sendall(b"ready:main\\n")
                time.sleep(10_000)

            def run_subinterp():
                subinterp = interpreters.create()
                import pickle
                pickled_code = {pickled_code!r}
                subinterp_code = pickle.loads(pickled_code)
                subinterp.exec(subinterp_code)

            sub_thread = threading.Thread(target=run_subinterp)
            sub_thread.start()

            main_thread = threading.Thread(target=main_worker)
            main_thread.start()

            main_thread.join()
            sub_thread.join()
            """
        )

        with os_helper.temp_dir() as work_dir:
            script_dir = os.path.join(work_dir, "script_pkg")
            os.mkdir(script_dir)

            server_socket = _create_server_socket(port)
            script_name = _make_test_script(script_dir, "script", script)
            client_sockets = []

            try:
                with _managed_subprocess([sys.executable, script_name]) as p:
                    # Accept connections from both main and subinterpreter
                    responses = set()
                    while len(responses) < 2:
                        try:
                            client_socket, _ = server_socket.accept()
                            client_sockets.append(client_socket)
                            response = client_socket.recv(1024)
                            if b"ready:main" in response:
                                responses.add("main")
                            if b"ready:sub" in response:
                                responses.add("sub")
                        except socket.timeout:
                            break

                    server_socket.close()
                    server_socket = None

                    stack_trace = get_stack_trace(p.pid)

                    # Verify we have at least one interpreter
                    self.assertGreaterEqual(len(stack_trace), 1)

                    # Look for main interpreter (ID 0) and subinterpreter (ID > 0)
                    main_interp = None
                    sub_interp = None
                    for interpreter_info in stack_trace:
                        if interpreter_info.interpreter_id == 0:
                            main_interp = interpreter_info
                        elif interpreter_info.interpreter_id > 0:
                            sub_interp = interpreter_info

                    self.assertIsNotNone(
                        main_interp, "Main interpreter should be present"
                    )

                    # Check main interpreter has expected stack trace
                    main_found = self._find_frame_in_trace(
                        [main_interp], lambda f: f.funcname == "main_worker"
                    )
                    self.assertIsNotNone(
                        main_found,
                        "Main interpreter should have main_worker in stack",
                    )

                    # If subinterpreter is present, check its stack trace
                    if sub_interp:
                        sub_found = self._find_frame_in_trace(
                            [sub_interp],
                            lambda f: f.funcname
                            in ("sub_worker", "nested_func"),
                        )
                        self.assertIsNotNone(
                            sub_found,
                            "Subinterpreter should have sub_worker or nested_func in stack",
                        )
            finally:
                _cleanup_sockets(*client_sockets, server_socket)

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    @requires_subinterpreters
    def test_multiple_subinterpreters_with_threads(self):
        port = find_unused_port()

        import pickle

        subinterp1_code = textwrap.dedent(f"""
            import socket
            import time
            import threading

            def worker1():
                def nested_func():
                    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    sock.connect(('localhost', {port}))
                    sock.sendall(b"ready:sub1-t1\\n")
                    time.sleep(10_000)
                nested_func()

            def worker2():
                def nested_func():
                    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    sock.connect(('localhost', {port}))
                    sock.sendall(b"ready:sub1-t2\\n")
                    time.sleep(10_000)
                nested_func()

            t1 = threading.Thread(target=worker1)
            t2 = threading.Thread(target=worker2)
            t1.start()
            t2.start()
            t1.join()
            t2.join()
        """).strip()

        subinterp2_code = textwrap.dedent(f"""
            import socket
            import time
            import threading

            def worker1():
                def nested_func():
                    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    sock.connect(('localhost', {port}))
                    sock.sendall(b"ready:sub2-t1\\n")
                    time.sleep(10_000)
                nested_func()

            def worker2():
                def nested_func():
                    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    sock.connect(('localhost', {port}))
                    sock.sendall(b"ready:sub2-t2\\n")
                    time.sleep(10_000)
                nested_func()

            t1 = threading.Thread(target=worker1)
            t2 = threading.Thread(target=worker2)
            t1.start()
            t2.start()
            t1.join()
            t2.join()
        """).strip()

        pickled_code1 = pickle.dumps(subinterp1_code)
        pickled_code2 = pickle.dumps(subinterp2_code)

        script = textwrap.dedent(
            f"""
            from concurrent import interpreters
            import time
            import sys
            import socket
            import threading

            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(('localhost', {port}))

            def main_worker():
                sock.sendall(b"ready:main\\n")
                time.sleep(10_000)

            def run_subinterp1():
                subinterp = interpreters.create()
                import pickle
                pickled_code = {pickled_code1!r}
                subinterp_code = pickle.loads(pickled_code)
                subinterp.exec(subinterp_code)

            def run_subinterp2():
                subinterp = interpreters.create()
                import pickle
                pickled_code = {pickled_code2!r}
                subinterp_code = pickle.loads(pickled_code)
                subinterp.exec(subinterp_code)

            sub1_thread = threading.Thread(target=run_subinterp1)
            sub2_thread = threading.Thread(target=run_subinterp2)
            sub1_thread.start()
            sub2_thread.start()

            main_thread = threading.Thread(target=main_worker)
            main_thread.start()

            main_thread.join()
            sub1_thread.join()
            sub2_thread.join()
            """
        )

        with os_helper.temp_dir() as work_dir:
            script_dir = os.path.join(work_dir, "script_pkg")
            os.mkdir(script_dir)

            server_socket = _create_server_socket(port, backlog=5)
            script_name = _make_test_script(script_dir, "script", script)
            client_sockets = []

            try:
                with _managed_subprocess([sys.executable, script_name]) as p:
                    # Accept connections from main and all subinterpreter threads
                    expected_responses = {
                        "ready:main",
                        "ready:sub1-t1",
                        "ready:sub1-t2",
                        "ready:sub2-t1",
                        "ready:sub2-t2",
                    }
                    responses = set()

                    while len(responses) < 5:
                        try:
                            client_socket, _ = server_socket.accept()
                            client_sockets.append(client_socket)
                            response = client_socket.recv(1024)
                            response_str = response.decode().strip()
                            if response_str in expected_responses:
                                responses.add(response_str)
                        except socket.timeout:
                            break

                    server_socket.close()
                    server_socket = None

                    stack_trace = get_stack_trace(p.pid)

                    # Verify we have multiple interpreters
                    self.assertGreaterEqual(len(stack_trace), 2)

                    # Count interpreters by ID
                    interpreter_ids = {
                        interp.interpreter_id for interp in stack_trace
                    }
                    self.assertIn(
                        0,
                        interpreter_ids,
                        "Main interpreter should be present",
                    )
                    self.assertGreaterEqual(len(interpreter_ids), 3)

                    # Count total threads
                    total_threads = sum(
                        len(interp.threads) for interp in stack_trace
                    )
                    self.assertGreaterEqual(total_threads, 5)

                    # Look for expected function names
                    all_funcnames = set()
                    for interpreter_info in stack_trace:
                        for thread_info in interpreter_info.threads:
                            for frame in thread_info.frame_info:
                                all_funcnames.add(frame.funcname)

                    expected_funcs = {
                        "main_worker",
                        "worker1",
                        "worker2",
                        "nested_func",
                    }
                    found_funcs = expected_funcs.intersection(all_funcnames)
                    self.assertGreater(len(found_funcs), 0)
            finally:
                _cleanup_sockets(*client_sockets, server_socket)

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    @requires_gil_enabled("Free threaded builds don't have an 'active thread'")
    def test_only_active_thread(self):
        port = find_unused_port()
        script = textwrap.dedent(
            f"""\
            import time, sys, socket, threading

            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(('localhost', {port}))

            def worker_thread(name, barrier, ready_event):
                barrier.wait()
                ready_event.wait()
                time.sleep(10_000)

            def main_work():
                sock.sendall(b"working\\n")
                count = 0
                while count < 100000000:
                    count += 1
                    if count % 10000000 == 0:
                        pass
                sock.sendall(b"done\\n")

            num_threads = 3
            barrier = threading.Barrier(num_threads + 1)
            ready_event = threading.Event()

            threads = []
            for i in range(num_threads):
                t = threading.Thread(target=worker_thread, args=(f"Worker-{{i}}", barrier, ready_event))
                t.start()
                threads.append(t)

            barrier.wait()
            sock.sendall(b"ready\\n")
            ready_event.set()
            main_work()
            """
        )

        with os_helper.temp_dir() as work_dir:
            script_dir = os.path.join(work_dir, "script_pkg")
            os.mkdir(script_dir)

            server_socket = _create_server_socket(port)
            script_name = _make_test_script(script_dir, "script", script)
            client_socket = None

            try:
                with _managed_subprocess([sys.executable, script_name]) as p:
                    client_socket, _ = server_socket.accept()
                    server_socket.close()
                    server_socket = None

                    # Wait for ready and working signals
                    _wait_for_signal(client_socket, [b"ready", b"working"])

                    # Get stack trace with all threads
                    unwinder_all = RemoteUnwinder(p.pid, all_threads=True)
                    for _ in busy_retry(SHORT_TIMEOUT):
                        with contextlib.suppress(*TRANSIENT_ERRORS):
                            all_traces = unwinder_all.get_stack_trace()
                            found = self._find_frame_in_trace(
                                all_traces,
                                lambda f: f.funcname == "main_work"
                                and f.location.lineno > 12,
                            )
                            if found:
                                break
                    else:
                        self.fail(
                            "Main thread did not start its busy work on time"
                        )

                    # Get stack trace with only GIL holder
                    unwinder_gil = RemoteUnwinder(
                        p.pid, only_active_thread=True
                    )
                    # Use condition to retry until we capture a thread holding the GIL
                    # (sampling may catch moments with no GIL holder on slow CI)
                    gil_traces = _get_stack_trace_with_retry(
                        unwinder_gil,
                        condition=lambda t: sum(len(i.threads) for i in t) >= 1,
                    )

                    # Count threads
                    total_threads = sum(
                        len(interp.threads) for interp in all_traces
                    )
                    self.assertGreater(total_threads, 1)

                    total_gil_threads = sum(
                        len(interp.threads) for interp in gil_traces
                    )
                    self.assertEqual(total_gil_threads, 1)

                    # Get the GIL holder thread ID
                    gil_thread_id = None
                    for interpreter_info in gil_traces:
                        if interpreter_info.threads:
                            gil_thread_id = interpreter_info.threads[
                                0
                            ].thread_id
                            break

                    # Get all thread IDs
                    all_thread_ids = []
                    for interpreter_info in all_traces:
                        for thread_info in interpreter_info.threads:
                            all_thread_ids.append(thread_info.thread_id)

                    self.assertIn(gil_thread_id, all_thread_ids)
            finally:
                _cleanup_sockets(client_socket, server_socket)

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    def test_opcodes_collection(self):
        """Test that opcodes are collected when the opcodes flag is set."""
        script = textwrap.dedent(
            """\
            import time, sys, socket

            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(('localhost', {port}))

            def foo():
                sock.sendall(b"ready")
                time.sleep(10_000)

            foo()
            """
        )

        def get_trace_with_opcodes(pid):
            return RemoteUnwinder(pid, opcodes=True).get_stack_trace()

        stack_trace, _ = self._run_script_and_get_trace(
            script, get_trace_with_opcodes, wait_for_signals=b"ready"
        )

        # Find our foo frame and verify it has an opcode
        foo_frame = self._find_frame_in_trace(
            stack_trace, lambda f: f.funcname == "foo"
        )
        self.assertIsNotNone(foo_frame, "Could not find foo frame")
        self.assertIsInstance(foo_frame.opcode, int)
        self.assertGreaterEqual(foo_frame.opcode, 0)

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    def test_location_tuple_format(self):
        """Test that location is a 4-tuple (lineno, end_lineno, col_offset, end_col_offset)."""
        script = textwrap.dedent(
            """\
            import time, sys, socket

            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(('localhost', {port}))

            def foo():
                sock.sendall(b"ready")
                time.sleep(10_000)

            foo()
            """
        )

        def get_trace_with_opcodes(pid):
            return RemoteUnwinder(pid, opcodes=True).get_stack_trace()

        stack_trace, _ = self._run_script_and_get_trace(
            script, get_trace_with_opcodes, wait_for_signals=b"ready"
        )

        # Find our foo frame
        foo_frame = self._find_frame_in_trace(
            stack_trace, lambda f: f.funcname == "foo"
        )
        self.assertIsNotNone(foo_frame, "Could not find foo frame")

        # Check location is a 4-tuple with valid values
        location = foo_frame.location
        self.assertIsInstance(location, tuple)
        self.assertEqual(len(location), 4)
        lineno, end_lineno, col_offset, end_col_offset = location
        self.assertIsInstance(lineno, int)
        self.assertGreater(lineno, 0)
        self.assertIsInstance(end_lineno, int)
        self.assertGreaterEqual(end_lineno, lineno)
        self.assertIsInstance(col_offset, int)
        self.assertGreaterEqual(col_offset, 0)
        self.assertIsInstance(end_col_offset, int)
        self.assertGreaterEqual(end_col_offset, col_offset)

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    def test_location_tuple_exact_values(self):
        """Test exact values of location tuple including column offsets."""
        script = textwrap.dedent(
            """\
            import time, sys, socket

            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(('localhost', {port}))

            def foo():
                sock.sendall(b"ready")
                time.sleep(10_000)

            foo()
            """
        )

        def get_trace_with_opcodes(pid):
            return RemoteUnwinder(pid, opcodes=True).get_stack_trace()

        stack_trace, _ = self._run_script_and_get_trace(
            script, get_trace_with_opcodes, wait_for_signals=b"ready"
        )

        foo_frame = self._find_frame_in_trace(
            stack_trace, lambda f: f.funcname == "foo"
        )
        self.assertIsNotNone(foo_frame, "Could not find foo frame")

        # Can catch either sock.sendall (line 7) or time.sleep (line 8)
        location = foo_frame.location
        valid_locations = [
            (7, 7, 4, 26),  # sock.sendall(b"ready")
            (8, 8, 4, 22),  # time.sleep(10_000)
        ]
        actual = (location.lineno, location.end_lineno,
                  location.col_offset, location.end_col_offset)
        self.assertIn(actual, valid_locations)


class TestUnsupportedPlatformHandling(unittest.TestCase):
    @unittest.skipIf(
        sys.platform in ("linux", "darwin", "win32"),
        "Test only runs on unsupported platforms (not Linux, macOS, or Windows)",
    )
    @unittest.skipIf(
        sys.platform == "android", "Android raises Linux-specific exception"
    )
    def test_unsupported_platform_error(self):
        with self.assertRaises(RuntimeError) as cm:
            RemoteUnwinder(os.getpid())

        self.assertIn(
            "Reading the PyRuntime section is not supported on this platform",
            str(cm.exception),
        )


@requires_remote_subprocess_debugging()
class TestDetectionOfThreadStatus(RemoteInspectionTestBase):
    def _run_thread_status_test(self, mode, check_condition):
        """
        Common pattern for thread status detection tests.

        Args:
            mode: Profiling mode (PROFILING_MODE_CPU, PROFILING_MODE_GIL, etc.)
            check_condition: Function(statuses, sleeper_tid, busy_tid) -> bool
        """
        port = find_unused_port()
        script = textwrap.dedent(
            f"""\
            import time, sys, socket, threading
            import os

            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(('localhost', {port}))

            def sleeper():
                tid = threading.get_native_id()
                sock.sendall(f'ready:sleeper:{{tid}}\\n'.encode())
                time.sleep(10000)

            def busy():
                tid = threading.get_native_id()
                sock.sendall(f'ready:busy:{{tid}}\\n'.encode())
                x = 0
                while True:
                    x = x + 1
                time.sleep(0.5)

            t1 = threading.Thread(target=sleeper)
            t2 = threading.Thread(target=busy)
            t1.start()
            t2.start()
            sock.sendall(b'ready:main\\n')
            t1.join()
            t2.join()
            sock.close()
            """
        )

        with os_helper.temp_dir() as work_dir:
            script_dir = os.path.join(work_dir, "script_pkg")
            os.mkdir(script_dir)

            server_socket = _create_server_socket(port)
            script_name = _make_test_script(
                script_dir, "thread_status_script", script
            )
            client_socket = None

            try:
                with _managed_subprocess([sys.executable, script_name]) as p:
                    client_socket, _ = server_socket.accept()
                    server_socket.close()
                    server_socket = None

                    # Wait for all ready signals and parse TIDs
                    response = _wait_for_signal(
                        client_socket,
                        [b"ready:main", b"ready:sleeper", b"ready:busy"],
                    )

                    sleeper_tid = None
                    busy_tid = None
                    for line in response.split(b"\n"):
                        if line.startswith(b"ready:sleeper:"):
                            try:
                                sleeper_tid = int(line.split(b":")[-1])
                            except (ValueError, IndexError):
                                pass
                        elif line.startswith(b"ready:busy:"):
                            try:
                                busy_tid = int(line.split(b":")[-1])
                            except (ValueError, IndexError):
                                pass

                    self.assertIsNotNone(
                        sleeper_tid, "Sleeper thread id not received"
                    )
                    self.assertIsNotNone(
                        busy_tid, "Busy thread id not received"
                    )

                    # Sample until we see expected thread states
                    statuses = {}
                    unwinder = RemoteUnwinder(
                        p.pid,
                        all_threads=True,
                        mode=mode,
                        skip_non_matching_threads=False,
                    )
                    for _ in busy_retry(SHORT_TIMEOUT):
                        with contextlib.suppress(*TRANSIENT_ERRORS):
                            traces = unwinder.get_stack_trace()
                            statuses = self._get_thread_statuses(traces)

                            if check_condition(
                                statuses, sleeper_tid, busy_tid
                            ):
                                break

                    return statuses, sleeper_tid, busy_tid
            finally:
                _cleanup_sockets(client_socket, server_socket)

    @unittest.skipIf(
        sys.platform not in ("linux", "darwin", "win32"),
        "Test only runs on supported platforms (Linux, macOS, or Windows)",
    )
    @unittest.skipIf(
        sys.platform == "android", "Android raises Linux-specific exception"
    )
    def test_thread_status_detection(self):
        def check_cpu_status(statuses, sleeper_tid, busy_tid):
            return (
                sleeper_tid in statuses
                and busy_tid in statuses
                and not (statuses[sleeper_tid] & THREAD_STATUS_ON_CPU)
                and (statuses[busy_tid] & THREAD_STATUS_ON_CPU)
            )

        statuses, sleeper_tid, busy_tid = self._run_thread_status_test(
            PROFILING_MODE_CPU, check_cpu_status
        )

        self.assertIn(sleeper_tid, statuses)
        self.assertIn(busy_tid, statuses)
        self.assertFalse(
            statuses[sleeper_tid] & THREAD_STATUS_ON_CPU,
            "Sleeper thread should be off CPU",
        )
        self.assertTrue(
            statuses[busy_tid] & THREAD_STATUS_ON_CPU,
            "Busy thread should be on CPU",
        )

    @unittest.skipIf(
        sys.platform not in ("linux", "darwin", "win32"),
        "Test only runs on supported platforms (Linux, macOS, or Windows)",
    )
    @unittest.skipIf(
        sys.platform == "android", "Android raises Linux-specific exception"
    )
    def test_thread_status_gil_detection(self):
        def check_gil_status(statuses, sleeper_tid, busy_tid):
            return (
                sleeper_tid in statuses
                and busy_tid in statuses
                and not (statuses[sleeper_tid] & THREAD_STATUS_HAS_GIL)
                and (statuses[busy_tid] & THREAD_STATUS_HAS_GIL)
            )

        statuses, sleeper_tid, busy_tid = self._run_thread_status_test(
            PROFILING_MODE_GIL, check_gil_status
        )

        self.assertIn(sleeper_tid, statuses)
        self.assertIn(busy_tid, statuses)
        self.assertFalse(
            statuses[sleeper_tid] & THREAD_STATUS_HAS_GIL,
            "Sleeper thread should not have GIL",
        )
        self.assertTrue(
            statuses[busy_tid] & THREAD_STATUS_HAS_GIL,
            "Busy thread should have GIL",
        )

    @unittest.skipIf(
        sys.platform not in ("linux", "darwin", "win32"),
        "Test only runs on supported platforms (Linux, macOS, or Windows)",
    )
    @unittest.skipIf(
        sys.platform == "android", "Android raises Linux-specific exception"
    )
    def test_thread_status_all_mode_detection(self):
        port = find_unused_port()
        script = textwrap.dedent(
            f"""\
            import socket
            import threading
            import time
            import sys

            def sleeper_thread():
                conn = socket.create_connection(("localhost", {port}))
                conn.sendall(b"sleeper:" + str(threading.get_native_id()).encode())
                while True:
                    time.sleep(1)

            def busy_thread():
                conn = socket.create_connection(("localhost", {port}))
                conn.sendall(b"busy:" + str(threading.get_native_id()).encode())
                while True:
                    sum(range(100000))

            t1 = threading.Thread(target=sleeper_thread)
            t2 = threading.Thread(target=busy_thread)
            t1.start()
            t2.start()
            t1.join()
            t2.join()
            """
        )

        with os_helper.temp_dir() as tmp_dir:
            script_file = make_script(tmp_dir, "script", script)
            server_socket = _create_server_socket(port, backlog=2)
            client_sockets = []

            try:
                with _managed_subprocess(
                    [sys.executable, script_file],
                ) as p:
                    sleeper_tid = None
                    busy_tid = None

                    # Receive thread IDs from the child process
                    for _ in range(2):
                        client_socket, _ = server_socket.accept()
                        client_sockets.append(client_socket)
                        line = client_socket.recv(1024)
                        if line:
                            if line.startswith(b"sleeper:"):
                                try:
                                    sleeper_tid = int(line.split(b":")[-1])
                                except (ValueError, IndexError):
                                    pass
                            elif line.startswith(b"busy:"):
                                try:
                                    busy_tid = int(line.split(b":")[-1])
                                except (ValueError, IndexError):
                                    pass

                    server_socket.close()
                    server_socket = None

                    statuses = {}
                    unwinder = RemoteUnwinder(
                        p.pid,
                        all_threads=True,
                        mode=PROFILING_MODE_ALL,
                        skip_non_matching_threads=False,
                    )
                    for _ in busy_retry(SHORT_TIMEOUT):
                        with contextlib.suppress(*TRANSIENT_ERRORS):
                            traces = unwinder.get_stack_trace()
                            statuses = self._get_thread_statuses(traces)

                            # Check ALL mode provides both GIL and CPU info
                            if (
                                sleeper_tid in statuses
                                and busy_tid in statuses
                                and not (
                                    statuses[sleeper_tid]
                                    & THREAD_STATUS_ON_CPU
                                )
                                and not (
                                    statuses[sleeper_tid]
                                    & THREAD_STATUS_HAS_GIL
                                )
                                and (statuses[busy_tid] & THREAD_STATUS_ON_CPU)
                                and (
                                    statuses[busy_tid] & THREAD_STATUS_HAS_GIL
                                )
                            ):
                                break

                    self.assertIsNotNone(
                        sleeper_tid, "Sleeper thread id not received"
                    )
                    self.assertIsNotNone(
                        busy_tid, "Busy thread id not received"
                    )
                    self.assertIn(sleeper_tid, statuses)
                    self.assertIn(busy_tid, statuses)

                    # Sleeper: off CPU, no GIL
                    self.assertFalse(
                        statuses[sleeper_tid] & THREAD_STATUS_ON_CPU,
                        "Sleeper should be off CPU",
                    )
                    self.assertFalse(
                        statuses[sleeper_tid] & THREAD_STATUS_HAS_GIL,
                        "Sleeper should not have GIL",
                    )

                    # Busy: on CPU, has GIL
                    self.assertTrue(
                        statuses[busy_tid] & THREAD_STATUS_ON_CPU,
                        "Busy should be on CPU",
                    )
                    self.assertTrue(
                        statuses[busy_tid] & THREAD_STATUS_HAS_GIL,
                        "Busy should have GIL",
                    )
            finally:
                _cleanup_sockets(*client_sockets, server_socket)

    def _make_exception_test_script(self, port):
        """Create script with exception and normal threads for testing."""
        return textwrap.dedent(
            f"""\
            import socket
            import threading
            import time

            def exception_thread():
                conn = socket.create_connection(("localhost", {port}))
                conn.sendall(b"exception:" + str(threading.get_native_id()).encode())
                try:
                    raise ValueError("test exception")
                except ValueError:
                    while True:
                        time.sleep(0.01)

            def normal_thread():
                conn = socket.create_connection(("localhost", {port}))
                conn.sendall(b"normal:" + str(threading.get_native_id()).encode())
                while True:
                    sum(range(1000))

            t1 = threading.Thread(target=exception_thread)
            t2 = threading.Thread(target=normal_thread)
            t1.start()
            t2.start()
            t1.join()
            t2.join()
            """
        )

    @contextmanager
    def _run_exception_test_process(self):
        """Context manager to run exception test script and yield thread IDs and process."""
        port = find_unused_port()
        script = self._make_exception_test_script(port)

        with os_helper.temp_dir() as tmp_dir:
            script_file = make_script(tmp_dir, "script", script)
            server_socket = _create_server_socket(port, backlog=2)
            client_sockets = []

            try:
                with _managed_subprocess([sys.executable, script_file]) as p:
                    exception_tid = None
                    normal_tid = None

                    for _ in range(2):
                        client_socket, _ = server_socket.accept()
                        client_sockets.append(client_socket)
                        line = client_socket.recv(1024)
                        if line:
                            if line.startswith(b"exception:"):
                                try:
                                    exception_tid = int(line.split(b":")[-1])
                                except (ValueError, IndexError):
                                    pass
                            elif line.startswith(b"normal:"):
                                try:
                                    normal_tid = int(line.split(b":")[-1])
                                except (ValueError, IndexError):
                                    pass

                    server_socket.close()
                    server_socket = None

                    yield p, exception_tid, normal_tid
            finally:
                _cleanup_sockets(*client_sockets, server_socket)

    @unittest.skipIf(
        sys.platform not in ("linux", "darwin", "win32"),
        "Test only runs on supported platforms (Linux, macOS, or Windows)",
    )
    @unittest.skipIf(
        sys.platform == "android", "Android raises Linux-specific exception"
    )
    def test_thread_status_exception_detection(self):
        """Test that THREAD_STATUS_HAS_EXCEPTION is set when thread has an active exception."""
        with self._run_exception_test_process() as (p, exception_tid, normal_tid):
            self.assertIsNotNone(exception_tid, "Exception thread id not received")
            self.assertIsNotNone(normal_tid, "Normal thread id not received")

            statuses = {}
            unwinder = RemoteUnwinder(
                p.pid,
                all_threads=True,
                mode=PROFILING_MODE_ALL,
                skip_non_matching_threads=False,
            )
            for _ in busy_retry(SHORT_TIMEOUT):
                with contextlib.suppress(*TRANSIENT_ERRORS):
                    traces = unwinder.get_stack_trace()
                    statuses = self._get_thread_statuses(traces)

                    if (
                        exception_tid in statuses
                        and normal_tid in statuses
                        and (statuses[exception_tid] & THREAD_STATUS_HAS_EXCEPTION)
                        and not (statuses[normal_tid] & THREAD_STATUS_HAS_EXCEPTION)
                    ):
                        break

            self.assertIn(exception_tid, statuses)
            self.assertIn(normal_tid, statuses)
            self.assertTrue(
                statuses[exception_tid] & THREAD_STATUS_HAS_EXCEPTION,
                "Exception thread should have HAS_EXCEPTION flag",
            )
            self.assertFalse(
                statuses[normal_tid] & THREAD_STATUS_HAS_EXCEPTION,
                "Normal thread should not have HAS_EXCEPTION flag",
            )

    @unittest.skipIf(
        sys.platform not in ("linux", "darwin", "win32"),
        "Test only runs on supported platforms (Linux, macOS, or Windows)",
    )
    @unittest.skipIf(
        sys.platform == "android", "Android raises Linux-specific exception"
    )
    def test_thread_status_exception_mode_filtering(self):
        """Test that PROFILING_MODE_EXCEPTION correctly filters threads."""
        with self._run_exception_test_process() as (p, exception_tid, normal_tid):
            self.assertIsNotNone(exception_tid, "Exception thread id not received")
            self.assertIsNotNone(normal_tid, "Normal thread id not received")

            unwinder = RemoteUnwinder(
                p.pid,
                all_threads=True,
                mode=PROFILING_MODE_EXCEPTION,
                skip_non_matching_threads=True,
            )
            for _ in busy_retry(SHORT_TIMEOUT):
                with contextlib.suppress(*TRANSIENT_ERRORS):
                    traces = unwinder.get_stack_trace()
                    statuses = self._get_thread_statuses(traces)

                    if exception_tid in statuses:
                        self.assertNotIn(
                            normal_tid,
                            statuses,
                            "Normal thread should be filtered out in exception mode",
                        )
                        return

            self.fail("Never found exception thread in exception mode")

@requires_remote_subprocess_debugging()
class TestExceptionDetectionScenarios(RemoteInspectionTestBase):
    """Test exception detection across all scenarios.

    This class verifies the exact conditions under which THREAD_STATUS_HAS_EXCEPTION
    is set. Each test covers a specific scenario:

    1. except_block: Thread inside except block
       -> SHOULD have HAS_EXCEPTION (exc_info->exc_value is set)

    2. finally_propagating: Exception propagating through finally block
       -> SHOULD have HAS_EXCEPTION (current_exception is set)

    3. finally_after_except: Finally block after except handled exception
       -> Should NOT have HAS_EXCEPTION (exc_info cleared after except)

    4. finally_no_exception: Finally block with no exception raised
       -> Should NOT have HAS_EXCEPTION (no exception state)
    """

    def _make_single_scenario_script(self, port, scenario):
        """Create script for a single exception scenario."""
        scenarios = {
            "except_block": f"""\
import socket
import threading
import time

def target_thread():
    '''Inside except block - exception info is present'''
    conn = socket.create_connection(("localhost", {port}))
    conn.sendall(b"ready:" + str(threading.get_native_id()).encode())
    try:
        raise ValueError("test")
    except ValueError:
        while True:
            time.sleep(0.01)

t = threading.Thread(target=target_thread)
t.start()
t.join()
""",
            "finally_propagating": f"""\
import socket
import threading
import time

def target_thread():
    '''Exception propagating through finally - current_exception is set'''
    conn = socket.create_connection(("localhost", {port}))
    conn.sendall(b"ready:" + str(threading.get_native_id()).encode())
    try:
        try:
            raise ValueError("propagating")
        finally:
            # Exception is propagating through here
            while True:
                time.sleep(0.01)
    except:
        pass  # Never reached due to infinite loop

t = threading.Thread(target=target_thread)
t.start()
t.join()
""",
            "finally_after_except": f"""\
import socket
import threading
import time

def target_thread():
    '''Finally runs after except handled - exc_info is cleared'''
    conn = socket.create_connection(("localhost", {port}))
    conn.sendall(b"ready:" + str(threading.get_native_id()).encode())
    try:
        raise ValueError("test")
    except ValueError:
        pass  # Exception caught and handled
    finally:
        while True:
            time.sleep(0.01)

t = threading.Thread(target=target_thread)
t.start()
t.join()
""",
            "finally_no_exception": f"""\
import socket
import threading
import time

def target_thread():
    '''Finally with no exception at all'''
    conn = socket.create_connection(("localhost", {port}))
    conn.sendall(b"ready:" + str(threading.get_native_id()).encode())
    try:
        pass  # No exception
    finally:
        while True:
            time.sleep(0.01)

t = threading.Thread(target=target_thread)
t.start()
t.join()
""",
        }

        return scenarios[scenario]

    @contextmanager
    def _run_scenario_process(self, scenario):
        """Context manager to run a single scenario and yield thread ID and process."""
        port = find_unused_port()
        script = self._make_single_scenario_script(port, scenario)

        with os_helper.temp_dir() as tmp_dir:
            script_file = make_script(tmp_dir, "script", script)
            server_socket = _create_server_socket(port, backlog=1)
            client_socket = None

            try:
                with _managed_subprocess([sys.executable, script_file]) as p:
                    thread_tid = None

                    client_socket, _ = server_socket.accept()
                    line = client_socket.recv(1024)
                    if line and line.startswith(b"ready:"):
                        try:
                            thread_tid = int(line.split(b":")[-1])
                        except (ValueError, IndexError):
                            pass

                    server_socket.close()
                    server_socket = None

                    yield p, thread_tid
            finally:
                _cleanup_sockets(client_socket, server_socket)

    def _check_thread_status(
        self, p, thread_tid, condition, condition_name="condition"
    ):
        """Helper to check thread status with a custom condition.

        This waits until we see 3 consecutive samples where the condition
        returns True, which confirms the thread has reached and is stable
        in the expected state. Samples that don't match are ignored (the
        thread may not have reached the expected state yet).

        Args:
            p: Process object with pid attribute
            thread_tid: Thread ID to check
            condition: Callable(statuses, thread_tid) -> bool that returns
                       True when the thread is in the expected state
            condition_name: Description of condition for error messages
        """
        unwinder = RemoteUnwinder(
            p.pid,
            all_threads=True,
            mode=PROFILING_MODE_ALL,
            skip_non_matching_threads=False,
        )

        # Wait for 3 consecutive samples matching expected state
        matching_samples = 0
        for _ in busy_retry(SHORT_TIMEOUT):
            with contextlib.suppress(*TRANSIENT_ERRORS):
                traces = unwinder.get_stack_trace()
                statuses = self._get_thread_statuses(traces)

                if thread_tid in statuses:
                    if condition(statuses, thread_tid):
                        matching_samples += 1
                        if matching_samples >= 3:
                            return  # Success - confirmed stable in expected state
                    else:
                        # Thread not yet in expected state, reset counter
                        matching_samples = 0

        self.fail(
            f"Thread did not stabilize in expected state "
            f"({condition_name}) within timeout"
        )

    def _check_exception_status(self, p, thread_tid, expect_exception):
        """Helper to check if thread has expected exception status."""
        def condition(statuses, tid):
            has_exc = bool(statuses[tid] & THREAD_STATUS_HAS_EXCEPTION)
            return has_exc == expect_exception

        self._check_thread_status(
            p, thread_tid, condition,
            condition_name=f"expect_exception={expect_exception}"
        )

    @unittest.skipIf(
        sys.platform not in ("linux", "darwin", "win32"),
        "Test only runs on supported platforms (Linux, macOS, or Windows)",
    )
    @unittest.skipIf(
        sys.platform == "android", "Android raises Linux-specific exception"
    )
    def test_except_block_has_exception(self):
        """Test that thread inside except block has HAS_EXCEPTION flag.

        When a thread is executing inside an except block, exc_info->exc_value
        is set, so THREAD_STATUS_HAS_EXCEPTION should be True.
        """
        with self._run_scenario_process("except_block") as (p, thread_tid):
            self.assertIsNotNone(thread_tid, "Thread ID not received")
            self._check_exception_status(p, thread_tid, expect_exception=True)

    @unittest.skipIf(
        sys.platform not in ("linux", "darwin", "win32"),
        "Test only runs on supported platforms (Linux, macOS, or Windows)",
    )
    @unittest.skipIf(
        sys.platform == "android", "Android raises Linux-specific exception"
    )
    def test_finally_propagating_has_exception(self):
        """Test that finally block with propagating exception has HAS_EXCEPTION flag.

        When an exception is propagating through a finally block (not yet caught),
        current_exception is set, so THREAD_STATUS_HAS_EXCEPTION should be True.
        """
        with self._run_scenario_process("finally_propagating") as (p, thread_tid):
            self.assertIsNotNone(thread_tid, "Thread ID not received")
            self._check_exception_status(p, thread_tid, expect_exception=True)

    @unittest.skipIf(
        sys.platform not in ("linux", "darwin", "win32"),
        "Test only runs on supported platforms (Linux, macOS, or Windows)",
    )
    @unittest.skipIf(
        sys.platform == "android", "Android raises Linux-specific exception"
    )
    def test_finally_after_except_no_exception(self):
        """Test that finally block after except has NO HAS_EXCEPTION flag.

        When a finally block runs after an except block has handled the exception,
        Python clears exc_info before entering finally, so THREAD_STATUS_HAS_EXCEPTION
        should be False.
        """
        with self._run_scenario_process("finally_after_except") as (p, thread_tid):
            self.assertIsNotNone(thread_tid, "Thread ID not received")
            self._check_exception_status(p, thread_tid, expect_exception=False)

    @unittest.skipIf(
        sys.platform not in ("linux", "darwin", "win32"),
        "Test only runs on supported platforms (Linux, macOS, or Windows)",
    )
    @unittest.skipIf(
        sys.platform == "android", "Android raises Linux-specific exception"
    )
    def test_finally_no_exception_no_flag(self):
        """Test that finally block with no exception has NO HAS_EXCEPTION flag.

        When a finally block runs during normal execution (no exception raised),
        there is no exception state, so THREAD_STATUS_HAS_EXCEPTION should be False.
        """
        with self._run_scenario_process("finally_no_exception") as (p, thread_tid):
            self.assertIsNotNone(thread_tid, "Thread ID not received")
            self._check_exception_status(p, thread_tid, expect_exception=False)


@requires_remote_subprocess_debugging()
class TestFrameCaching(RemoteInspectionTestBase):
    """Test that frame caching produces correct results.

    Uses socket-based synchronization for deterministic testing.
    All tests verify cache reuse via object identity checks (assertIs).
    """

    @contextmanager
    def _target_process(self, script_body):
        """Context manager for running a target process with socket sync."""
        port = find_unused_port()
        script = f"""\
import socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('localhost', {port}))
{textwrap.dedent(script_body)}
"""

        with os_helper.temp_dir() as work_dir:
            script_dir = os.path.join(work_dir, "script_pkg")
            os.mkdir(script_dir)

            server_socket = _create_server_socket(port)
            script_name = _make_test_script(script_dir, "script", script)
            client_socket = None

            try:
                with _managed_subprocess([sys.executable, script_name]) as p:
                    client_socket, _ = server_socket.accept()
                    server_socket.close()
                    server_socket = None

                    def make_unwinder(cache_frames=True):
                        return RemoteUnwinder(
                            p.pid, all_threads=True, cache_frames=cache_frames
                        )

                    yield p, client_socket, make_unwinder
            finally:
                _cleanup_sockets(client_socket, server_socket)

    def _get_frames_with_retry(self, unwinder, required_funcs):
        """Get frames containing required_funcs, with retry for transient errors."""
        for _ in range(MAX_TRIES):
            with contextlib.suppress(*TRANSIENT_ERRORS):
                traces = unwinder.get_stack_trace()
                for interp in traces:
                    for thread in interp.threads:
                        funcs = {f.funcname for f in thread.frame_info}
                        if required_funcs.issubset(funcs):
                            return thread.frame_info
            time.sleep(RETRY_DELAY)
        return None

    def _sample_frames(
        self,
        client_socket,
        unwinder,
        wait_signal,
        send_ack,
        required_funcs,
        expected_frames=1,
    ):
        """Wait for signal, sample frames with retry until required funcs present, send ack."""
        _wait_for_signal(client_socket, wait_signal)
        frames = None
        for _ in range(MAX_TRIES):
            frames = self._get_frames_with_retry(unwinder, required_funcs)
            if frames and len(frames) >= expected_frames:
                break
            time.sleep(RETRY_DELAY)
        client_socket.sendall(send_ack)
        return frames

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    def test_cache_hit_same_stack(self):
        """Test that consecutive samples reuse cached parent frame objects.

        The current frame (index 0) is always re-read from memory to get
        updated line numbers, so it may be a different object. Parent frames
        (index 1+) should be identical objects from cache.
        """
        script_body = """\
            def level3():
                sock.sendall(b"sync1")
                sock.recv(16)
                sock.sendall(b"sync2")
                sock.recv(16)
                sock.sendall(b"sync3")
                sock.recv(16)

            def level2():
                level3()

            def level1():
                level2()

            level1()
            """

        with self._target_process(script_body) as (
            p,
            client_socket,
            make_unwinder,
        ):
            unwinder = make_unwinder(cache_frames=True)
            expected = {"level1", "level2", "level3"}

            frames1 = self._sample_frames(
                client_socket, unwinder, b"sync1", b"ack", expected
            )
            frames2 = self._sample_frames(
                client_socket, unwinder, b"sync2", b"ack", expected
            )
            frames3 = self._sample_frames(
                client_socket, unwinder, b"sync3", b"done", expected
            )

        self.assertIsNotNone(frames1)
        self.assertIsNotNone(frames2)
        self.assertIsNotNone(frames3)
        self.assertEqual(len(frames1), len(frames2))
        self.assertEqual(len(frames2), len(frames3))

        # Current frame (index 0) is always re-read, so check value equality
        self.assertEqual(frames1[0].funcname, frames2[0].funcname)
        self.assertEqual(frames2[0].funcname, frames3[0].funcname)

        # Parent frames (index 1+) must be identical objects (cache reuse)
        for i in range(1, len(frames1)):
            f1, f2, f3 = frames1[i], frames2[i], frames3[i]
            self.assertIs(
                f1, f2, f"Frame {i}: samples 1-2 must be same object"
            )
            self.assertIs(
                f2, f3, f"Frame {i}: samples 2-3 must be same object"
            )

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    def test_line_number_updates_in_same_frame(self):
        """Test that line numbers are correctly updated when execution moves within a function.

        When the profiler samples at different points within the same function,
        it must report the correct line number for each sample, not stale cached values.
        """
        script_body = """\
            def outer():
                inner()

            def inner():
                sock.sendall(b"line_a"); sock.recv(16)
                sock.sendall(b"line_b"); sock.recv(16)
                sock.sendall(b"line_c"); sock.recv(16)
                sock.sendall(b"line_d"); sock.recv(16)

            outer()
            """

        with self._target_process(script_body) as (
            p,
            client_socket,
            make_unwinder,
        ):
            unwinder = make_unwinder(cache_frames=True)

            frames_a = self._sample_frames(
                client_socket, unwinder, b"line_a", b"ack", {"inner"}
            )
            frames_b = self._sample_frames(
                client_socket, unwinder, b"line_b", b"ack", {"inner"}
            )
            frames_c = self._sample_frames(
                client_socket, unwinder, b"line_c", b"ack", {"inner"}
            )
            frames_d = self._sample_frames(
                client_socket, unwinder, b"line_d", b"done", {"inner"}
            )

        self.assertIsNotNone(frames_a)
        self.assertIsNotNone(frames_b)
        self.assertIsNotNone(frames_c)
        self.assertIsNotNone(frames_d)

        # Get the 'inner' frame from each sample (should be index 0)
        inner_a = frames_a[0]
        inner_b = frames_b[0]
        inner_c = frames_c[0]
        inner_d = frames_d[0]

        self.assertEqual(inner_a.funcname, "inner")
        self.assertEqual(inner_b.funcname, "inner")
        self.assertEqual(inner_c.funcname, "inner")
        self.assertEqual(inner_d.funcname, "inner")

        # Line numbers must be different and increasing (execution moves forward)
        self.assertLess(
            inner_a.location.lineno, inner_b.location.lineno, "Line B should be after line A"
        )
        self.assertLess(
            inner_b.location.lineno, inner_c.location.lineno, "Line C should be after line B"
        )
        self.assertLess(
            inner_c.location.lineno, inner_d.location.lineno, "Line D should be after line C"
        )

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    def test_cache_invalidation_on_return(self):
        """Test cache invalidation when stack shrinks (function returns)."""
        script_body = """\
            def inner():
                sock.sendall(b"at_inner")
                sock.recv(16)

            def outer():
                inner()
                sock.sendall(b"at_outer")
                sock.recv(16)

            outer()
            """

        with self._target_process(script_body) as (
            p,
            client_socket,
            make_unwinder,
        ):
            unwinder = make_unwinder(cache_frames=True)

            frames_deep = self._sample_frames(
                client_socket,
                unwinder,
                b"at_inner",
                b"ack",
                {"inner", "outer"},
            )
            frames_shallow = self._sample_frames(
                client_socket, unwinder, b"at_outer", b"done", {"outer"}
            )

        self.assertIsNotNone(frames_deep)
        self.assertIsNotNone(frames_shallow)

        funcs_deep = [f.funcname for f in frames_deep]
        funcs_shallow = [f.funcname for f in frames_shallow]

        self.assertIn("inner", funcs_deep)
        self.assertIn("outer", funcs_deep)
        self.assertNotIn("inner", funcs_shallow)
        self.assertIn("outer", funcs_shallow)

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    def test_cache_invalidation_on_call(self):
        """Test cache invalidation when stack grows (new function called)."""
        script_body = """\
            def deeper():
                sock.sendall(b"at_deeper")
                sock.recv(16)

            def middle():
                sock.sendall(b"at_middle")
                sock.recv(16)
                deeper()

            def top():
                middle()

            top()
            """

        with self._target_process(script_body) as (
            p,
            client_socket,
            make_unwinder,
        ):
            unwinder = make_unwinder(cache_frames=True)

            frames_before = self._sample_frames(
                client_socket,
                unwinder,
                b"at_middle",
                b"ack",
                {"middle", "top"},
            )
            frames_after = self._sample_frames(
                client_socket,
                unwinder,
                b"at_deeper",
                b"done",
                {"deeper", "middle", "top"},
            )

        self.assertIsNotNone(frames_before)
        self.assertIsNotNone(frames_after)

        funcs_before = [f.funcname for f in frames_before]
        funcs_after = [f.funcname for f in frames_after]

        self.assertIn("middle", funcs_before)
        self.assertIn("top", funcs_before)
        self.assertNotIn("deeper", funcs_before)

        self.assertIn("deeper", funcs_after)
        self.assertIn("middle", funcs_after)
        self.assertIn("top", funcs_after)

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    def test_partial_stack_reuse(self):
        """Test that unchanged parent frames are reused from cache when top frame moves."""
        script_body = """\
            def level4():
                sock.sendall(b"sync1")
                sock.recv(16)
                sock.sendall(b"sync2")
                sock.recv(16)

            def level3():
                level4()

            def level2():
                level3()

            def level1():
                level2()

            level1()
            """

        with self._target_process(script_body) as (
            p,
            client_socket,
            make_unwinder,
        ):
            unwinder = make_unwinder(cache_frames=True)

            # Sample 1: level4 at first sendall
            frames1 = self._sample_frames(
                client_socket,
                unwinder,
                b"sync1",
                b"ack",
                {"level1", "level2", "level3", "level4"},
            )
            # Sample 2: level4 at second sendall (same stack, different line)
            frames2 = self._sample_frames(
                client_socket,
                unwinder,
                b"sync2",
                b"done",
                {"level1", "level2", "level3", "level4"},
            )

        self.assertIsNotNone(frames1)
        self.assertIsNotNone(frames2)

        def find_frame(frames, funcname):
            for f in frames:
                if f.funcname == funcname:
                    return f
            return None

        # level4 should have different line numbers (it moved)
        l4_1 = find_frame(frames1, "level4")
        l4_2 = find_frame(frames2, "level4")
        self.assertIsNotNone(l4_1)
        self.assertIsNotNone(l4_2)
        self.assertNotEqual(
            l4_1.location.lineno,
            l4_2.location.lineno,
            "level4 should be at different lines",
        )

        # Parent frames (level1, level2, level3) should be reused from cache
        for name in ["level1", "level2", "level3"]:
            f1 = find_frame(frames1, name)
            f2 = find_frame(frames2, name)
            self.assertIsNotNone(f1, f"{name} missing from sample 1")
            self.assertIsNotNone(f2, f"{name} missing from sample 2")
            self.assertIs(f1, f2, f"{name} should be reused from cache")

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    def test_recursive_frames(self):
        """Test caching with same function appearing multiple times (recursion)."""
        script_body = """\
            def recurse(n):
                if n <= 0:
                    sock.sendall(b"sync1")
                    sock.recv(16)
                    sock.sendall(b"sync2")
                    sock.recv(16)
                else:
                    recurse(n - 1)

            recurse(5)
            """

        with self._target_process(script_body) as (
            p,
            client_socket,
            make_unwinder,
        ):
            unwinder = make_unwinder(cache_frames=True)

            frames1 = self._sample_frames(
                client_socket, unwinder, b"sync1", b"ack", {"recurse"}
            )
            frames2 = self._sample_frames(
                client_socket, unwinder, b"sync2", b"done", {"recurse"}
            )

        self.assertIsNotNone(frames1)
        self.assertIsNotNone(frames2)

        # Should have multiple "recurse" frames (6 total: recurse(5) down to recurse(0))
        recurse_count = sum(1 for f in frames1 if f.funcname == "recurse")
        self.assertEqual(recurse_count, 6, "Should have 6 recursive frames")

        self.assertEqual(len(frames1), len(frames2))

        # Current frame (index 0) is re-read, check value equality
        self.assertEqual(frames1[0].funcname, frames2[0].funcname)

        # Parent frames (index 1+) should be identical objects (cache reuse)
        for i in range(1, len(frames1)):
            self.assertIs(
                frames1[i],
                frames2[i],
                f"Frame {i}: recursive frames must be same object",
            )

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    def test_cache_vs_no_cache_equivalence(self):
        """Test that cache_frames=True and cache_frames=False produce equivalent results."""
        script_body = """\
            def level3():
                sock.sendall(b"ready"); sock.recv(16)

            def level2():
                level3()

            def level1():
                level2()

            level1()
            """

        with self._target_process(script_body) as (
            p,
            client_socket,
            make_unwinder,
        ):
            _wait_for_signal(client_socket, b"ready")

            # Sample with cache
            unwinder_cache = make_unwinder(cache_frames=True)
            frames_cached = self._get_frames_with_retry(
                unwinder_cache, {"level1", "level2", "level3"}
            )

            # Sample without cache
            unwinder_no_cache = make_unwinder(cache_frames=False)
            frames_no_cache = self._get_frames_with_retry(
                unwinder_no_cache, {"level1", "level2", "level3"}
            )

            client_socket.sendall(b"done")

        self.assertIsNotNone(frames_cached)
        self.assertIsNotNone(frames_no_cache)

        # Same number of frames
        self.assertEqual(len(frames_cached), len(frames_no_cache))

        # Same function names in same order
        funcs_cached = [f.funcname for f in frames_cached]
        funcs_no_cache = [f.funcname for f in frames_no_cache]
        self.assertEqual(funcs_cached, funcs_no_cache)

        # For level3 (leaf frame), due to timing races we can be at either
        # sock.sendall() or sock.recv() - both are valid. For parent frames,
        # the locations should match exactly.
        # Valid locations for level3: line 3 has two statements
        #   sock.sendall(b"ready") -> col 4-26
        #   sock.recv(16) -> col 28-41
        level3_valid_cols = {(4, 26), (28, 41)}

        for i in range(len(frames_cached)):
            loc_cached = frames_cached[i].location
            loc_no_cache = frames_no_cache[i].location

            if frames_cached[i].funcname == "level3":
                # Leaf frame: can be at either statement
                self.assertIn(
                    (loc_cached.col_offset, loc_cached.end_col_offset),
                    level3_valid_cols,
                )
                self.assertIn(
                    (loc_no_cache.col_offset, loc_no_cache.end_col_offset),
                    level3_valid_cols,
                )
            else:
                # Parent frames: must match exactly
                self.assertEqual(loc_cached, loc_no_cache)

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    def test_cache_per_thread_isolation(self):
        """Test that frame cache is per-thread and cache invalidation works independently."""
        script_body = """\
            import threading

            lock = threading.Lock()

            def sync(msg):
                with lock:
                    sock.sendall(msg + b"\\n")
                    sock.recv(1)

            # Thread 1 functions
            def baz1():
                sync(b"t1:baz1")

            def bar1():
                baz1()

            def blech1():
                sync(b"t1:blech1")

            def foo1():
                bar1()  # Goes down to baz1, syncs
                blech1()  # Returns up, goes down to blech1, syncs

            # Thread 2 functions
            def baz2():
                sync(b"t2:baz2")

            def bar2():
                baz2()

            def blech2():
                sync(b"t2:blech2")

            def foo2():
                bar2()  # Goes down to baz2, syncs
                blech2()  # Returns up, goes down to blech2, syncs

            t1 = threading.Thread(target=foo1)
            t2 = threading.Thread(target=foo2)
            t1.start()
            t2.start()
            t1.join()
            t2.join()
            """

        with self._target_process(script_body) as (
            p,
            client_socket,
            make_unwinder,
        ):
            unwinder = make_unwinder(cache_frames=True)

            # Message dispatch table: signal -> required functions for that thread
            dispatch = {
                b"t1:baz1": {"baz1", "bar1", "foo1"},
                b"t2:baz2": {"baz2", "bar2", "foo2"},
                b"t1:blech1": {"blech1", "foo1"},
                b"t2:blech2": {"blech2", "foo2"},
            }

            # Track results for each sync point
            results = {}

            # Process 4 sync points (order depends on thread scheduling)
            buffer = _wait_for_signal(client_socket, b"\n")
            for i in range(4):
                # Extract first message from buffer
                msg, sep, buffer = buffer.partition(b"\n")
                self.assertIn(msg, dispatch, f"Unexpected message: {msg!r}")

                # Sample frames for the thread at this sync point
                required_funcs = dispatch[msg]
                frames = self._get_frames_with_retry(unwinder, required_funcs)
                self.assertIsNotNone(frames, f"Thread not found for {msg!r}")
                results[msg] = [f.funcname for f in frames]

                # Release thread and wait for next message (if not last)
                client_socket.sendall(b"k")
                if i < 3:
                    buffer += _wait_for_signal(client_socket, b"\n")

            # Validate Phase 1: baz snapshots
            t1_baz = results.get(b"t1:baz1")
            t2_baz = results.get(b"t2:baz2")
            self.assertIsNotNone(t1_baz, "Missing t1:baz1 snapshot")
            self.assertIsNotNone(t2_baz, "Missing t2:baz2 snapshot")

            # Thread 1 at baz1: should have foo1->bar1->baz1
            self.assertIn("baz1", t1_baz)
            self.assertIn("bar1", t1_baz)
            self.assertIn("foo1", t1_baz)
            self.assertNotIn("blech1", t1_baz)
            # No cross-contamination
            self.assertNotIn("baz2", t1_baz)
            self.assertNotIn("bar2", t1_baz)
            self.assertNotIn("foo2", t1_baz)

            # Thread 2 at baz2: should have foo2->bar2->baz2
            self.assertIn("baz2", t2_baz)
            self.assertIn("bar2", t2_baz)
            self.assertIn("foo2", t2_baz)
            self.assertNotIn("blech2", t2_baz)
            # No cross-contamination
            self.assertNotIn("baz1", t2_baz)
            self.assertNotIn("bar1", t2_baz)
            self.assertNotIn("foo1", t2_baz)

            # Validate Phase 2: blech snapshots (cache invalidation test)
            t1_blech = results.get(b"t1:blech1")
            t2_blech = results.get(b"t2:blech2")
            self.assertIsNotNone(t1_blech, "Missing t1:blech1 snapshot")
            self.assertIsNotNone(t2_blech, "Missing t2:blech2 snapshot")

            # Thread 1 at blech1: bar1/baz1 should be GONE (cache invalidated)
            self.assertIn("blech1", t1_blech)
            self.assertIn("foo1", t1_blech)
            self.assertNotIn(
                "bar1", t1_blech, "Cache not invalidated: bar1 still present"
            )
            self.assertNotIn(
                "baz1", t1_blech, "Cache not invalidated: baz1 still present"
            )
            # No cross-contamination
            self.assertNotIn("blech2", t1_blech)

            # Thread 2 at blech2: bar2/baz2 should be GONE (cache invalidated)
            self.assertIn("blech2", t2_blech)
            self.assertIn("foo2", t2_blech)
            self.assertNotIn(
                "bar2", t2_blech, "Cache not invalidated: bar2 still present"
            )
            self.assertNotIn(
                "baz2", t2_blech, "Cache not invalidated: baz2 still present"
            )
            # No cross-contamination
            self.assertNotIn("blech1", t2_blech)

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    def test_new_unwinder_with_stale_last_profiled_frame(self):
        """Test that a new unwinder returns complete stack when cache lookup misses."""
        script_body = """\
            def level4():
                sock.sendall(b"sync1")
                sock.recv(16)
                sock.sendall(b"sync2")
                sock.recv(16)

            def level3():
                level4()

            def level2():
                level3()

            def level1():
                level2()

            level1()
            """

        with self._target_process(script_body) as (
            p,
            client_socket,
            make_unwinder,
        ):
            expected = {"level1", "level2", "level3", "level4"}

            # First unwinder samples - this sets last_profiled_frame in target
            unwinder1 = make_unwinder(cache_frames=True)
            frames1 = self._sample_frames(
                client_socket, unwinder1, b"sync1", b"ack", expected
            )

            # Create NEW unwinder (empty cache) and sample
            # The target still has last_profiled_frame set from unwinder1
            unwinder2 = make_unwinder(cache_frames=True)
            frames2 = self._sample_frames(
                client_socket, unwinder2, b"sync2", b"done", expected
            )

        self.assertIsNotNone(frames1)
        self.assertIsNotNone(frames2)

        funcs1 = [f.funcname for f in frames1]
        funcs2 = [f.funcname for f in frames2]

        # Both should have all levels
        for level in ["level1", "level2", "level3", "level4"]:
            self.assertIn(level, funcs1, f"{level} missing from first sample")
            self.assertIn(level, funcs2, f"{level} missing from second sample")

        # Should have same stack depth
        self.assertEqual(
            len(frames1),
            len(frames2),
            "New unwinder should return complete stack despite stale last_profiled_frame",
        )

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    def test_cache_exhaustion(self):
        """Test cache works when frame limit (1024) is exceeded.

        FRAME_CACHE_MAX_FRAMES=1024. With 1100 recursive frames,
        the cache can't store all of them but should still work.
        """
        # Use 1100 to exceed FRAME_CACHE_MAX_FRAMES=1024
        depth = 1100
        script_body = f"""\
import sys
sys.setrecursionlimit(2000)

def recurse(n):
    if n <= 0:
        sock.sendall(b"ready")
        sock.recv(16)  # wait for ack
        sock.sendall(b"ready2")
        sock.recv(16)  # wait for done
        return
    recurse(n - 1)

recurse({depth})
"""

        with self._target_process(script_body) as (
            p,
            client_socket,
            make_unwinder,
        ):
            unwinder_cache = make_unwinder(cache_frames=True)
            unwinder_no_cache = make_unwinder(cache_frames=False)

            frames_cached = self._sample_frames(
                client_socket,
                unwinder_cache,
                b"ready",
                b"ack",
                {"recurse"},
                expected_frames=1102,
            )
            # Sample again with no cache for comparison
            frames_no_cache = self._sample_frames(
                client_socket,
                unwinder_no_cache,
                b"ready2",
                b"done",
                {"recurse"},
                expected_frames=1102,
            )

        self.assertIsNotNone(frames_cached)
        self.assertIsNotNone(frames_no_cache)

        # Both should have many recurse frames (> 1024 limit)
        cached_count = [f.funcname for f in frames_cached].count("recurse")
        no_cache_count = [f.funcname for f in frames_no_cache].count("recurse")

        self.assertGreater(
            cached_count, 1000, "Should have >1000 recurse frames"
        )
        self.assertGreater(
            no_cache_count, 1000, "Should have >1000 recurse frames"
        )

        # Both modes should produce same frame count
        self.assertEqual(
            len(frames_cached),
            len(frames_no_cache),
            "Cache exhaustion should not affect stack completeness",
        )

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    def test_get_stats(self):
        """Test that get_stats() returns statistics when stats=True."""
        script_body = """\
            sock.sendall(b"ready")
            sock.recv(16)
            """

        with self._target_process(script_body) as (p, client_socket, _):
            unwinder = RemoteUnwinder(p.pid, all_threads=True, stats=True)
            _wait_for_signal(client_socket, b"ready")

            # Take a sample
            _get_stack_trace_with_retry(unwinder)

            stats = unwinder.get_stats()
            client_socket.sendall(b"done")

        # Verify expected keys exist
        expected_keys = [
            "total_samples",
            "frame_cache_hits",
            "frame_cache_misses",
            "frame_cache_partial_hits",
            "frames_read_from_cache",
            "frames_read_from_memory",
            "frame_cache_hit_rate",
        ]
        for key in expected_keys:
            self.assertIn(key, stats)

        self.assertEqual(stats["total_samples"], 1)

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    def test_get_stats_disabled_raises(self):
        """Test that get_stats() raises RuntimeError when stats=False."""
        script_body = """\
            sock.sendall(b"ready")
            sock.recv(16)
            """

        with self._target_process(script_body) as (p, client_socket, _):
            unwinder = RemoteUnwinder(
                p.pid, all_threads=True
            )  # stats=False by default
            _wait_for_signal(client_socket, b"ready")

            with self.assertRaises(RuntimeError):
                unwinder.get_stats()

            client_socket.sendall(b"done")


if __name__ == "__main__":
    unittest.main()
