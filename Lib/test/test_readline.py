"""
Very minimal unittests for parts of the readline module.
"""
from contextlib import ExitStack
from errno import EIO
import locale
import os
import selectors
import subprocess
import sys
import tempfile
import unittest
from test.support import verbose
from test.support.import_helper import import_module
from test.support.os_helper import unlink, temp_dir, TESTFN
from test.support.script_helper import assert_python_ok

# Skip tests if there is no readline module
readline = import_module('readline')

if hasattr(readline, "_READLINE_LIBRARY_VERSION"):
    is_editline = ("EditLine wrapper" in readline._READLINE_LIBRARY_VERSION)
else:
    is_editline = (readline.__doc__ and "libedit" in readline.__doc__)


def setUpModule():
    if verbose:
        # Python implementations other than CPython may not have
        # these private attributes
        if hasattr(readline, "_READLINE_VERSION"):
            print(f"readline version: {readline._READLINE_VERSION:#x}")
            print(f"readline runtime version: {readline._READLINE_RUNTIME_VERSION:#x}")
        if hasattr(readline, "_READLINE_LIBRARY_VERSION"):
            print(f"readline library version: {readline._READLINE_LIBRARY_VERSION!r}")
        print(f"use libedit emulation? {is_editline}")


@unittest.skipUnless(hasattr(readline, "clear_history"),
                     "The history update test cannot be run because the "
                     "clear_history method is not available.")
class TestHistoryManipulation (unittest.TestCase):
    """
    These tests were added to check that the libedit emulation on OSX and the
    "real" readline have the same interface for history manipulation. That's
    why the tests cover only a small subset of the interface.
    """

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

    def test_nonascii_history(self):
        readline.clear_history()
        try:
            readline.add_history("entrée 1")
        except UnicodeEncodeError as err:
            self.skipTest("Locale cannot encode test data: " + format(err))
        readline.add_history("entrée 2")
        readline.replace_history_item(1, "entrée 22")
        readline.write_history_file(TESTFN)
        self.addCleanup(os.remove, TESTFN)
        readline.clear_history()
        readline.read_history_file(TESTFN)
        if is_editline:
            # An add_history() call seems to be required for get_history_
            # item() to register items from the file
            readline.add_history("dummy")
        self.assertEqual(readline.get_history_item(1), "entrée 1")
        self.assertEqual(readline.get_history_item(2), "entrée 22")


class TestReadline(unittest.TestCase):

    @unittest.skipIf(readline._READLINE_VERSION < 0x0601 and not is_editline,
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

    def test_nonascii(self):
        loc = locale.setlocale(locale.LC_CTYPE, None)
        if loc in ('C', 'POSIX'):
            # bpo-29240: On FreeBSD, if the LC_CTYPE locale is C or POSIX,
            # writing and reading non-ASCII bytes into/from a TTY works, but
            # readline or ncurses ignores non-ASCII bytes on read.
            self.skipTest(f"the LC_CTYPE locale is {loc!r}")

        try:
            readline.add_history("\xEB\xEF")
        except UnicodeEncodeError as err:
            self.skipTest("Locale cannot encode test data: " + format(err))

        script = r"""import readline

is_editline = readline.__doc__ and "libedit" in readline.__doc__
inserted = "[\xEFnserted]"
macro = "|t\xEB[after]"
set_pre_input_hook = getattr(readline, "set_pre_input_hook", None)
if is_editline or not set_pre_input_hook:
    # The insert_line() call via pre_input_hook() does nothing with Editline,
    # so include the extra text that would have been inserted here
    macro = inserted + macro

if is_editline:
    readline.parse_and_bind(r'bind ^B ed-prev-char')
    readline.parse_and_bind(r'bind "\t" rl_complete')
    readline.parse_and_bind(r'bind -s ^A "{}"'.format(macro))
else:
    readline.parse_and_bind(r'Control-b: backward-char')
    readline.parse_and_bind(r'"\t": complete')
    readline.parse_and_bind(r'set disable-completion off')
    readline.parse_and_bind(r'set show-all-if-ambiguous off')
    readline.parse_and_bind(r'set show-all-if-unmodified off')
    readline.parse_and_bind(r'Control-a: "{}"'.format(macro))

def pre_input_hook():
    readline.insert_text(inserted)
    readline.redisplay()
if set_pre_input_hook:
    set_pre_input_hook(pre_input_hook)

def completer(text, state):
    if text == "t\xEB":
        if state == 0:
            print("text", ascii(text))
            print("line", ascii(readline.get_line_buffer()))
            print("indexes", readline.get_begidx(), readline.get_endidx())
            return "t\xEBnt"
        if state == 1:
            return "t\xEBxt"
    if text == "t\xEBx" and state == 0:
        return "t\xEBxt"
    return None
readline.set_completer(completer)

def display(substitution, matches, longest_match_length):
    print("substitution", ascii(substitution))
    print("matches", ascii(matches))
readline.set_completion_display_matches_hook(display)

print("result", ascii(input()))
print("history", ascii(readline.get_history_item(1)))
"""

        input = b"\x01"  # Ctrl-A, expands to "|t\xEB[after]"
        input += b"\x02" * len("[after]")  # Move cursor back
        input += b"\t\t"  # Display possible completions
        input += b"x\t"  # Complete "t\xEBx" -> "t\xEBxt"
        input += b"\r"
        output = run_pty(script, input)
        self.assertIn(b"text 't\\xeb'\r\n", output)
        self.assertIn(b"line '[\\xefnserted]|t\\xeb[after]'\r\n", output)
        self.assertIn(b"indexes 11 13\r\n", output)
        if not is_editline and hasattr(readline, "set_pre_input_hook"):
            self.assertIn(b"substitution 't\\xeb'\r\n", output)
            self.assertIn(b"matches ['t\\xebnt', 't\\xebxt']\r\n", output)
        expected = br"'[\xefnserted]|t\xebxt[after]'"
        self.assertIn(b"result " + expected + b"\r\n", output)
        self.assertIn(b"history " + expected + b"\r\n", output)

    # We have 2 reasons to skip this test:
    # - readline: history size was added in 6.0
    #   See https://cnswww.cns.cwru.edu/php/chet/readline/CHANGES
    # - editline: history size is broken on OS X 10.11.6.
    #   Newer versions were not tested yet.
    @unittest.skipIf(readline._READLINE_VERSION < 0x600,
                     "this readline version does not support history-size")
    @unittest.skipIf(is_editline,
                     "editline history size configuration is broken")
    def test_history_size(self):
        history_size = 10
        with temp_dir() as test_dir:
            inputrc = os.path.join(test_dir, "inputrc")
            with open(inputrc, "wb") as f:
                f.write(b"set history-size %d\n" % history_size)

            history_file = os.path.join(test_dir, "history")
            with open(history_file, "wb") as f:
                # history_size * 2 items crashes readline
                data = b"".join(b"item %d\n" % i
                                for i in range(history_size * 2))
                f.write(data)

            script = """
import os
import readline

history_file = os.environ["HISTORY_FILE"]
readline.read_history_file(history_file)
input()
readline.write_history_file(history_file)
"""

            env = dict(os.environ)
            env["INPUTRC"] = inputrc
            env["HISTORY_FILE"] = history_file

            run_pty(script, input=b"last input\r", env=env)

            with open(history_file, "rb") as f:
                lines = f.readlines()
            self.assertEqual(len(lines), history_size)
            self.assertEqual(lines[-1].strip(), b"last input")


def run_pty(script, input=b"dummy input\r", env=None):
    pty = import_module('pty')
    output = bytearray()
    [master, slave] = pty.openpty()
    args = (sys.executable, '-c', script)
    proc = subprocess.Popen(args, stdin=slave, stdout=slave, stderr=slave, env=env)
    os.close(slave)
    with ExitStack() as cleanup:
        cleanup.enter_context(proc)
        def terminate(proc):
            try:
                proc.terminate()
            except ProcessLookupError:
                # Workaround for Open/Net BSD bug (Issue 16762)
                pass
        cleanup.callback(terminate, proc)
        cleanup.callback(os.close, master)
        # Avoid using DefaultSelector and PollSelector. Kqueue() does not
        # work with pseudo-terminals on OS X < 10.9 (Issue 20365) and Open
        # BSD (Issue 20667). Poll() does not work with OS X 10.6 or 10.4
        # either (Issue 20472). Hopefully the file descriptor is low enough
        # to use with select().
        sel = cleanup.enter_context(selectors.SelectSelector())
        sel.register(master, selectors.EVENT_READ | selectors.EVENT_WRITE)
        os.set_blocking(master, False)
        while True:
            for [_, events] in sel.select():
                if events & selectors.EVENT_READ:
                    try:
                        chunk = os.read(master, 0x10000)
                    except OSError as err:
                        # Linux raises EIO when slave is closed (Issue 5380)
                        if err.errno != EIO:
                            raise
                        chunk = b""
                    if not chunk:
                        return output
                    output.extend(chunk)
                if events & selectors.EVENT_WRITE:
                    try:
                        input = input[os.write(master, input):]
                    except OSError as err:
                        # Apparently EIO means the slave was closed
                        if err.errno != EIO:
                            raise
                        input = b""  # Stop writing
                    if not input:
                        sel.modify(master, selectors.EVENT_READ)


if __name__ == "__main__":
    unittest.main()
