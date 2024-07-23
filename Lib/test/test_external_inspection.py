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
    def test_self_trace(self):
        stack_trace = get_stack_trace(os.getpid())
        self.assertEqual(stack_trace[0], "test_self_trace")

if __name__ == "__main__":
    unittest.main()
