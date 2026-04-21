# test_tkinter_pipe.py
import unittest
import subprocess
import sys
from test import support


@unittest.skipUnless(support.has_subprocess_support, "test requires subprocess")
class TkinterPipeTest(unittest.TestCase):

    def test_tkinter_pipe_buffered(self):
        args = [sys.executable, "-i"]
        proc = subprocess.Popen(args,
                                 stdin=subprocess.PIPE,
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE)
        proc.stdin.write(b"import tkinter\n")
        proc.stdin.write(b"interpreter = tkinter.Tcl()\n")
        proc.stdin.write(b"print('hello')\n")
        proc.stdin.write(b"print('goodbye')\n")
        proc.stdin.write(b"quit()\n")
        stdout, stderr = proc.communicate()
        stdout = stdout.decode()
        self.assertEqual(stdout.split(), ['hello', 'goodbye'])

    def test_tkinter_pipe_unbuffered(self):
        args = [sys.executable, "-i", "-u"]
        proc = subprocess.Popen(args,
                                 stdin=subprocess.PIPE,
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE)
        proc.stdin.write(b"import tkinter\n")
        proc.stdin.write(b"interpreter = tkinter.Tcl()\n")

        proc.stdin.write(b"print('hello')\n")
        proc.stdin.flush()
        stdout = proc.stdout.readline()
        stdout = stdout.decode()
        self.assertEqual(stdout.strip(), 'hello')

        proc.stdin.write(b"print('hello again')\n")
        proc.stdin.flush()
        stdout = proc.stdout.readline()
        stdout = stdout.decode()
        self.assertEqual(stdout.strip(), 'hello again')

        proc.stdin.write(b"print('goodbye')\n")
        proc.stdin.write(b"quit()\n")
        stdout, stderr = proc.communicate()
        stdout = stdout.decode()
        self.assertEqual(stdout.strip(), 'goodbye')


if __name__ == "__main__":
    unittest.main()
