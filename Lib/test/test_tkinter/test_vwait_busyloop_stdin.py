# test_tkinter_vwait_mainloop_stdin.py
import unittest
import subprocess
import sys
import time
from test import support
from test.support import import_helper

tkinter = import_helper.import_module("tkinter")


@unittest.skipUnless(support.has_subprocess_support, "test requires subprocess")
@unittest.skipIf(sys.platform == "win32", "test is not supported on Windows")
class TkVwaitMainloopStdinTest(unittest.TestCase):

    def run_child(self):
        # Full Python script to execute in the child
        code = r"""
import tkinter as tk
import time

interp = tk.Tcl()

def do_vwait():
    start_wall = time.time()
    start_cpu = time.process_time()
    interp.eval("vwait myvar")
    end_cpu = time.process_time()
    end_wall = time.time()
    cpu_frac = (end_cpu - start_cpu) / (end_wall - start_wall)
    print(cpu_frac)

# Schedule vwait and release
interp.after(100, do_vwait)
interp.after(500, lambda: interp.setvar("myvar", "done"))
"""

        # Start child in interactive mode, but use -c to execute code immediately
        proc = subprocess.Popen(
            [sys.executable, "-i", "-c", code],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        return proc

    def test_vwait_stdin_busy_loop(self):
        proc = self.run_child()

        # Wait until vwait likely started
        time.sleep(0.15)

        # Send input to stdin to trigger the bug
        proc.stdin.write(b"\n")

        # Ensure child exits cleanly
        proc.stdin.write(b"exit()\n")

        stdout, stderr = proc.communicate()
        out = stdout.decode("utf-8", errors="replace")
        err = stderr.decode("utf-8", errors="replace")

        if proc.returncode != 0:
            self.fail(f"Child exited with {proc.returncode}\nSTDOUT:\n{out}\nSTDERR:\n{err}")

        # Extract CPU fraction printed by child
        cpu_frac = float(out)

        # Fail if CPU fraction is too high (indicative of busy-loop)
        self.assertLess(cpu_frac, 0.5,
            f"CPU usage too high during vwait with stdin input (fraction={cpu_frac:.2f})")


if __name__ == "__main__":
    unittest.main()
