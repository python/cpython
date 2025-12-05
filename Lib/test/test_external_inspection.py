import contextlib
import unittest
import os
import textwrap
import importlib
import sys
import socket
import threading
import time
from asyncio import staggered, taskgroups, base_events, tasks
from unittest.mock import ANY
from test.support import (
    os_helper,
    SHORT_TIMEOUT,
    busy_retry,
    requires_gil_enabled,
)
from test.support.script_helper import make_script
from test.support.socket_helper import find_unused_port

import subprocess

# Profiling mode constants
PROFILING_MODE_WALL = 0
PROFILING_MODE_CPU = 1
PROFILING_MODE_GIL = 2
PROFILING_MODE_ALL = 3

# Thread status flags
THREAD_STATUS_HAS_GIL = (1 << 0)
THREAD_STATUS_ON_CPU = (1 << 1)
THREAD_STATUS_UNKNOWN = (1 << 2)

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


def _make_test_script(script_dir, script_basename, source):
    to_return = make_script(script_dir, script_basename, source)
    importlib.invalidate_caches()
    return to_return


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
    return unittest.skipIf(interpreters is None,
                           'subinterpreters required')(meth)


def get_stack_trace(pid):
    unwinder = RemoteUnwinder(pid, all_threads=True, debug=True)
    return unwinder.get_stack_trace()


def get_async_stack_trace(pid):
    unwinder = RemoteUnwinder(pid, debug=True)
    return unwinder.get_async_stack_trace()


def get_all_awaited_by(pid):
    unwinder = RemoteUnwinder(pid, debug=True)
    return unwinder.get_all_awaited_by()


class TestGetStackTrace(unittest.TestCase):
    maxDiff = None

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    def test_remote_stack_trace(self):
        # Spawn a process with some realistic Python code
        port = find_unused_port()
        script = textwrap.dedent(
            f"""\
            import time, sys, socket, threading
            # Connect to the test process
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(('localhost', {port}))

            def bar():
                for x in range(100):
                    if x == 50:
                        baz()

            def baz():
                foo()

            def foo():
                sock.sendall(b"ready:thread\\n"); time.sleep(10_000)  # same line number

            t = threading.Thread(target=bar)
            t.start()
            sock.sendall(b"ready:main\\n"); t.join()  # same line number
            """
        )
        stack_trace = None
        with os_helper.temp_dir() as work_dir:
            script_dir = os.path.join(work_dir, "script_pkg")
            os.mkdir(script_dir)

            # Create a socket server to communicate with the target process
            server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server_socket.bind(("localhost", port))
            server_socket.settimeout(SHORT_TIMEOUT)
            server_socket.listen(1)

            script_name = _make_test_script(script_dir, "script", script)
            client_socket = None
            try:
                p = subprocess.Popen([sys.executable, script_name])
                client_socket, _ = server_socket.accept()
                server_socket.close()
                response = b""
                while (
                    b"ready:main" not in response
                    or b"ready:thread" not in response
                ):
                    response += client_socket.recv(1024)
                stack_trace = get_stack_trace(p.pid)
            except PermissionError:
                self.skipTest(
                    "Insufficient permissions to read the stack trace"
                )
            finally:
                if client_socket is not None:
                    client_socket.close()
                p.kill()
                p.terminate()
                p.wait(timeout=SHORT_TIMEOUT)

            thread_expected_stack_trace = [
                FrameInfo([script_name, 15, "foo"]),
                FrameInfo([script_name, 12, "baz"]),
                FrameInfo([script_name, 9, "bar"]),
                FrameInfo([threading.__file__, ANY, "Thread.run"]),
                FrameInfo([threading.__file__, ANY, "Thread._bootstrap_inner"]),
                FrameInfo([threading.__file__, ANY, "Thread._bootstrap"]),
            ]
            # Is possible that there are more threads, so we check that the
            # expected stack traces are in the result (looking at you Windows!)
            found_expected_stack = False
            for interpreter_info in stack_trace:
                for thread_info in interpreter_info.threads:
                    if thread_info.frame_info == thread_expected_stack_trace:
                        found_expected_stack = True
                        break
                if found_expected_stack:
                    break
            self.assertTrue(found_expected_stack, "Expected thread stack trace not found")

            # Check that the main thread stack trace is in the result
            frame = FrameInfo([script_name, 19, "<module>"])
            main_thread_found = False
            for interpreter_info in stack_trace:
                for thread_info in interpreter_info.threads:
                    if frame in thread_info.frame_info:
                        main_thread_found = True
                        break
                if main_thread_found:
                    break
            self.assertTrue(main_thread_found, "Main thread stack trace not found in result")

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    def test_async_remote_stack_trace(self):
        # Spawn a process with some realistic Python code
        port = find_unused_port()
        script = textwrap.dedent(
            f"""\
            import asyncio
            import time
            import sys
            import socket
            # Connect to the test process
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(('localhost', {port}))

            def c5():
                sock.sendall(b"ready"); time.sleep(10_000)  # same line number

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
        stack_trace = None
        for task_factory_variant in "asyncio.new_event_loop", "new_eager_loop":
            with (
                self.subTest(task_factory_variant=task_factory_variant),
                os_helper.temp_dir() as work_dir,
            ):
                script_dir = os.path.join(work_dir, "script_pkg")
                os.mkdir(script_dir)
                server_socket = socket.socket(
                    socket.AF_INET, socket.SOCK_STREAM
                )
                server_socket.setsockopt(
                    socket.SOL_SOCKET, socket.SO_REUSEADDR, 1
                )
                server_socket.bind(("localhost", port))
                server_socket.settimeout(SHORT_TIMEOUT)
                server_socket.listen(1)
                script_name = _make_test_script(
                    script_dir,
                    "script",
                    script.format(TASK_FACTORY=task_factory_variant),
                )
                client_socket = None
                try:
                    p = subprocess.Popen([sys.executable, script_name])
                    client_socket, _ = server_socket.accept()
                    server_socket.close()
                    response = client_socket.recv(1024)
                    self.assertEqual(response, b"ready")
                    stack_trace = get_async_stack_trace(p.pid)
                except PermissionError:
                    self.skipTest(
                        "Insufficient permissions to read the stack trace"
                    )
                finally:
                    if client_socket is not None:
                        client_socket.close()
                    p.kill()
                    p.terminate()
                    p.wait(timeout=SHORT_TIMEOUT)

                # First check all the tasks are present
                tasks_names = [
                    task.task_name for task in stack_trace[0].awaited_by
                ]
                for task_name in ["c2_root", "sub_main_1", "sub_main_2"]:
                    self.assertIn(task_name, tasks_names)

                # Now ensure that the awaited_by_relationships are correct
                id_to_task = {
                    task.task_id: task for task in stack_trace[0].awaited_by
                }
                task_name_to_awaited_by = {
                    task.task_name: set(
                        id_to_task[awaited.task_name].task_name
                        for awaited in task.awaited_by
                    )
                    for task in stack_trace[0].awaited_by
                }
                self.assertEqual(
                    task_name_to_awaited_by,
                    {
                        "c2_root": {"Task-1", "sub_main_1", "sub_main_2"},
                        "Task-1": set(),
                        "sub_main_1": {"Task-1"},
                        "sub_main_2": {"Task-1"},
                    },
                )

                # Now ensure that the coroutine stacks are correct
                coroutine_stacks = {
                    task.task_name: sorted(
                        tuple(tuple(frame) for frame in coro.call_stack)
                        for coro in task.coroutine_stack
                    )
                    for task in stack_trace[0].awaited_by
                }
                self.assertEqual(
                    coroutine_stacks,
                    {
                        "Task-1": [
                            (
                                tuple(
                                    [
                                        taskgroups.__file__,
                                        ANY,
                                        "TaskGroup._aexit",
                                    ]
                                ),
                                tuple(
                                    [
                                        taskgroups.__file__,
                                        ANY,
                                        "TaskGroup.__aexit__",
                                    ]
                                ),
                                tuple([script_name, 26, "main"]),
                            )
                        ],
                        "c2_root": [
                            (
                                tuple([script_name, 10, "c5"]),
                                tuple([script_name, 14, "c4"]),
                                tuple([script_name, 17, "c3"]),
                                tuple([script_name, 20, "c2"]),
                            )
                        ],
                        "sub_main_1": [(tuple([script_name, 23, "c1"]),)],
                        "sub_main_2": [(tuple([script_name, 23, "c1"]),)],
                    },
                )

                # Now ensure the coroutine stacks for the awaited_by relationships are correct.
                awaited_by_coroutine_stacks = {
                    task.task_name: sorted(
                        (
                            id_to_task[coro.task_name].task_name,
                            tuple(tuple(frame) for frame in coro.call_stack),
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
                                    tuple(
                                        [
                                            taskgroups.__file__,
                                            ANY,
                                            "TaskGroup._aexit",
                                        ]
                                    ),
                                    tuple(
                                        [
                                            taskgroups.__file__,
                                            ANY,
                                            "TaskGroup.__aexit__",
                                        ]
                                    ),
                                    tuple([script_name, 26, "main"]),
                                ),
                            ),
                            ("sub_main_1", (tuple([script_name, 23, "c1"]),)),
                            ("sub_main_2", (tuple([script_name, 23, "c1"]),)),
                        ],
                        "sub_main_1": [
                            (
                                "Task-1",
                                (
                                    tuple(
                                        [
                                            taskgroups.__file__,
                                            ANY,
                                            "TaskGroup._aexit",
                                        ]
                                    ),
                                    tuple(
                                        [
                                            taskgroups.__file__,
                                            ANY,
                                            "TaskGroup.__aexit__",
                                        ]
                                    ),
                                    tuple([script_name, 26, "main"]),
                                ),
                            )
                        ],
                        "sub_main_2": [
                            (
                                "Task-1",
                                (
                                    tuple(
                                        [
                                            taskgroups.__file__,
                                            ANY,
                                            "TaskGroup._aexit",
                                        ]
                                    ),
                                    tuple(
                                        [
                                            taskgroups.__file__,
                                            ANY,
                                            "TaskGroup.__aexit__",
                                        ]
                                    ),
                                    tuple([script_name, 26, "main"]),
                                ),
                            )
                        ],
                    },
                )

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    def test_asyncgen_remote_stack_trace(self):
        # Spawn a process with some realistic Python code
        port = find_unused_port()
        script = textwrap.dedent(
            f"""\
            import asyncio
            import time
            import sys
            import socket
            # Connect to the test process
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(('localhost', {port}))

            async def gen_nested_call():
                sock.sendall(b"ready"); time.sleep(10_000)  # same line number

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
        stack_trace = None
        with os_helper.temp_dir() as work_dir:
            script_dir = os.path.join(work_dir, "script_pkg")
            os.mkdir(script_dir)
            # Create a socket server to communicate with the target process
            server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server_socket.bind(("localhost", port))
            server_socket.settimeout(SHORT_TIMEOUT)
            server_socket.listen(1)
            script_name = _make_test_script(script_dir, "script", script)
            client_socket = None
            try:
                p = subprocess.Popen([sys.executable, script_name])
                client_socket, _ = server_socket.accept()
                server_socket.close()
                response = client_socket.recv(1024)
                self.assertEqual(response, b"ready")
                stack_trace = get_async_stack_trace(p.pid)
            except PermissionError:
                self.skipTest(
                    "Insufficient permissions to read the stack trace"
                )
            finally:
                if client_socket is not None:
                    client_socket.close()
                p.kill()
                p.terminate()
                p.wait(timeout=SHORT_TIMEOUT)

            # For this simple asyncgen test, we only expect one task with the full coroutine stack
            self.assertEqual(len(stack_trace[0].awaited_by), 1)
            task = stack_trace[0].awaited_by[0]
            self.assertEqual(task.task_name, "Task-1")

            # Check the coroutine stack - based on actual output, only shows main
            coroutine_stack = sorted(
                tuple(tuple(frame) for frame in coro.call_stack)
                for coro in task.coroutine_stack
            )
            self.assertEqual(
                coroutine_stack,
                [
                    (
                        tuple([script_name, 10, "gen_nested_call"]),
                        tuple([script_name, 16, "gen"]),
                        tuple([script_name, 19, "main"]),
                    )
                ],
            )

            # No awaited_by relationships expected for this simple case
            self.assertEqual(task.awaited_by, [])

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    def test_async_gather_remote_stack_trace(self):
        # Spawn a process with some realistic Python code
        port = find_unused_port()
        script = textwrap.dedent(
            f"""\
            import asyncio
            import time
            import sys
            import socket
            # Connect to the test process
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(('localhost', {port}))

            async def deep():
                await asyncio.sleep(0)
                sock.sendall(b"ready"); time.sleep(10_000)  # same line number

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
        stack_trace = None
        with os_helper.temp_dir() as work_dir:
            script_dir = os.path.join(work_dir, "script_pkg")
            os.mkdir(script_dir)
            # Create a socket server to communicate with the target process
            server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server_socket.bind(("localhost", port))
            server_socket.settimeout(SHORT_TIMEOUT)
            server_socket.listen(1)
            script_name = _make_test_script(script_dir, "script", script)
            client_socket = None
            try:
                p = subprocess.Popen([sys.executable, script_name])
                client_socket, _ = server_socket.accept()
                server_socket.close()
                response = client_socket.recv(1024)
                self.assertEqual(response, b"ready")
                stack_trace = get_async_stack_trace(p.pid)
            except PermissionError:
                self.skipTest(
                    "Insufficient permissions to read the stack trace"
                )
            finally:
                if client_socket is not None:
                    client_socket.close()
                p.kill()
                p.terminate()
                p.wait(timeout=SHORT_TIMEOUT)

            # First check all the tasks are present
            tasks_names = [
                task.task_name for task in stack_trace[0].awaited_by
            ]
            for task_name in ["Task-1", "Task-2"]:
                self.assertIn(task_name, tasks_names)

            # Now ensure that the awaited_by_relationships are correct
            id_to_task = {
                task.task_id: task for task in stack_trace[0].awaited_by
            }
            task_name_to_awaited_by = {
                task.task_name: set(
                    id_to_task[awaited.task_name].task_name
                    for awaited in task.awaited_by
                )
                for task in stack_trace[0].awaited_by
            }
            self.assertEqual(
                task_name_to_awaited_by,
                {
                    "Task-1": set(),
                    "Task-2": {"Task-1"},
                },
            )

            # Now ensure that the coroutine stacks are correct
            coroutine_stacks = {
                task.task_name: sorted(
                    tuple(tuple(frame) for frame in coro.call_stack)
                    for coro in task.coroutine_stack
                )
                for task in stack_trace[0].awaited_by
            }
            self.assertEqual(
                coroutine_stacks,
                {
                    "Task-1": [(tuple([script_name, 21, "main"]),)],
                    "Task-2": [
                        (
                            tuple([script_name, 11, "deep"]),
                            tuple([script_name, 15, "c1"]),
                        )
                    ],
                },
            )

            # Now ensure the coroutine stacks for the awaited_by relationships are correct.
            awaited_by_coroutine_stacks = {
                task.task_name: sorted(
                    (
                        id_to_task[coro.task_name].task_name,
                        tuple(tuple(frame) for frame in coro.call_stack),
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
                        ("Task-1", (tuple([script_name, 21, "main"]),))
                    ],
                },
            )

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    def test_async_staggered_race_remote_stack_trace(self):
        # Spawn a process with some realistic Python code
        port = find_unused_port()
        script = textwrap.dedent(
            f"""\
            import asyncio.staggered
            import time
            import sys
            import socket
            # Connect to the test process
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(('localhost', {port}))

            async def deep():
                await asyncio.sleep(0)
                sock.sendall(b"ready"); time.sleep(10_000)  # same line number

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
        stack_trace = None
        with os_helper.temp_dir() as work_dir:
            script_dir = os.path.join(work_dir, "script_pkg")
            os.mkdir(script_dir)
            # Create a socket server to communicate with the target process
            server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server_socket.bind(("localhost", port))
            server_socket.settimeout(SHORT_TIMEOUT)
            server_socket.listen(1)
            script_name = _make_test_script(script_dir, "script", script)
            client_socket = None
            try:
                p = subprocess.Popen([sys.executable, script_name])
                client_socket, _ = server_socket.accept()
                server_socket.close()
                response = client_socket.recv(1024)
                self.assertEqual(response, b"ready")
                stack_trace = get_async_stack_trace(p.pid)
            except PermissionError:
                self.skipTest(
                    "Insufficient permissions to read the stack trace"
                )
            finally:
                if client_socket is not None:
                    client_socket.close()
                p.kill()
                p.terminate()
                p.wait(timeout=SHORT_TIMEOUT)

            # First check all the tasks are present
            tasks_names = [
                task.task_name for task in stack_trace[0].awaited_by
            ]
            for task_name in ["Task-1", "Task-2"]:
                self.assertIn(task_name, tasks_names)

            # Now ensure that the awaited_by_relationships are correct
            id_to_task = {
                task.task_id: task for task in stack_trace[0].awaited_by
            }
            task_name_to_awaited_by = {
                task.task_name: set(
                    id_to_task[awaited.task_name].task_name
                    for awaited in task.awaited_by
                )
                for task in stack_trace[0].awaited_by
            }
            self.assertEqual(
                task_name_to_awaited_by,
                {
                    "Task-1": set(),
                    "Task-2": {"Task-1"},
                },
            )

            # Now ensure that the coroutine stacks are correct
            coroutine_stacks = {
                task.task_name: sorted(
                    tuple(tuple(frame) for frame in coro.call_stack)
                    for coro in task.coroutine_stack
                )
                for task in stack_trace[0].awaited_by
            }
            self.assertEqual(
                coroutine_stacks,
                {
                    "Task-1": [
                        (
                            tuple([staggered.__file__, ANY, "staggered_race"]),
                            tuple([script_name, 21, "main"]),
                        )
                    ],
                    "Task-2": [
                        (
                            tuple([script_name, 11, "deep"]),
                            tuple([script_name, 15, "c1"]),
                            tuple(
                                [
                                    staggered.__file__,
                                    ANY,
                                    "staggered_race.<locals>.run_one_coro",
                                ]
                            ),
                        )
                    ],
                },
            )

            # Now ensure the coroutine stacks for the awaited_by relationships are correct.
            awaited_by_coroutine_stacks = {
                task.task_name: sorted(
                    (
                        id_to_task[coro.task_name].task_name,
                        tuple(tuple(frame) for frame in coro.call_stack),
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
                                tuple(
                                    [staggered.__file__, ANY, "staggered_race"]
                                ),
                                tuple([script_name, 21, "main"]),
                            ),
                        )
                    ],
                },
            )

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    def test_async_global_awaited_by(self):
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

            # Connect to the test process
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
                # Signal we are ready to sleep
                sock.sendall(b"ready")
                await asyncio.sleep(SHORT_TIMEOUT)

            async def echo_client_spam(server):
                async with asyncio.TaskGroup() as tg:
                    while connections < 1000:
                        msg = list(ascii_lowercase + digits)
                        random.shuffle(msg)
                        tg.create_task(echo_client("".join(msg)))
                        await asyncio.sleep(0)
                    # at least a 1000 tasks created. Each task will signal
                    # when is ready to avoid the race caused by the fact that
                    # tasks are waited on tg.__exit__ and we cannot signal when
                    # that happens otherwise
                # at this point all client tasks completed without assertion errors
                # let's wrap up the test
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
        stack_trace = None
        with os_helper.temp_dir() as work_dir:
            script_dir = os.path.join(work_dir, "script_pkg")
            os.mkdir(script_dir)
            # Create a socket server to communicate with the target process
            server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server_socket.bind(("localhost", port))
            server_socket.settimeout(SHORT_TIMEOUT)
            server_socket.listen(1)
            script_name = _make_test_script(script_dir, "script", script)
            client_socket = None
            try:
                p = subprocess.Popen([sys.executable, script_name])
                client_socket, _ = server_socket.accept()
                server_socket.close()
                for _ in range(1000):
                    expected_response = b"ready"
                    response = client_socket.recv(len(expected_response))
                    self.assertEqual(response, expected_response)
                for _ in busy_retry(SHORT_TIMEOUT):
                    try:
                        all_awaited_by = get_all_awaited_by(p.pid)
                    except RuntimeError as re:
                        # This call reads a linked list in another process with
                        # no synchronization. That occasionally leads to invalid
                        # reads. Here we avoid making the test flaky.
                        msg = str(re)
                        if msg.startswith("Task list appears corrupted"):
                            continue
                        elif msg.startswith(
                            "Invalid linked list structure reading remote memory"
                        ):
                            continue
                        elif msg.startswith("Unknown error reading memory"):
                            continue
                        elif msg.startswith("Unhandled frame owner"):
                            continue
                        raise  # Unrecognized exception, safest not to ignore it
                    else:
                        break
                # expected: a list of two elements: 1 thread, 1 interp
                self.assertEqual(len(all_awaited_by), 2)
                # expected: a tuple with the thread ID and the awaited_by list
                self.assertEqual(len(all_awaited_by[0]), 2)
                # expected: no tasks in the fallback per-interp task list
                self.assertEqual(all_awaited_by[1], (0, []))
                entries = all_awaited_by[0][1]
                # expected: at least 1000 pending tasks
                self.assertGreaterEqual(len(entries), 1000)
                # the first three tasks stem from the code structure
                main_stack = [
                    FrameInfo([taskgroups.__file__, ANY, "TaskGroup._aexit"]),
                    FrameInfo(
                        [taskgroups.__file__, ANY, "TaskGroup.__aexit__"]
                    ),
                    FrameInfo([script_name, 60, "main"]),
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
                                                ]
                                            ),
                                            FrameInfo(
                                                [
                                                    taskgroups.__file__,
                                                    ANY,
                                                    "TaskGroup.__aexit__",
                                                ]
                                            ),
                                            FrameInfo(
                                                [script_name, ANY, "main"]
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
                                                [tasks.__file__, ANY, "sleep"]
                                            ),
                                            FrameInfo(
                                                [
                                                    script_name,
                                                    38,
                                                    "echo_client",
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
                                                ]
                                            ),
                                            FrameInfo(
                                                [
                                                    taskgroups.__file__,
                                                    ANY,
                                                    "TaskGroup.__aexit__",
                                                ]
                                            ),
                                            FrameInfo(
                                                [
                                                    script_name,
                                                    41,
                                                    "echo_client_spam",
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

                expected_awaited_by = [
                    CoroInfo(
                        [
                            [
                                FrameInfo(
                                    [
                                        taskgroups.__file__,
                                        ANY,
                                        "TaskGroup._aexit",
                                    ]
                                ),
                                FrameInfo(
                                    [
                                        taskgroups.__file__,
                                        ANY,
                                        "TaskGroup.__aexit__",
                                    ]
                                ),
                                FrameInfo(
                                    [script_name, 41, "echo_client_spam"]
                                ),
                            ],
                            ANY,
                        ]
                    )
                ]
                tasks_with_awaited = [
                    task
                    for task in entries
                    if task.awaited_by == expected_awaited_by
                ]
                self.assertGreaterEqual(len(tasks_with_awaited), 1000)

                # the final task will have some random number, but it should for
                # sure be one of the echo client spam horde (In windows this is not true
                # for some reason)
                if sys.platform != "win32":
                    self.assertEqual(
                        tasks_with_awaited[-1].awaited_by,
                        entries[-1].awaited_by,
                    )
            except PermissionError:
                self.skipTest(
                    "Insufficient permissions to read the stack trace"
                )
            finally:
                if client_socket is not None:
                    client_socket.close()
                p.kill()
                p.terminate()
                p.wait(timeout=SHORT_TIMEOUT)

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    def test_self_trace(self):
        stack_trace = get_stack_trace(os.getpid())
        # Is possible that there are more threads, so we check that the
        # expected stack traces are in the result (looking at you Windows!)
        this_tread_stack = None
        # New format: [InterpreterInfo(interpreter_id, [ThreadInfo(...)])]
        for interpreter_info in stack_trace:
            for thread_info in interpreter_info.threads:
                if thread_info.thread_id == threading.get_native_id():
                    this_tread_stack = thread_info.frame_info
                    break
            if this_tread_stack:
                break
        self.assertIsNotNone(this_tread_stack)
        self.assertEqual(
            this_tread_stack[:2],
            [
                FrameInfo(
                    [
                        __file__,
                        get_stack_trace.__code__.co_firstlineno + 2,
                        "get_stack_trace",
                    ]
                ),
                FrameInfo(
                    [
                        __file__,
                        self.test_self_trace.__code__.co_firstlineno + 6,
                        "TestGetStackTrace.test_self_trace",
                    ]
                ),
            ],
        )

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    @requires_subinterpreters
    def test_subinterpreter_stack_trace(self):
        # Test that subinterpreters are correctly handled
        port = find_unused_port()

        # Calculate subinterpreter code separately and pickle it to avoid f-string issues
        import pickle
        subinterp_code = textwrap.dedent(f'''
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
        ''').strip()

        # Pickle the subinterpreter code
        pickled_code = pickle.dumps(subinterp_code)

        script = textwrap.dedent(
            f"""
            from concurrent import interpreters
            import time
            import sys
            import socket
            import threading

            # Connect to the test process
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(('localhost', {port}))

            def main_worker():
                # Function running in main interpreter
                sock.sendall(b"ready:main\\n")
                time.sleep(10_000)

            def run_subinterp():
                # Create and run subinterpreter
                subinterp = interpreters.create()

                import pickle
                pickled_code = {pickled_code!r}
                subinterp_code = pickle.loads(pickled_code)
                subinterp.exec(subinterp_code)

            # Start subinterpreter in thread
            sub_thread = threading.Thread(target=run_subinterp)
            sub_thread.start()

            # Start main thread work
            main_thread = threading.Thread(target=main_worker)
            main_thread.start()

            # Keep main thread alive
            main_thread.join()
            sub_thread.join()
            """
        )

        with os_helper.temp_dir() as work_dir:
            script_dir = os.path.join(work_dir, "script_pkg")
            os.mkdir(script_dir)

            # Create a socket server to communicate with the target process
            server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server_socket.bind(("localhost", port))
            server_socket.settimeout(SHORT_TIMEOUT)
            server_socket.listen(1)

            script_name = _make_test_script(script_dir, "script", script)
            client_sockets = []
            try:
                p = subprocess.Popen([sys.executable, script_name])

                # Accept connections from both main and subinterpreter
                responses = set()
                while len(responses) < 2:  # Wait for both "ready:main" and "ready:sub"
                    try:
                        client_socket, _ = server_socket.accept()
                        client_sockets.append(client_socket)

                        # Read the response from this connection
                        response = client_socket.recv(1024)
                        if b"ready:main" in response:
                            responses.add("main")
                        if b"ready:sub" in response:
                            responses.add("sub")
                    except socket.timeout:
                        break

                server_socket.close()
                stack_trace = get_stack_trace(p.pid)
            except PermissionError:
                self.skipTest(
                    "Insufficient permissions to read the stack trace"
                )
            finally:
                for client_socket in client_sockets:
                    if client_socket is not None:
                        client_socket.close()
                p.kill()
                p.terminate()
                p.wait(timeout=SHORT_TIMEOUT)

            # Verify we have multiple interpreters
            self.assertGreaterEqual(len(stack_trace), 1, "Should have at least one interpreter")

            # Look for main interpreter (ID 0) and subinterpreter (ID > 0)
            main_interp = None
            sub_interp = None

            for interpreter_info in stack_trace:
                if interpreter_info.interpreter_id == 0:
                    main_interp = interpreter_info
                elif interpreter_info.interpreter_id > 0:
                    sub_interp = interpreter_info

            self.assertIsNotNone(main_interp, "Main interpreter should be present")

            # Check main interpreter has expected stack trace
            main_found = False
            for thread_info in main_interp.threads:
                for frame in thread_info.frame_info:
                    if frame.funcname == "main_worker":
                        main_found = True
                        break
                if main_found:
                    break
            self.assertTrue(main_found, "Main interpreter should have main_worker in stack")

            # If subinterpreter is present, check its stack trace
            if sub_interp:
                sub_found = False
                for thread_info in sub_interp.threads:
                    for frame in thread_info.frame_info:
                        if frame.funcname in ("sub_worker", "nested_func"):
                            sub_found = True
                            break
                    if sub_found:
                        break
                self.assertTrue(sub_found, "Subinterpreter should have sub_worker or nested_func in stack")

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    @requires_subinterpreters
    def test_multiple_subinterpreters_with_threads(self):
        # Test multiple subinterpreters, each with multiple threads
        port = find_unused_port()

        # Calculate subinterpreter codes separately and pickle them
        import pickle

        # Code for first subinterpreter with 2 threads
        subinterp1_code = textwrap.dedent(f'''
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
        ''').strip()

        # Code for second subinterpreter with 2 threads
        subinterp2_code = textwrap.dedent(f'''
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
        ''').strip()

        # Pickle the subinterpreter codes
        pickled_code1 = pickle.dumps(subinterp1_code)
        pickled_code2 = pickle.dumps(subinterp2_code)

        script = textwrap.dedent(
            f"""
            from concurrent import interpreters
            import time
            import sys
            import socket
            import threading

            # Connect to the test process
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(('localhost', {port}))

            def main_worker():
                # Function running in main interpreter
                sock.sendall(b"ready:main\\n")
                time.sleep(10_000)

            def run_subinterp1():
                # Create and run first subinterpreter
                subinterp = interpreters.create()

                import pickle
                pickled_code = {pickled_code1!r}
                subinterp_code = pickle.loads(pickled_code)
                subinterp.exec(subinterp_code)

            def run_subinterp2():
                # Create and run second subinterpreter
                subinterp = interpreters.create()

                import pickle
                pickled_code = {pickled_code2!r}
                subinterp_code = pickle.loads(pickled_code)
                subinterp.exec(subinterp_code)

            # Start subinterpreters in threads
            sub1_thread = threading.Thread(target=run_subinterp1)
            sub2_thread = threading.Thread(target=run_subinterp2)
            sub1_thread.start()
            sub2_thread.start()

            # Start main thread work
            main_thread = threading.Thread(target=main_worker)
            main_thread.start()

            # Keep main thread alive
            main_thread.join()
            sub1_thread.join()
            sub2_thread.join()
            """
        )

        with os_helper.temp_dir() as work_dir:
            script_dir = os.path.join(work_dir, "script_pkg")
            os.mkdir(script_dir)

            # Create a socket server to communicate with the target process
            server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server_socket.bind(("localhost", port))
            server_socket.settimeout(SHORT_TIMEOUT)
            server_socket.listen(5)  # Allow multiple connections

            script_name = _make_test_script(script_dir, "script", script)
            client_sockets = []
            try:
                p = subprocess.Popen([sys.executable, script_name])

                # Accept connections from main and all subinterpreter threads
                expected_responses = {"ready:main", "ready:sub1-t1", "ready:sub1-t2", "ready:sub2-t1", "ready:sub2-t2"}
                responses = set()

                while len(responses) < 5:  # Wait for all 5 ready signals
                    try:
                        client_socket, _ = server_socket.accept()
                        client_sockets.append(client_socket)

                        # Read the response from this connection
                        response = client_socket.recv(1024)
                        response_str = response.decode().strip()
                        if response_str in expected_responses:
                            responses.add(response_str)
                    except socket.timeout:
                        break

                server_socket.close()
                stack_trace = get_stack_trace(p.pid)
            except PermissionError:
                self.skipTest(
                    "Insufficient permissions to read the stack trace"
                )
            finally:
                for client_socket in client_sockets:
                    if client_socket is not None:
                        client_socket.close()
                p.kill()
                p.terminate()
                p.wait(timeout=SHORT_TIMEOUT)

            # Verify we have multiple interpreters
            self.assertGreaterEqual(len(stack_trace), 2, "Should have at least two interpreters")

            # Count interpreters by ID
            interpreter_ids = {interp.interpreter_id for interp in stack_trace}
            self.assertIn(0, interpreter_ids, "Main interpreter should be present")
            self.assertGreaterEqual(len(interpreter_ids), 3, "Should have main + at least 2 subinterpreters")

            # Count total threads across all interpreters
            total_threads = sum(len(interp.threads) for interp in stack_trace)
            self.assertGreaterEqual(total_threads, 5, "Should have at least 5 threads total")

            # Look for expected function names in stack traces
            all_funcnames = set()
            for interpreter_info in stack_trace:
                for thread_info in interpreter_info.threads:
                    for frame in thread_info.frame_info:
                        all_funcnames.add(frame.funcname)

            # Should find functions from different interpreters and threads
            expected_funcs = {"main_worker", "worker1", "worker2", "nested_func"}
            found_funcs = expected_funcs.intersection(all_funcnames)
            self.assertGreater(len(found_funcs), 0, f"Should find some expected functions, got: {all_funcnames}")

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    @requires_gil_enabled("Free threaded builds don't have an 'active thread'")
    def test_only_active_thread(self):
        # Test that only_active_thread parameter works correctly
        port = find_unused_port()
        script = textwrap.dedent(
            f"""\
            import time, sys, socket, threading

            # Connect to the test process
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(('localhost', {port}))

            def worker_thread(name, barrier, ready_event):
                barrier.wait()  # Synchronize thread start
                ready_event.wait()  # Wait for main thread signal
                # Sleep to keep thread alive
                time.sleep(10_000)

            def main_work():
                # Do busy work to hold the GIL
                sock.sendall(b"working\\n")
                count = 0
                while count < 100000000:
                    count += 1
                    if count % 10000000 == 0:
                        pass  # Keep main thread busy
                sock.sendall(b"done\\n")

            # Create synchronization primitives
            num_threads = 3
            barrier = threading.Barrier(num_threads + 1)  # +1 for main thread
            ready_event = threading.Event()

            # Start worker threads
            threads = []
            for i in range(num_threads):
                t = threading.Thread(target=worker_thread, args=(f"Worker-{{i}}", barrier, ready_event))
                t.start()
                threads.append(t)

            # Wait for all threads to be ready
            barrier.wait()

            # Signal ready to parent process
            sock.sendall(b"ready\\n")

            # Signal threads to start waiting
            ready_event.set()

            # Now do busy work to hold the GIL
            main_work()
            """
        )

        with os_helper.temp_dir() as work_dir:
            script_dir = os.path.join(work_dir, "script_pkg")
            os.mkdir(script_dir)

            # Create a socket server to communicate with the target process
            server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server_socket.bind(("localhost", port))
            server_socket.settimeout(SHORT_TIMEOUT)
            server_socket.listen(1)

            script_name = _make_test_script(script_dir, "script", script)
            client_socket = None
            try:
                p = subprocess.Popen([sys.executable, script_name])
                client_socket, _ = server_socket.accept()
                server_socket.close()

                # Wait for ready signal
                response = b""
                while b"ready" not in response:
                    response += client_socket.recv(1024)

                # Wait for the main thread to start its busy work
                while b"working" not in response:
                    response += client_socket.recv(1024)

                # Get stack trace with all threads
                unwinder_all = RemoteUnwinder(p.pid, all_threads=True)
                for _ in range(10):
                    # Wait for the main thread to start its busy work
                    all_traces = unwinder_all.get_stack_trace()
                    found = False
                    # New format: [InterpreterInfo(interpreter_id, [ThreadInfo(...)])]
                    for interpreter_info in all_traces:
                        for thread_info in interpreter_info.threads:
                            if not thread_info.frame_info:
                                continue
                            current_frame = thread_info.frame_info[0]
                            if (
                                current_frame.funcname == "main_work"
                                and current_frame.lineno > 15
                            ):
                                found = True
                                break
                        if found:
                            break

                    if found:
                        break
                    # Give a bit of time to take the next sample
                    time.sleep(0.1)
                else:
                    self.fail(
                        "Main thread did not start its busy work on time"
                    )

                # Get stack trace with only GIL holder
                unwinder_gil = RemoteUnwinder(p.pid, only_active_thread=True)
                gil_traces = unwinder_gil.get_stack_trace()

            except PermissionError:
                self.skipTest(
                    "Insufficient permissions to read the stack trace"
                )
            finally:
                if client_socket is not None:
                    client_socket.close()
                p.kill()
                p.terminate()
                p.wait(timeout=SHORT_TIMEOUT)

            # Count total threads across all interpreters in all_traces
            total_threads = sum(len(interpreter_info.threads) for interpreter_info in all_traces)
            self.assertGreater(
                total_threads, 1, "Should have multiple threads"
            )

            # Count total threads across all interpreters in gil_traces
            total_gil_threads = sum(len(interpreter_info.threads) for interpreter_info in gil_traces)
            self.assertEqual(
                total_gil_threads, 1, "Should have exactly one GIL holder"
            )

            # Get the GIL holder thread ID
            gil_thread_id = None
            for interpreter_info in gil_traces:
                if interpreter_info.threads:
                    gil_thread_id = interpreter_info.threads[0].thread_id
                    break

            # Get all thread IDs from all_traces
            all_thread_ids = []
            for interpreter_info in all_traces:
                for thread_info in interpreter_info.threads:
                    all_thread_ids.append(thread_info.thread_id)

            self.assertIn(
                gil_thread_id,
                all_thread_ids,
                "GIL holder should be among all threads",
            )


class TestUnsupportedPlatformHandling(unittest.TestCase):
    @unittest.skipIf(
        sys.platform in ("linux", "darwin", "win32"),
        "Test only runs on unsupported platforms (not Linux, macOS, or Windows)",
    )
    @unittest.skipIf(sys.platform == "android", "Android raises Linux-specific exception")
    def test_unsupported_platform_error(self):
        with self.assertRaises(RuntimeError) as cm:
            RemoteUnwinder(os.getpid())

        self.assertIn(
            "Reading the PyRuntime section is not supported on this platform",
            str(cm.exception)
        )

class TestDetectionOfThreadStatus(unittest.TestCase):
    @unittest.skipIf(
        sys.platform not in ("linux", "darwin", "win32"),
        "Test only runs on unsupported platforms (not Linux, macOS, or Windows)",
    )
    @unittest.skipIf(sys.platform == "android", "Android raises Linux-specific exception")
    def test_thread_status_detection(self):
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
            server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server_socket.bind(("localhost", port))
            server_socket.settimeout(SHORT_TIMEOUT)
            server_socket.listen(1)

            script_name = _make_test_script(script_dir, "thread_status_script", script)
            client_socket = None
            try:
                p = subprocess.Popen([sys.executable, script_name])
                client_socket, _ = server_socket.accept()
                server_socket.close()
                response = b""
                sleeper_tid = None
                busy_tid = None
                while True:
                    chunk = client_socket.recv(1024)
                    response += chunk
                    if b"ready:main" in response and b"ready:sleeper" in response and b"ready:busy" in response:
                        # Parse TIDs from the response
                        for line in response.split(b"\n"):
                            if line.startswith(b"ready:sleeper:"):
                                try:
                                    sleeper_tid = int(line.split(b":")[-1])
                                except Exception:
                                    pass
                            elif line.startswith(b"ready:busy:"):
                                try:
                                    busy_tid = int(line.split(b":")[-1])
                                except Exception:
                                    pass
                        break

                attempts = 10
                statuses = {}
                try:
                    unwinder = RemoteUnwinder(p.pid, all_threads=True, mode=PROFILING_MODE_CPU,
                                                skip_non_matching_threads=False)
                    for _ in range(attempts):
                        traces = unwinder.get_stack_trace()
                        # Find threads and their statuses
                        statuses = {}
                        for interpreter_info in traces:
                            for thread_info in interpreter_info.threads:
                                statuses[thread_info.thread_id] = thread_info.status

                        # Check if sleeper thread is off CPU and busy thread is on CPU
                        # In the new flags system:
                        # - sleeper should NOT have ON_CPU flag (off CPU)
                        # - busy should have ON_CPU flag
                        if (sleeper_tid in statuses and
                            busy_tid in statuses and
                            not (statuses[sleeper_tid] & THREAD_STATUS_ON_CPU) and
                            (statuses[busy_tid] & THREAD_STATUS_ON_CPU)):
                            break
                        time.sleep(0.5)  # Give a bit of time to let threads settle
                except PermissionError:
                    self.skipTest(
                        "Insufficient permissions to read the stack trace"
                    )

                self.assertIsNotNone(sleeper_tid, "Sleeper thread id not received")
                self.assertIsNotNone(busy_tid, "Busy thread id not received")
                self.assertIn(sleeper_tid, statuses, "Sleeper tid not found in sampled threads")
                self.assertIn(busy_tid, statuses, "Busy tid not found in sampled threads")
                self.assertFalse(statuses[sleeper_tid] & THREAD_STATUS_ON_CPU, "Sleeper thread should be off CPU")
                self.assertTrue(statuses[busy_tid] & THREAD_STATUS_ON_CPU, "Busy thread should be on CPU")

            finally:
                if client_socket is not None:
                    client_socket.close()
                p.terminate()
                p.wait(timeout=SHORT_TIMEOUT)

    @unittest.skipIf(
        sys.platform not in ("linux", "darwin", "win32"),
        "Test only runs on unsupported platforms (not Linux, macOS, or Windows)",
    )
    @unittest.skipIf(sys.platform == "android", "Android raises Linux-specific exception")
    def test_thread_status_gil_detection(self):
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
            server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server_socket.bind(("localhost", port))
            server_socket.settimeout(SHORT_TIMEOUT)
            server_socket.listen(1)

            script_name = _make_test_script(script_dir, "thread_status_script", script)
            client_socket = None
            try:
                p = subprocess.Popen([sys.executable, script_name])
                client_socket, _ = server_socket.accept()
                server_socket.close()
                response = b""
                sleeper_tid = None
                busy_tid = None
                while True:
                    chunk = client_socket.recv(1024)
                    response += chunk
                    if b"ready:main" in response and b"ready:sleeper" in response and b"ready:busy" in response:
                        # Parse TIDs from the response
                        for line in response.split(b"\n"):
                            if line.startswith(b"ready:sleeper:"):
                                try:
                                    sleeper_tid = int(line.split(b":")[-1])
                                except Exception:
                                    pass
                            elif line.startswith(b"ready:busy:"):
                                try:
                                    busy_tid = int(line.split(b":")[-1])
                                except Exception:
                                    pass
                        break

                attempts = 10
                statuses = {}
                try:
                    unwinder = RemoteUnwinder(p.pid, all_threads=True, mode=PROFILING_MODE_GIL,
                                                skip_non_matching_threads=False)
                    for _ in range(attempts):
                        traces = unwinder.get_stack_trace()
                        # Find threads and their statuses
                        statuses = {}
                        for interpreter_info in traces:
                            for thread_info in interpreter_info.threads:
                                statuses[thread_info.thread_id] = thread_info.status

                        # Check if sleeper thread doesn't have GIL and busy thread has GIL
                        # In the new flags system:
                        # - sleeper should NOT have HAS_GIL flag (waiting for GIL)
                        # - busy should have HAS_GIL flag
                        if (sleeper_tid in statuses and
                            busy_tid in statuses and
                            not (statuses[sleeper_tid] & THREAD_STATUS_HAS_GIL) and
                            (statuses[busy_tid] & THREAD_STATUS_HAS_GIL)):
                            break
                        time.sleep(0.5)  # Give a bit of time to let threads settle
                except PermissionError:
                    self.skipTest(
                        "Insufficient permissions to read the stack trace"
                    )

                self.assertIsNotNone(sleeper_tid, "Sleeper thread id not received")
                self.assertIsNotNone(busy_tid, "Busy thread id not received")
                self.assertIn(sleeper_tid, statuses, "Sleeper tid not found in sampled threads")
                self.assertIn(busy_tid, statuses, "Busy tid not found in sampled threads")
                self.assertFalse(statuses[sleeper_tid] & THREAD_STATUS_HAS_GIL, "Sleeper thread should not have GIL")
                self.assertTrue(statuses[busy_tid] & THREAD_STATUS_HAS_GIL, "Busy thread should have GIL")

            finally:
                if client_socket is not None:
                    client_socket.close()
                p.terminate()
                p.wait(timeout=SHORT_TIMEOUT)

    @unittest.skipIf(
        sys.platform not in ("linux", "darwin", "win32"),
        "Test only runs on supported platforms (Linux, macOS, or Windows)",
    )
    @unittest.skipIf(sys.platform == "android", "Android raises Linux-specific exception")
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
            server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server_socket.bind(("localhost", port))
            server_socket.listen(2)
            server_socket.settimeout(SHORT_TIMEOUT)

            p = subprocess.Popen(
                [sys.executable, script_file],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )

            client_sockets = []
            try:
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
                            except Exception:
                                pass
                        elif line.startswith(b"busy:"):
                            try:
                                busy_tid = int(line.split(b":")[-1])
                            except Exception:
                                pass

                server_socket.close()

                attempts = 10
                statuses = {}
                try:
                    unwinder = RemoteUnwinder(p.pid, all_threads=True, mode=PROFILING_MODE_ALL,
                                                skip_non_matching_threads=False)
                    for _ in range(attempts):
                        traces = unwinder.get_stack_trace()
                        # Find threads and their statuses
                        statuses = {}
                        for interpreter_info in traces:
                            for thread_info in interpreter_info.threads:
                                statuses[thread_info.thread_id] = thread_info.status

                        # Check ALL mode provides both GIL and CPU info
                        # - sleeper should NOT have ON_CPU and NOT have HAS_GIL
                        # - busy should have ON_CPU and have HAS_GIL
                        if (sleeper_tid in statuses and
                            busy_tid in statuses and
                            not (statuses[sleeper_tid] & THREAD_STATUS_ON_CPU) and
                            not (statuses[sleeper_tid] & THREAD_STATUS_HAS_GIL) and
                            (statuses[busy_tid] & THREAD_STATUS_ON_CPU) and
                            (statuses[busy_tid] & THREAD_STATUS_HAS_GIL)):
                            break
                        time.sleep(0.5)
                except PermissionError:
                    self.skipTest(
                        "Insufficient permissions to read the stack trace"
                    )

                self.assertIsNotNone(sleeper_tid, "Sleeper thread id not received")
                self.assertIsNotNone(busy_tid, "Busy thread id not received")
                self.assertIn(sleeper_tid, statuses, "Sleeper tid not found in sampled threads")
                self.assertIn(busy_tid, statuses, "Busy tid not found in sampled threads")

                # Sleeper thread: off CPU, no GIL
                self.assertFalse(statuses[sleeper_tid] & THREAD_STATUS_ON_CPU, "Sleeper should be off CPU")
                self.assertFalse(statuses[sleeper_tid] & THREAD_STATUS_HAS_GIL, "Sleeper should not have GIL")

                # Busy thread: on CPU, has GIL
                self.assertTrue(statuses[busy_tid] & THREAD_STATUS_ON_CPU, "Busy should be on CPU")
                self.assertTrue(statuses[busy_tid] & THREAD_STATUS_HAS_GIL, "Busy should have GIL")

            finally:
                for client_socket in client_sockets:
                    client_socket.close()
                p.terminate()
                p.wait(timeout=SHORT_TIMEOUT)
                p.stdout.close()
                p.stderr.close()


class TestFrameCaching(unittest.TestCase):
    """Test that frame caching produces correct results.

    Uses socket-based synchronization for deterministic testing.
    All tests verify cache reuse via object identity checks (assertIs).
    """

    maxDiff = None
    MAX_TRIES = 10

    @contextlib.contextmanager
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

            server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server_socket.bind(("localhost", port))
            server_socket.settimeout(SHORT_TIMEOUT)
            server_socket.listen(1)

            script_name = _make_test_script(script_dir, "script", script)
            client_socket = None
            p = None
            try:
                p = subprocess.Popen([sys.executable, script_name])
                client_socket, _ = server_socket.accept()
                server_socket.close()

                def make_unwinder(cache_frames=True):
                    return RemoteUnwinder(p.pid, all_threads=True, cache_frames=cache_frames)

                yield p, client_socket, make_unwinder

            except PermissionError:
                self.skipTest("Insufficient permissions to read the stack trace")
            finally:
                if client_socket:
                    client_socket.close()
                if p:
                    p.kill()
                    p.terminate()
                    p.wait(timeout=SHORT_TIMEOUT)

    def _wait_for_signal(self, client_socket, signal):
        """Block until signal received from target."""
        response = b""
        while signal not in response:
            chunk = client_socket.recv(64)
            if not chunk:
                break
            response += chunk
        return response

    def _get_frames(self, unwinder, required_funcs):
        """Sample and return frame_info list for thread containing required_funcs."""
        traces = unwinder.get_stack_trace()
        for interp in traces:
            for thread in interp.threads:
                funcs = [f.funcname for f in thread.frame_info]
                if required_funcs.issubset(set(funcs)):
                    return thread.frame_info
        return None

    def _sample_frames(self, client_socket, unwinder, wait_signal, send_ack, required_funcs, expected_frames=1):
        """Wait for signal, sample frames, send ack. Returns frame_info list."""
        self._wait_for_signal(client_socket, wait_signal)
        # Give at least MAX_TRIES tries for the process to arrive to a steady state
        for _ in range(self.MAX_TRIES):
            frames = self._get_frames(unwinder, required_funcs)
            if frames and len(frames) >= expected_frames:
                break
            time.sleep(0.1)
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

        with self._target_process(script_body) as (p, client_socket, make_unwinder):
            unwinder = make_unwinder(cache_frames=True)
            expected = {"level1", "level2", "level3"}

            frames1 = self._sample_frames(client_socket, unwinder, b"sync1", b"ack", expected)
            frames2 = self._sample_frames(client_socket, unwinder, b"sync2", b"ack", expected)
            frames3 = self._sample_frames(client_socket, unwinder, b"sync3", b"done", expected)

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
            self.assertIs(f1, f2, f"Frame {i}: samples 1-2 must be same object")
            self.assertIs(f2, f3, f"Frame {i}: samples 2-3 must be same object")

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

        with self._target_process(script_body) as (p, client_socket, make_unwinder):
            unwinder = make_unwinder(cache_frames=True)

            frames_a = self._sample_frames(client_socket, unwinder, b"line_a", b"ack", {"inner"})
            frames_b = self._sample_frames(client_socket, unwinder, b"line_b", b"ack", {"inner"})
            frames_c = self._sample_frames(client_socket, unwinder, b"line_c", b"ack", {"inner"})
            frames_d = self._sample_frames(client_socket, unwinder, b"line_d", b"done", {"inner"})

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
        self.assertLess(inner_a.lineno, inner_b.lineno,
                        "Line B should be after line A")
        self.assertLess(inner_b.lineno, inner_c.lineno,
                        "Line C should be after line B")
        self.assertLess(inner_c.lineno, inner_d.lineno,
                        "Line D should be after line C")

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

        with self._target_process(script_body) as (p, client_socket, make_unwinder):
            unwinder = make_unwinder(cache_frames=True)

            frames_deep = self._sample_frames(
                client_socket, unwinder, b"at_inner", b"ack", {"inner", "outer"})
            frames_shallow = self._sample_frames(
                client_socket, unwinder, b"at_outer", b"done", {"outer"})

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

        with self._target_process(script_body) as (p, client_socket, make_unwinder):
            unwinder = make_unwinder(cache_frames=True)

            frames_before = self._sample_frames(
                client_socket, unwinder, b"at_middle", b"ack", {"middle", "top"})
            frames_after = self._sample_frames(
                client_socket, unwinder, b"at_deeper", b"done", {"deeper", "middle", "top"})

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
        """Test that unchanged bottom frames are reused when top changes (A→B→C to A→B→D)."""
        script_body = """\
            def func_c():
                sock.sendall(b"at_c")
                sock.recv(16)

            def func_d():
                sock.sendall(b"at_d")
                sock.recv(16)

            def func_b():
                func_c()
                func_d()

            def func_a():
                func_b()

            func_a()
            """

        with self._target_process(script_body) as (p, client_socket, make_unwinder):
            unwinder = make_unwinder(cache_frames=True)

            # Sample at C: stack is A→B→C
            frames_c = self._sample_frames(
                client_socket, unwinder, b"at_c", b"ack", {"func_a", "func_b", "func_c"})
            # Sample at D: stack is A→B→D (C returned, D called)
            frames_d = self._sample_frames(
                client_socket, unwinder, b"at_d", b"done", {"func_a", "func_b", "func_d"})

        self.assertIsNotNone(frames_c)
        self.assertIsNotNone(frames_d)

        # Find func_a and func_b frames in both samples
        def find_frame(frames, funcname):
            for f in frames:
                if f.funcname == funcname:
                    return f
            return None

        frame_a_in_c = find_frame(frames_c, "func_a")
        frame_b_in_c = find_frame(frames_c, "func_b")
        frame_a_in_d = find_frame(frames_d, "func_a")
        frame_b_in_d = find_frame(frames_d, "func_b")

        self.assertIsNotNone(frame_a_in_c)
        self.assertIsNotNone(frame_b_in_c)
        self.assertIsNotNone(frame_a_in_d)
        self.assertIsNotNone(frame_b_in_d)

        # The bottom frames (A, B) should be the SAME objects (cache reuse)
        self.assertIs(frame_a_in_c, frame_a_in_d, "func_a frame should be reused from cache")
        self.assertIs(frame_b_in_c, frame_b_in_d, "func_b frame should be reused from cache")

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

        with self._target_process(script_body) as (p, client_socket, make_unwinder):
            unwinder = make_unwinder(cache_frames=True)

            frames1 = self._sample_frames(
                client_socket, unwinder, b"sync1", b"ack", {"recurse"})
            frames2 = self._sample_frames(
                client_socket, unwinder, b"sync2", b"done", {"recurse"})

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
            self.assertIs(frames1[i], frames2[i],
                          f"Frame {i}: recursive frames must be same object")

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

        with self._target_process(script_body) as (p, client_socket, make_unwinder):
            self._wait_for_signal(client_socket, b"ready")

            # Sample with cache
            unwinder_cache = make_unwinder(cache_frames=True)
            frames_cached = self._get_frames(unwinder_cache, {"level1", "level2", "level3"})

            # Sample without cache
            unwinder_no_cache = make_unwinder(cache_frames=False)
            frames_no_cache = self._get_frames(unwinder_no_cache, {"level1", "level2", "level3"})

            client_socket.sendall(b"done")

        self.assertIsNotNone(frames_cached)
        self.assertIsNotNone(frames_no_cache)

        # Same number of frames
        self.assertEqual(len(frames_cached), len(frames_no_cache))

        # Same function names in same order
        funcs_cached = [f.funcname for f in frames_cached]
        funcs_no_cache = [f.funcname for f in frames_no_cache]
        self.assertEqual(funcs_cached, funcs_no_cache)

        # Same line numbers
        lines_cached = [f.lineno for f in frames_cached]
        lines_no_cache = [f.lineno for f in frames_no_cache]
        self.assertEqual(lines_cached, lines_no_cache)

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

        with self._target_process(script_body) as (p, client_socket, make_unwinder):
            unwinder = make_unwinder(cache_frames=True)
            buffer = b""

            def recv_msg():
                """Receive a single message from socket."""
                nonlocal buffer
                while b"\n" not in buffer:
                    chunk = client_socket.recv(256)
                    if not chunk:
                        return None
                    buffer += chunk
                msg, buffer = buffer.split(b"\n", 1)
                return msg

            def get_thread_frames(target_funcs):
                """Get frames for thread matching target functions."""
                retries = 0
                for _ in busy_retry(SHORT_TIMEOUT):
                    if retries >= 5:
                        break
                    retries += 1
                    # On Windows, ReadProcessMemory can fail with OSError
                    # (WinError 299) when frame pointers are in flux
                    with contextlib.suppress(RuntimeError, OSError):
                        traces = unwinder.get_stack_trace()
                        for interp in traces:
                            for thread in interp.threads:
                                funcs = [f.funcname for f in thread.frame_info]
                                if any(f in funcs for f in target_funcs):
                                    return funcs
                return None

            # Track results for each sync point
            results = {}

            # Process 4 sync points: baz1, baz2, blech1, blech2
            # With the lock, threads are serialized - handle one at a time
            for _ in range(4):
                msg = recv_msg()
                self.assertIsNotNone(msg, "Expected message from subprocess")

                # Determine which thread/function and take snapshot
                if msg == b"t1:baz1":
                    funcs = get_thread_frames(["baz1", "bar1", "foo1"])
                    self.assertIsNotNone(funcs, "Thread 1 not found at baz1")
                    results["t1:baz1"] = funcs
                elif msg == b"t2:baz2":
                    funcs = get_thread_frames(["baz2", "bar2", "foo2"])
                    self.assertIsNotNone(funcs, "Thread 2 not found at baz2")
                    results["t2:baz2"] = funcs
                elif msg == b"t1:blech1":
                    funcs = get_thread_frames(["blech1", "foo1"])
                    self.assertIsNotNone(funcs, "Thread 1 not found at blech1")
                    results["t1:blech1"] = funcs
                elif msg == b"t2:blech2":
                    funcs = get_thread_frames(["blech2", "foo2"])
                    self.assertIsNotNone(funcs, "Thread 2 not found at blech2")
                    results["t2:blech2"] = funcs

                # Release thread to continue
                client_socket.sendall(b"k")

            # Validate Phase 1: baz snapshots
            t1_baz = results.get("t1:baz1")
            t2_baz = results.get("t2:baz2")
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
            t1_blech = results.get("t1:blech1")
            t2_blech = results.get("t2:blech2")
            self.assertIsNotNone(t1_blech, "Missing t1:blech1 snapshot")
            self.assertIsNotNone(t2_blech, "Missing t2:blech2 snapshot")

            # Thread 1 at blech1: bar1/baz1 should be GONE (cache invalidated)
            self.assertIn("blech1", t1_blech)
            self.assertIn("foo1", t1_blech)
            self.assertNotIn("bar1", t1_blech, "Cache not invalidated: bar1 still present")
            self.assertNotIn("baz1", t1_blech, "Cache not invalidated: baz1 still present")
            # No cross-contamination
            self.assertNotIn("blech2", t1_blech)

            # Thread 2 at blech2: bar2/baz2 should be GONE (cache invalidated)
            self.assertIn("blech2", t2_blech)
            self.assertIn("foo2", t2_blech)
            self.assertNotIn("bar2", t2_blech, "Cache not invalidated: bar2 still present")
            self.assertNotIn("baz2", t2_blech, "Cache not invalidated: baz2 still present")
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

        with self._target_process(script_body) as (p, client_socket, make_unwinder):
            expected = {"level1", "level2", "level3", "level4"}

            # First unwinder samples - this sets last_profiled_frame in target
            unwinder1 = make_unwinder(cache_frames=True)
            frames1 = self._sample_frames(client_socket, unwinder1, b"sync1", b"ack", expected)

            # Create NEW unwinder (empty cache) and sample
            # The target still has last_profiled_frame set from unwinder1
            unwinder2 = make_unwinder(cache_frames=True)
            frames2 = self._sample_frames(client_socket, unwinder2, b"sync2", b"done", expected)

        self.assertIsNotNone(frames1)
        self.assertIsNotNone(frames2)

        funcs1 = [f.funcname for f in frames1]
        funcs2 = [f.funcname for f in frames2]

        # Both should have all levels
        for level in ["level1", "level2", "level3", "level4"]:
            self.assertIn(level, funcs1, f"{level} missing from first sample")
            self.assertIn(level, funcs2, f"{level} missing from second sample")

        # Should have same stack depth
        self.assertEqual(len(frames1), len(frames2),
                         "New unwinder should return complete stack despite stale last_profiled_frame")

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

        with self._target_process(script_body) as (p, client_socket, make_unwinder):
            unwinder_cache = make_unwinder(cache_frames=True)
            unwinder_no_cache = make_unwinder(cache_frames=False)

            frames_cached = self._sample_frames(
                client_socket, unwinder_cache, b"ready", b"ack", {"recurse"}, 1100
            )
            # Sample again with no cache for comparison
            frames_no_cache = self._sample_frames(
                client_socket, unwinder_no_cache, b"ready2", b"done", {"recurse"}, 1100
            )

        self.assertIsNotNone(frames_cached)
        self.assertIsNotNone(frames_no_cache)

        # Both should have many recurse frames (> 1024 limit)
        cached_count = [f.funcname for f in frames_cached].count("recurse")
        no_cache_count = [f.funcname for f in frames_no_cache].count("recurse")

        self.assertGreater(cached_count, 1000, "Should have >1000 recurse frames")
        self.assertGreater(no_cache_count, 1000, "Should have >1000 recurse frames")

        # Both modes should produce same frame count
        self.assertEqual(len(frames_cached), len(frames_no_cache),
                        "Cache exhaustion should not affect stack completeness")

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
            self._wait_for_signal(client_socket, b"ready")

            # Take a sample
            unwinder.get_stack_trace()

            stats = unwinder.get_stats()
            client_socket.sendall(b"done")

        # Verify expected keys exist
        expected_keys = [
            'total_samples', 'frame_cache_hits', 'frame_cache_misses',
            'frame_cache_partial_hits', 'frames_read_from_cache',
            'frames_read_from_memory', 'frame_cache_hit_rate'
        ]
        for key in expected_keys:
            self.assertIn(key, stats)

        self.assertEqual(stats['total_samples'], 1)

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
            unwinder = RemoteUnwinder(p.pid, all_threads=True)  # stats=False by default
            self._wait_for_signal(client_socket, b"ready")

            with self.assertRaises(RuntimeError):
                unwinder.get_stats()

            client_socket.sendall(b"done")


if __name__ == "__main__":
    unittest.main()
