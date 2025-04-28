import unittest
import os
import textwrap
import importlib
import sys
import socket
from test.support import os_helper, SHORT_TIMEOUT, busy_retry
from test.support.script_helper import make_script
from test.support.socket_helper import find_unused_port

import subprocess

PROCESS_VM_READV_SUPPORTED = False

try:
    from _testexternalinspection import PROCESS_VM_READV_SUPPORTED
    from _testexternalinspection import get_stack_trace
    from _testexternalinspection import get_async_stack_trace
    from _testexternalinspection import get_all_awaited_by
except ImportError:
    raise unittest.SkipTest(
        "Test only runs when _testexternalinspection is available")

def _make_test_script(script_dir, script_basename, source):
    to_return = make_script(script_dir, script_basename, source)
    importlib.invalidate_caches()
    return to_return

skip_if_not_supported = unittest.skipIf((sys.platform != "darwin"
                                         and sys.platform != "linux"
                                         and sys.platform != "win32"),
                                        "Test only runs on Linux, Windows and MacOS")
class TestGetStackTrace(unittest.TestCase):

    @skip_if_not_supported
    @unittest.skipIf(sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
                     "Test only runs on Linux with process_vm_readv support")
    def test_remote_stack_trace(self):
        # Spawn a process with some realistic Python code
        port = find_unused_port()
        script = textwrap.dedent(f"""\
            import time, sys, socket
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
                sock.sendall(b"ready")
                time.sleep(1000)

            bar()
            """)
        stack_trace = None
        with os_helper.temp_dir() as work_dir:
            script_dir = os.path.join(work_dir, "script_pkg")
            os.mkdir(script_dir)

            # Create a socket server to communicate with the target process
            server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server_socket.bind(('localhost', port))
            server_socket.settimeout(SHORT_TIMEOUT)
            server_socket.listen(1)

            script_name = _make_test_script(script_dir, 'script', script)
            client_socket = None
            try:
                p = subprocess.Popen([sys.executable, script_name])
                client_socket, _ = server_socket.accept()
                server_socket.close()
                response = client_socket.recv(1024)
                self.assertEqual(response, b"ready")
                stack_trace = get_stack_trace(p.pid)
            except PermissionError:
                self.skipTest("Insufficient permissions to read the stack trace")
            finally:
                if client_socket is not None:
                    client_socket.close()
                p.kill()
                p.terminate()
                p.wait(timeout=SHORT_TIMEOUT)


            expected_stack_trace = [
                'foo',
                'baz',
                'bar',
                '<module>'
            ]
            self.assertEqual(stack_trace, expected_stack_trace)

    @skip_if_not_supported
    @unittest.skipIf(sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
                     "Test only runs on Linux with process_vm_readv support")
    def test_async_remote_stack_trace(self):
        # Spawn a process with some realistic Python code
        port = find_unused_port()
        script = textwrap.dedent(f"""\
            import asyncio
            import time
            import sys
            import socket
            # Connect to the test process
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(('localhost', {port}))

            def c5():
                sock.sendall(b"ready")
                time.sleep(10000)

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
            """)
        stack_trace = None
        for task_factory_variant in "asyncio.new_event_loop", "new_eager_loop":
            with (
                self.subTest(task_factory_variant=task_factory_variant),
                os_helper.temp_dir() as work_dir,
            ):
                script_dir = os.path.join(work_dir, "script_pkg")
                os.mkdir(script_dir)
                server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                server_socket.bind(('localhost', port))
                server_socket.settimeout(SHORT_TIMEOUT)
                server_socket.listen(1)
                script_name = _make_test_script(
                    script_dir, 'script',
                    script.format(TASK_FACTORY=task_factory_variant))
                client_socket = None
                try:
                    p = subprocess.Popen(
                        [sys.executable, script_name]
                    )
                    client_socket, _ = server_socket.accept()
                    server_socket.close()
                    response = client_socket.recv(1024)
                    self.assertEqual(response, b"ready")
                    stack_trace = get_async_stack_trace(p.pid)
                except PermissionError:
                    self.skipTest(
                        "Insufficient permissions to read the stack trace")
                finally:
                    if client_socket is not None:
                        client_socket.close()
                    p.kill()
                    p.terminate()
                    p.wait(timeout=SHORT_TIMEOUT)

                # sets are unordered, so we want to sort "awaited_by"s
                stack_trace[2].sort(key=lambda x: x[1])

                root_task = "Task-1"
                expected_stack_trace = [
                    ["c5", "c4", "c3", "c2"],
                    "c2_root",
                    [
                        [["main"], root_task, []],
                        [["c1"], "sub_main_1", [[["main"], root_task, []]]],
                        [["c1"], "sub_main_2", [[["main"], root_task, []]]],
                    ],
                ]
                self.assertEqual(stack_trace, expected_stack_trace)

    @skip_if_not_supported
    @unittest.skipIf(sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
                     "Test only runs on Linux with process_vm_readv support")
    def test_asyncgen_remote_stack_trace(self):
        # Spawn a process with some realistic Python code
        port = find_unused_port()
        script = textwrap.dedent(f"""\
            import asyncio
            import time
            import sys
            import socket
            # Connect to the test process
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(('localhost', {port}))

            async def gen_nested_call():
                sock.sendall(b"ready")
                time.sleep(10000)

            async def gen():
                for num in range(2):
                    yield num
                    if num == 1:
                        await gen_nested_call()

            async def main():
                async for el in gen():
                    pass

            asyncio.run(main())
            """)
        stack_trace = None
        with os_helper.temp_dir() as work_dir:
            script_dir = os.path.join(work_dir, "script_pkg")
            os.mkdir(script_dir)
            # Create a socket server to communicate with the target process
            server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server_socket.bind(('localhost', port))
            server_socket.settimeout(SHORT_TIMEOUT)
            server_socket.listen(1)
            script_name = _make_test_script(script_dir, 'script', script)
            client_socket = None
            try:
                p = subprocess.Popen([sys.executable, script_name])
                client_socket, _ = server_socket.accept()
                server_socket.close()
                response = client_socket.recv(1024)
                self.assertEqual(response, b"ready")
                stack_trace = get_async_stack_trace(p.pid)
            except PermissionError:
                self.skipTest("Insufficient permissions to read the stack trace")
            finally:
                if client_socket is not None:
                    client_socket.close()
                p.kill()
                p.terminate()
                p.wait(timeout=SHORT_TIMEOUT)

            # sets are unordered, so we want to sort "awaited_by"s
            stack_trace[2].sort(key=lambda x: x[1])

            expected_stack_trace = [
                ['gen_nested_call', 'gen', 'main'], 'Task-1', []
            ]
            self.assertEqual(stack_trace, expected_stack_trace)

    @skip_if_not_supported
    @unittest.skipIf(sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
                     "Test only runs on Linux with process_vm_readv support")
    def test_async_gather_remote_stack_trace(self):
        # Spawn a process with some realistic Python code
        port = find_unused_port()
        script = textwrap.dedent(f"""\
            import asyncio
            import time
            import sys
            import socket
            # Connect to the test process
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(('localhost', {port}))

            async def deep():
                await asyncio.sleep(0)
                sock.sendall(b"ready")
                time.sleep(10000)

            async def c1():
                await asyncio.sleep(0)
                await deep()

            async def c2():
                await asyncio.sleep(0)

            async def main():
                await asyncio.gather(c1(), c2())

            asyncio.run(main())
            """)
        stack_trace = None
        with os_helper.temp_dir() as work_dir:
            script_dir = os.path.join(work_dir, "script_pkg")
            os.mkdir(script_dir)
            # Create a socket server to communicate with the target process
            server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server_socket.bind(('localhost', port))
            server_socket.settimeout(SHORT_TIMEOUT)
            server_socket.listen(1)
            script_name = _make_test_script(script_dir, 'script', script)
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
                    "Insufficient permissions to read the stack trace")
            finally:
                if client_socket is not None:
                    client_socket.close()
                p.kill()
                p.terminate()
                p.wait(timeout=SHORT_TIMEOUT)

            # sets are unordered, so we want to sort "awaited_by"s
            stack_trace[2].sort(key=lambda x: x[1])

            expected_stack_trace =  [
                ['deep', 'c1'], 'Task-2', [[['main'], 'Task-1', []]]
            ]
            self.assertEqual(stack_trace, expected_stack_trace)

    @skip_if_not_supported
    @unittest.skipIf(sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
                     "Test only runs on Linux with process_vm_readv support")
    def test_async_staggered_race_remote_stack_trace(self):
        # Spawn a process with some realistic Python code
        port = find_unused_port()
        script = textwrap.dedent(f"""\
            import asyncio.staggered
            import time
            import sys
            import socket
            # Connect to the test process
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(('localhost', {port}))

            async def deep():
                await asyncio.sleep(0)
                sock.sendall(b"ready")
                time.sleep(10000)

            async def c1():
                await asyncio.sleep(0)
                await deep()

            async def c2():
                await asyncio.sleep(10000)

            async def main():
                await asyncio.staggered.staggered_race(
                    [c1, c2],
                    delay=None,
                )

            asyncio.run(main())
            """)
        stack_trace = None
        with os_helper.temp_dir() as work_dir:
            script_dir = os.path.join(work_dir, "script_pkg")
            os.mkdir(script_dir)
            # Create a socket server to communicate with the target process
            server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server_socket.bind(('localhost', port))
            server_socket.settimeout(SHORT_TIMEOUT)
            server_socket.listen(1)
            script_name = _make_test_script(script_dir, 'script', script)
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
                    "Insufficient permissions to read the stack trace")
            finally:
                if client_socket is not None:
                    client_socket.close()
                p.kill()
                p.terminate()
                p.wait(timeout=SHORT_TIMEOUT)

            # sets are unordered, so we want to sort "awaited_by"s
            stack_trace[2].sort(key=lambda x: x[1])

            expected_stack_trace =  [
                ['deep', 'c1', 'run_one_coro'], 'Task-2', [[['main'], 'Task-1', []]]
            ]
            self.assertEqual(stack_trace, expected_stack_trace)

    @skip_if_not_supported
    @unittest.skipIf(sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
                     "Test only runs on Linux with process_vm_readv support")
    def test_async_global_awaited_by(self):
        port = find_unused_port()
        script = textwrap.dedent(f"""\
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
                await asyncio.sleep(SHORT_TIMEOUT)

            async def echo_client_spam(server):
                async with asyncio.TaskGroup() as tg:
                    while connections < 1000:
                        msg = list(ascii_lowercase + digits)
                        random.shuffle(msg)
                        tg.create_task(echo_client("".join(msg)))
                        await asyncio.sleep(0)
                    # at least a 1000 tasks created
                    sock.sendall(b"ready")
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
            """)
        stack_trace = None
        with os_helper.temp_dir() as work_dir:
            script_dir = os.path.join(work_dir, "script_pkg")
            os.mkdir(script_dir)
            # Create a socket server to communicate with the target process
            server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server_socket.bind(('localhost', port))
            server_socket.settimeout(SHORT_TIMEOUT)
            server_socket.listen(1)
            script_name = _make_test_script(script_dir, 'script', script)
            client_socket = None
            try:
                p = subprocess.Popen([sys.executable, script_name])
                client_socket, _ = server_socket.accept()
                server_socket.close()
                response = client_socket.recv(1024)
                self.assertEqual(response, b"ready")
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
                        elif msg.startswith("Invalid linked list structure reading remote memory"):
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
                self.assertIn(('Task-1', []), entries)
                self.assertIn(('server task', [[['main'], 'Task-1', []]]), entries)
                self.assertIn(('echo client spam', [[['main'], 'Task-1', []]]), entries)

                expected_stack = [[['echo_client_spam'], 'echo client spam', [[['main'], 'Task-1', []]]]]
                tasks_with_stack = [task for task in entries if task[1] == expected_stack]
                self.assertGreaterEqual(len(tasks_with_stack), 1000)

                # the final task will have some random number, but it should for
                # sure be one of the echo client spam horde (In windows this is not true
                # for some reason)
                if sys.platform != "win32":
                    self.assertEqual([[['echo_client_spam'], 'echo client spam', [[['main'], 'Task-1', []]]]], entries[-1][1])
            except PermissionError:
                self.skipTest(
                    "Insufficient permissions to read the stack trace")
            finally:
                if client_socket is not None:
                    client_socket.close()
                p.kill()
                p.terminate()
                p.wait(timeout=SHORT_TIMEOUT)

    @skip_if_not_supported
    @unittest.skipIf(sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
                     "Test only runs on Linux with process_vm_readv support")
    def test_self_trace(self):
        stack_trace = get_stack_trace(os.getpid())
        print(stack_trace)
        self.assertEqual(stack_trace[0], "test_self_trace")

if __name__ == "__main__":
    unittest.main()
