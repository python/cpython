"""
Very minimal unittests for parts of the readline module.
"""
from contextlib import ExitStack
from errno import EIO
import os
import selectors
import subprocess
import sys
import tempfile
import unittest
from test.support import import_module, unlink, get_original_stdout
from test.support.script_helper import assert_python_ok

# Skip tests if there is no readline module
readline = import_module('readline')

class TestHistoryManipulation (unittest.TestCase):
    """
    These tests were added to check that the libedit emulation on OSX and the
    "real" readline have the same interface for history manipulation. That's
    why the tests cover only a small subset of the interface.
    """

    @unittest.skipUnless(hasattr(readline, "clear_history"),
                         "The history update test cannot be run because the "
                         "clear_history method is not available.")
    def testHistoryUpdates(self):
        readline.clear_history()

        readline.add_history("first line")
        readline.add_history("second line")

        self.assertEqual(readline.get_history_item(0), None)
        self.assertEqual(readline.get_history_item(1), "first line")
        self.assertEqual(readline.get_history_item(2), "second line")

        readline.replace_history_item(0, "replaced line")
        self.assertEqual(readline.get_history_item(0), None)
        self.assertEqual(readline.get_history_item(1), "replaced line")
        self.assertEqual(readline.get_history_item(2), "second line")

        self.assertEqual(readline.get_current_history_length(), 2)

        readline.remove_history_item(0)
        self.assertEqual(readline.get_history_item(0), None)
        self.assertEqual(readline.get_history_item(1), "second line")

        self.assertEqual(readline.get_current_history_length(), 1)

    @unittest.skipUnless(hasattr(readline, "append_history_file"),
                         "append_history not available")
    def test_write_read_append(self):
        hfile = tempfile.NamedTemporaryFile(delete=False)
        hfile.close()
        hfilename = hfile.name
        self.addCleanup(unlink, hfilename)

        # test write-clear-read == nop
        readline.clear_history()
        readline.add_history("first line")
        readline.add_history("second line")
        readline.write_history_file(hfilename)

        readline.clear_history()
        self.assertEqual(readline.get_current_history_length(), 0)

        readline.read_history_file(hfilename)
        self.assertEqual(readline.get_current_history_length(), 2)
        self.assertEqual(readline.get_history_item(1), "first line")
        self.assertEqual(readline.get_history_item(2), "second line")

        # test append
        readline.append_history_file(1, hfilename)
        readline.clear_history()
        readline.read_history_file(hfilename)
        self.assertEqual(readline.get_current_history_length(), 3)
        self.assertEqual(readline.get_history_item(1), "first line")
        self.assertEqual(readline.get_history_item(2), "second line")
        self.assertEqual(readline.get_history_item(3), "second line")

        # test 'no such file' behaviour
        os.unlink(hfilename)
        with self.assertRaises(FileNotFoundError):
            readline.append_history_file(1, hfilename)

        # write_history_file can create the target
        readline.write_history_file(hfilename)


class TestReadline(unittest.TestCase):

    @unittest.skipIf(readline._READLINE_VERSION < 0x0600
                     and "libedit" not in readline.__doc__,
                     "not supported in this library version")
    def test_init(self):
        # Issue #19884: Ensure that the ANSI sequence "\033[1034h" is not
        # written into stdout when the readline module is imported and stdout
        # is redirected to a pipe.
        rc, stdout, stderr = assert_python_ok('-c', 'import readline',
                                              TERM='xterm-256color')
        self.assertEqual(stdout, b'')

    auto_history_script = """\
import readline
readline.set_auto_history({})
input()
print("History length:", readline.get_current_history_length())
"""

    def test_auto_history_enabled(self):
        output = run_pty(self.auto_history_script.format(True))
        self.assertIn(b"History length: 1\r\n", output)

    def test_auto_history_disabled(self):
        output = run_pty(self.auto_history_script.format(False))
        self.assertIn(b"History length: 0\r\n", output)


def run_pty(script, input=b"dummy input\r"):
    pty = import_module('pty')
    output = bytearray()
    [master, slave] = pty.openpty()
    args = (sys.executable, '-c', script)
    proc = subprocess.Popen(args, stdin=slave, stdout=slave, stderr=slave)
    os.close(slave)
    with ExitStack() as cleanup:
        cleanup.enter_context(proc)
        cleanup.callback(os.close, master)
        sel = cleanup.enter_context(selectors.DefaultSelector())
        sel.register(master, selectors.EVENT_READ | selectors.EVENT_WRITE)
        os.set_blocking(master, False)
        while True:
            get_original_stdout().write(f"test_readline: select()\n")
            for [_, events] in sel.select():
                if events & selectors.EVENT_READ:
                    try:
                        get_original_stdout().write(f"test_readline: read()\n")
                        chunk = os.read(master, 0x10000)
                    except OSError as err:
                        # Linux raises EIO when the slave is closed
                        if err.errno != EIO:
                            raise
                        chunk = b""
                    get_original_stdout().write(f"test_readline: read {chunk!r}\n")
                    if not chunk:
                        return output
                    output.extend(chunk)
                if events & selectors.EVENT_WRITE:
                    get_original_stdout().write(f"test_readline: write()\n")
                    input = input[os.write(master, input):]
                    get_original_stdout().write(f"test_readline: remaining input = {input!r}\n")
                    if not input:
                        sel.modify(master, selectors.EVENT_READ)


if __name__ == "__main__":
    unittest.main()
