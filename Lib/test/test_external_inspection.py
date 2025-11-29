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


if __name__ == "__main__":
    unittest.main()
