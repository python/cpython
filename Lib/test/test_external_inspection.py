import unittest
import os
import textwrap
import importlib
import sys
import socket
import threading
from asyncio import staggered, taskgroups, base_events, tasks
from unittest.mock import ANY
from test.support import os_helper, SHORT_TIMEOUT, busy_retry
from test.support.script_helper import make_script
from test.support.socket_helper import find_unused_port

import subprocess

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
            ]
            # Is possible that there are more threads, so we check that the
            # expected stack traces are in the result (looking at you Windows!)
            self.assertIn((ANY, thread_expected_stack_trace), stack_trace)

            # Check that the main thread stack trace is in the result
            frame = FrameInfo([script_name, 19, "<module>"])
            for _, stack in stack_trace:
                if frame in stack:
                    break
            else:
                self.fail("Main thread stack trace not found in result")

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

                # sets are unordered, so we want to sort "awaited_by"s
                stack_trace[2].sort(key=lambda x: x[1])

                expected_stack_trace = [
                    [
                        FrameInfo([script_name, 10, "c5"]),
                        FrameInfo([script_name, 14, "c4"]),
                        FrameInfo([script_name, 17, "c3"]),
                        FrameInfo([script_name, 20, "c2"]),
                    ],
                    "c2_root",
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
                                    FrameInfo([script_name, 26, "main"]),
                                ],
                                "Task-1",
                            ]
                        ),
                        CoroInfo(
                            [
                                [FrameInfo([script_name, 23, "c1"])],
                                "sub_main_1",
                            ]
                        ),
                        CoroInfo(
                            [
                                [FrameInfo([script_name, 23, "c1"])],
                                "sub_main_2",
                            ]
                        ),
                    ],
                ]
                self.assertEqual(stack_trace, expected_stack_trace)

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

            # sets are unordered, so we want to sort "awaited_by"s
            stack_trace[2].sort(key=lambda x: x[1])

            expected_stack_trace = [
                [
                    FrameInfo([script_name, 10, "gen_nested_call"]),
                    FrameInfo([script_name, 16, "gen"]),
                    FrameInfo([script_name, 19, "main"]),
                ],
                "Task-1",
                [],
            ]
            self.assertEqual(stack_trace, expected_stack_trace)

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

            # sets are unordered, so we want to sort "awaited_by"s
            stack_trace[2].sort(key=lambda x: x[1])

            expected_stack_trace = [
                [
                    FrameInfo([script_name, 11, "deep"]),
                    FrameInfo([script_name, 15, "c1"]),
                ],
                "Task-2",
                [CoroInfo([[FrameInfo([script_name, 21, "main"])], "Task-1"])],
            ]
            self.assertEqual(stack_trace, expected_stack_trace)

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

            # sets are unordered, so we want to sort "awaited_by"s
            stack_trace[2].sort(key=lambda x: x[1])
            expected_stack_trace = [
                [
                    FrameInfo([script_name, 11, "deep"]),
                    FrameInfo([script_name, 15, "c1"]),
                    FrameInfo(
                        [
                            staggered.__file__,
                            ANY,
                            "staggered_race.<locals>.run_one_coro",
                        ]
                    ),
                ],
                "Task-2",
                [
                    CoroInfo(
                        [
                            [
                                FrameInfo(
                                    [staggered.__file__, ANY, "staggered_race"]
                                ),
                                FrameInfo([script_name, 21, "main"]),
                            ],
                            "Task-1",
                        ]
                    )
                ],
            ]
            self.assertEqual(stack_trace, expected_stack_trace)

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
        for thread_id, stack in stack_trace:
            if thread_id == threading.get_native_id():
                this_tread_stack = stack
                break
        self.assertIsNotNone(this_tread_stack)
        self.assertEqual(
            stack[:2],
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


if __name__ == "__main__":
    unittest.main()
