import unittest
import subprocess
import sys
from test import support
from test.support import import_helper

tkinter = import_helper.import_module("tkinter")


@unittest.skipUnless(support.has_subprocess_support, "test requires subprocess")
class TkStdinPipe(unittest.TestCase):

    def test_pipe_stdin(self):
        proc = subprocess.Popen([sys.executable, "-i"],
                                 stdin=subprocess.PIPE,
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE)
        proc.stdin.write(b"import tkinter\n")
        proc.stdin.write(b"interpreter = tkinter.Tcl()\n")
        proc.stdin.write(b"print('hello')\n")
        proc.stdin.write(b"quit()\n")

        stdout, stderr = proc.communicate()
        stdout = stdout.decode()
        stderr = stderr.decode()

        if proc.returncode != 0:
            self.fail(f"Child exited with {proc.returncode}\nSTDOUT:\n{out}\nSTDERR:\n{err}")

        self.assertEqual(stdout.rstrip(), "hello")


if __name__ == "__main__":
    unittest.main()
