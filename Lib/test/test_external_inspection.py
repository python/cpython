import unittest
import os
import textwrap
import importlib
import sys
from test.support import os_helper, SHORT_TIMEOUT
from test.support.script_helper import make_script

import subprocess

PROCESS_VM_READV_SUPPORTED = False

try:
    from _testexternalinspection import PROCESS_VM_READV_SUPPORTED
    from _testexternalinspection import get_stack_trace
    from _testexternalinspection import get_async_stack_trace
except ImportError:
    raise unittest.SkipTest("Test only runs when _testexternalinspection is available")

def _make_test_script(script_dir, script_basename, source):
    to_return = make_script(script_dir, script_basename, source)
    importlib.invalidate_caches()
    return to_return

class TestGetStackTrace(unittest.TestCase):

    @unittest.skipIf(sys.platform != "darwin" and sys.platform != "linux", "Test only runs on Linux and MacOS")
    @unittest.skipIf(sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED, "Test only runs on Linux with process_vm_readv support")
    def test_remote_stack_trace(self):
        # Spawn a process with some realistic Python code
        script = textwrap.dedent("""\
            import time, sys, os
            def bar():
                for x in range(100):
                    if x == 50:
                        baz()
            def baz():
                foo()

            def foo():
                fifo = sys.argv[1]
                with open(sys.argv[1], "w") as fifo:
                    fifo.write("ready")
                time.sleep(1000)

            bar()
            """)
        stack_trace = None
        with os_helper.temp_dir() as work_dir:
            script_dir = os.path.join(work_dir, "script_pkg")
            os.mkdir(script_dir)
            fifo = f"{work_dir}/the_fifo"
            os.mkfifo(fifo)
            script_name = _make_test_script(script_dir, 'script', script)
            try:
                p = subprocess.Popen([sys.executable, script_name,  str(fifo)])
                with open(fifo, "r") as fifo_file:
                    response = fifo_file.read()
                self.assertEqual(response, "ready")
                stack_trace = get_stack_trace(p.pid)
            except PermissionError:
                self.skipTest("Insufficient permissions to read the stack trace")
            finally:
                os.remove(fifo)
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

    @unittest.skipIf(sys.platform != "darwin" and sys.platform != "linux", "Test only runs on Linux and MacOS")
    @unittest.skipIf(sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED, "Test only runs on Linux with process_vm_readv support")
    def test_async_remote_stack_trace(self):
        # Spawn a process with some realistic Python code
        script = textwrap.dedent("""\
            import asyncio
            import time
            import os
            import sys
            import test.test_asyncio.test_stack as ts

            def c5():
                fifo = sys.argv[1]
                with open(sys.argv[1], "w") as fifo:
                    fifo.write("ready")
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

            asyncio.run(main())
            """)
        stack_trace = None
        with os_helper.temp_dir() as work_dir:
            script_dir = os.path.join(work_dir, "script_pkg")
            os.mkdir(script_dir)
            fifo = f"{work_dir}/the_fifo"
            os.mkfifo(fifo)
            script_name = _make_test_script(script_dir, 'script', script)
            try:
                p = subprocess.Popen([sys.executable, script_name,  str(fifo)])
                with open(fifo, "r") as fifo_file:
                    response = fifo_file.read()
                self.assertEqual(response, "ready")
                stack_trace = get_async_stack_trace(p.pid)
            except PermissionError:
                self.skipTest("Insufficient permissions to read the stack trace")
            finally:
                os.remove(fifo)
                p.kill()
                p.terminate()
                p.wait(timeout=SHORT_TIMEOUT)

            # sets are unordered, so we want to sort "awaited_by"s
            stack_trace[2].sort(key=lambda x: x[1])

            expected_stack_trace = [
                ["c5", "c4", "c3", "c2"],
                "c2_root",
                [
                    [["main"], "Task-1", []],
                    [["c1"], "sub_main_1", [[["main"], "Task-1", []]]],
                    [["c1"], "sub_main_2", [[["main"], "Task-1", []]]],
                ],
            ]
            self.assertEqual(stack_trace, expected_stack_trace)

    @unittest.skipIf(sys.platform != "darwin" and sys.platform != "linux", "Test only runs on Linux and MacOS")
    @unittest.skipIf(sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED, "Test only runs on Linux with process_vm_readv support")
    def test_asyncgen_remote_stack_trace(self):
        # Spawn a process with some realistic Python code
        script = textwrap.dedent("""\
            import asyncio
            import time
            import os
            import sys
            import test.test_asyncio.test_stack as ts

            async def gen_nested_call():
                fifo = sys.argv[1]
                with open(sys.argv[1], "w") as fifo:
                    fifo.write("ready")
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
            fifo = f"{work_dir}/the_fifo"
            os.mkfifo(fifo)
            script_name = _make_test_script(script_dir, 'script', script)
            try:
                p = subprocess.Popen([sys.executable, script_name,  str(fifo)])
                with open(fifo, "r") as fifo_file:
                    response = fifo_file.read()
                self.assertEqual(response, "ready")
                stack_trace = get_async_stack_trace(p.pid)
            except PermissionError:
                self.skipTest("Insufficient permissions to read the stack trace")
            finally:
                os.remove(fifo)
                p.kill()
                p.terminate()
                p.wait(timeout=SHORT_TIMEOUT)

            # sets are unordered, so we want to sort "awaited_by"s
            stack_trace[2].sort(key=lambda x: x[1])

            expected_stack_trace = [['gen_nested_call', 'gen', 'main'], 'Task-1', []]
            self.assertEqual(stack_trace, expected_stack_trace)

    @unittest.skipIf(sys.platform != "darwin" and sys.platform != "linux", "Test only runs on Linux and MacOS")
    @unittest.skipIf(sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED, "Test only runs on Linux with process_vm_readv support")
    def test_async_gather_remote_stack_trace(self):
        # Spawn a process with some realistic Python code
        script = textwrap.dedent("""\
            import asyncio
            import time
            import os
            import sys
            import test.test_asyncio.test_stack as ts

            async def deep():
                await asyncio.sleep(0)
                fifo = sys.argv[1]
                with open(sys.argv[1], "w") as fifo:
                    fifo.write("ready")
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
            fifo = f"{work_dir}/the_fifo"
            os.mkfifo(fifo)
            script_name = _make_test_script(script_dir, 'script', script)
            try:
                p = subprocess.Popen([sys.executable, script_name,  str(fifo)])
                with open(fifo, "r") as fifo_file:
                    response = fifo_file.read()
                self.assertEqual(response, "ready")
                stack_trace = get_async_stack_trace(p.pid)
            except PermissionError:
                self.skipTest("Insufficient permissions to read the stack trace")
            finally:
                os.remove(fifo)
                p.kill()
                p.terminate()
                p.wait(timeout=SHORT_TIMEOUT)

            # sets are unordered, so we want to sort "awaited_by"s
            stack_trace[2].sort(key=lambda x: x[1])

            expected_stack_trace =  [['deep', 'c1'], 'Task-2', [[['main'], 'Task-1', []]]]
            self.assertEqual(stack_trace, expected_stack_trace)

    @unittest.skipIf(sys.platform != "darwin" and sys.platform != "linux", "Test only runs on Linux and MacOS")
    @unittest.skipIf(sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED, "Test only runs on Linux with process_vm_readv support")
    def test_self_trace(self):
        stack_trace = get_stack_trace(os.getpid())
        self.assertEqual(stack_trace[0], "test_self_trace")

if __name__ == "__main__":
    unittest.main()
