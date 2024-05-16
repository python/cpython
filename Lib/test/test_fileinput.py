'''
Tests for fileinput module.
Nick Mathewson
'''
import io
import os
import sys
import re
import fileinput
import collections
import builtins
import tempfile
import unittest

try:
    import bz2
except ImportError:
    bz2 = None
try:
    import gzip
except ImportError:
    gzip = None

from io import BytesIO, StringIO
from fileinput import FileInput, hook_encoded

from test.support import verbose
from test.support.os_helper import TESTFN, FakePath
from test.support.os_helper import unlink as safe_unlink
from test.support import os_helper
from test import support
from unittest import mock


# The fileinput module has 2 interfaces: the FileInput class which does
# all the work, and a few functions (input, etc.) that use a global _state
# variable.

class BaseTests:
    # Write a content (str or bytes) to temp file, and return the
    # temp file's name.
    def writeTmp(self, content, *, mode='w'):  # opening in text mode is the default
        fd, name = tempfile.mkstemp()
        self.addCleanup(os_helper.unlink, name)
        encoding = None if "b" in mode else "utf-8"
        with open(fd, mode, encoding=encoding) as f:
            f.write(content)
        return name

class LineReader:

    def __init__(self):
        self._linesread = []

    @property
    def linesread(self):
        try:
            return self._linesread[:]
        finally:
            self._linesread = []

    def openhook(self, filename, mode):
        self.it = iter(filename.splitlines(True))
        return self

    def readline(self, size=None):
        line = next(self.it, '')
        self._linesread.append(line)
        return line

    def readlines(self, hint=-1):
        lines = []
        size = 0
        while True:
            line = self.readline()
            if not line:
                return lines
            lines.append(line)
            size += len(line)
            if size >= hint:
                return lines

    def close(self):
        pass

class BufferSizesTests(BaseTests, unittest.TestCase):
    def test_buffer_sizes(self):

        t1 = self.writeTmp(''.join("Line %s of file 1\n" % (i+1) for i in range(15)))
        t2 = self.writeTmp(''.join("Line %s of file 2\n" % (i+1) for i in range(10)))
        t3 = self.writeTmp(''.join("Line %s of file 3\n" % (i+1) for i in range(5)))
        t4 = self.writeTmp(''.join("Line %s of file 4\n" % (i+1) for i in range(1)))

        pat = re.compile(r'LINE (\d+) OF FILE (\d+)')

        if verbose:
            print('1. Simple iteration')
        fi = FileInput(files=(t1, t2, t3, t4), encoding="utf-8")
        lines = list(fi)
        fi.close()
        self.assertEqual(len(lines), 31)
        self.assertEqual(lines[4], 'Line 5 of file 1\n')
        self.assertEqual(lines[30], 'Line 1 of file 4\n')
        self.assertEqual(fi.lineno(), 31)
        self.assertEqual(fi.filename(), t4)

        if verbose:
            print('2. Status variables')
        fi = FileInput(files=(t1, t2, t3, t4), encoding="utf-8")
        s = "x"
        while s and s != 'Line 6 of file 2\n':
            s = fi.readline()
        self.assertEqual(fi.filename(), t2)
        self.assertEqual(fi.lineno(), 21)
        self.assertEqual(fi.filelineno(), 6)
        self.assertFalse(fi.isfirstline())
        self.assertFalse(fi.isstdin())

        if verbose:
            print('3. Nextfile')
        fi.nextfile()
        self.assertEqual(fi.readline(), 'Line 1 of file 3\n')
        self.assertEqual(fi.lineno(), 22)
        fi.close()

        if verbose:
            print('4. Stdin')
        fi = FileInput(files=(t1, t2, t3, t4, '-'), encoding="utf-8")
        savestdin = sys.stdin
        try:
            sys.stdin = StringIO("Line 1 of stdin\nLine 2 of stdin\n")
            lines = list(fi)
            self.assertEqual(len(lines), 33)
            self.assertEqual(lines[32], 'Line 2 of stdin\n')
            self.assertEqual(fi.filename(), '<stdin>')
            fi.nextfile()
        finally:
            sys.stdin = savestdin

        if verbose:
            print('5. Boundary conditions')
        fi = FileInput(files=(t1, t2, t3, t4), encoding="utf-8")
        self.assertEqual(fi.lineno(), 0)
        self.assertEqual(fi.filename(), None)
        fi.nextfile()
        self.assertEqual(fi.lineno(), 0)
        self.assertEqual(fi.filename(), None)

        if verbose:
            print('6. Inplace')
        savestdout = sys.stdout
        try:
            fi = FileInput(files=(t1, t2, t3, t4), inplace=1, encoding="utf-8")
            for line in fi:
                line = line[:-1].upper()
                print(line)
            fi.close()
        finally:
            sys.stdout = savestdout

        fi = FileInput(files=(t1, t2, t3, t4), encoding="utf-8")
        for line in fi:
            self.assertEqual(line[-1], '\n')
            m = pat.match(line[:-1])
            self.assertNotEqual(m, None)
            self.assertEqual(int(m.group(1)), fi.filelineno())
        fi.close()

class UnconditionallyRaise:
    def __init__(self, exception_type):
        self.exception_type = exception_type
        self.invoked = False
    def __call__(self, *args, **kwargs):
        self.invoked = True
        raise self.exception_type()

class FileInputTests(BaseTests, unittest.TestCase):

    def test_zero_byte_files(self):
        t1 = self.writeTmp("")
        t2 = self.writeTmp("")
        t3 = self.writeTmp("The only line there is.\n")
        t4 = self.writeTmp("")
        fi = FileInput(files=(t1, t2, t3, t4), encoding="utf-8")

        line = fi.readline()
        self.assertEqual(line, 'The only line there is.\n')
        self.assertEqual(fi.lineno(), 1)
        self.assertEqual(fi.filelineno(), 1)
        self.assertEqual(fi.filename(), t3)

        line = fi.readline()
        self.assertFalse(line)
        self.assertEqual(fi.lineno(), 1)
        self.assertEqual(fi.filelineno(), 0)
        self.assertEqual(fi.filename(), t4)
        fi.close()

    def test_files_that_dont_end_with_newline(self):
        t1 = self.writeTmp("A\nB\nC")
        t2 = self.writeTmp("D\nE\nF")
        fi = FileInput(files=(t1, t2), encoding="utf-8")
        lines = list(fi)
        self.assertEqual(lines, ["A\n", "B\n", "C", "D\n", "E\n", "F"])
        self.assertEqual(fi.filelineno(), 3)
        self.assertEqual(fi.lineno(), 6)

##     def test_unicode_filenames(self):
##         # XXX A unicode string is always returned by writeTmp.
##         #     So is this needed?
##         t1 = self.writeTmp("A\nB")
##         encoding = sys.getfilesystemencoding()
##         if encoding is None:
##             encoding = 'ascii'
##         fi = FileInput(files=str(t1, encoding), encoding="utf-8")
##         lines = list(fi)
##         self.assertEqual(lines, ["A\n", "B"])

    def test_fileno(self):
        t1 = self.writeTmp("A\nB")
        t2 = self.writeTmp("C\nD")
        fi = FileInput(files=(t1, t2), encoding="utf-8")
        self.assertEqual(fi.fileno(), -1)
        line = next(fi)
        self.assertNotEqual(fi.fileno(), -1)
        fi.nextfile()
        self.assertEqual(fi.fileno(), -1)
        line = list(fi)
        self.assertEqual(fi.fileno(), -1)

    def test_invalid_opening_mode(self):
        for mode in ('w', 'rU', 'U'):
            with self.subTest(mode=mode):
                with self.assertRaises(ValueError):
                    FileInput(mode=mode)

    def test_stdin_binary_mode(self):
        with mock.patch('sys.stdin') as m_stdin:
            m_stdin.buffer = BytesIO(b'spam, bacon, sausage, and spam')
            fi = FileInput(files=['-'], mode='rb')
            lines = list(fi)
            self.assertEqual(lines, [b'spam, bacon, sausage, and spam'])

    def test_detached_stdin_binary_mode(self):
        orig_stdin = sys.stdin
        try:
            sys.stdin = BytesIO(b'spam, bacon, sausage, and spam')
            self.assertFalse(hasattr(sys.stdin, 'buffer'))
            fi = FileInput(files=['-'], mode='rb')
            lines = list(fi)
            self.assertEqual(lines, [b'spam, bacon, sausage, and spam'])
        finally:
            sys.stdin = orig_stdin

    def test_file_opening_hook(self):
        try:
            # cannot use openhook and inplace mode
            fi = FileInput(inplace=1, openhook=lambda f, m: None)
            self.fail("FileInput should raise if both inplace "
                             "and openhook arguments are given")
        except ValueError:
            pass
        try:
            fi = FileInput(openhook=1)
            self.fail("FileInput should check openhook for being callable")
        except ValueError:
            pass

        class CustomOpenHook:
            def __init__(self):
                self.invoked = False
            def __call__(self, *args, **kargs):
                self.invoked = True
                return open(*args, encoding="utf-8")

        t = self.writeTmp("\n")
        custom_open_hook = CustomOpenHook()
        with FileInput([t], openhook=custom_open_hook) as fi:
            fi.readline()
        self.assertTrue(custom_open_hook.invoked, "openhook not invoked")

    def test_readline(self):
        with open(TESTFN, 'wb') as f:
            f.write(b'A\nB\r\nC\r')
            # Fill TextIOWrapper buffer.
            f.write(b'123456789\n' * 1000)
            # Issue #20501: readline() shouldn't read whole file.
            f.write(b'\x80')
        self.addCleanup(safe_unlink, TESTFN)

        with FileInput(files=TESTFN,
                       openhook=hook_encoded('ascii')) as fi:
            try:
                self.assertEqual(fi.readline(), 'A\n')
                self.assertEqual(fi.readline(), 'B\n')
                self.assertEqual(fi.readline(), 'C\n')
            except UnicodeDecodeError:
                self.fail('Read to end of file')
            with self.assertRaises(UnicodeDecodeError):
                # Read to the end of file.
                list(fi)
            self.assertEqual(fi.readline(), '')
            self.assertEqual(fi.readline(), '')

    def test_readline_binary_mode(self):
        with open(TESTFN, 'wb') as f:
            f.write(b'A\nB\r\nC\rD')
        self.addCleanup(safe_unlink, TESTFN)

        with FileInput(files=TESTFN, mode='rb') as fi:
            self.assertEqual(fi.readline(), b'A\n')
            self.assertEqual(fi.readline(), b'B\r\n')
            self.assertEqual(fi.readline(), b'C\rD')
            # Read to the end of file.
            self.assertEqual(fi.readline(), b'')
            self.assertEqual(fi.readline(), b'')

    def test_inplace_binary_write_mode(self):
        temp_file = self.writeTmp(b'Initial text.', mode='wb')
        with FileInput(temp_file, mode='rb', inplace=True) as fobj:
            line = fobj.readline()
            self.assertEqual(line, b'Initial text.')
            # print() cannot be used with files opened in binary mode.
            sys.stdout.write(b'New line.')
        with open(temp_file, 'rb') as f:
            self.assertEqual(f.read(), b'New line.')

    def test_inplace_encoding_errors(self):
        temp_file = self.writeTmp(b'Initial text \x88', mode='wb')
        with FileInput(temp_file, inplace=True,
                       encoding="ascii", errors="replace") as fobj:
            line = fobj.readline()
            self.assertEqual(line, 'Initial text \ufffd')
            print("New line \x88")
        with open(temp_file, 'rb') as f:
            self.assertEqual(f.read().rstrip(b'\r\n'), b'New line ?')

    def test_file_hook_backward_compatibility(self):
        def old_hook(filename, mode):
            return io.StringIO("I used to receive only filename and mode")
        t = self.writeTmp("\n")
        with FileInput([t], openhook=old_hook) as fi:
            result = fi.readline()
        self.assertEqual(result, "I used to receive only filename and mode")

    def test_context_manager(self):
        t1 = self.writeTmp("A\nB\nC")
        t2 = self.writeTmp("D\nE\nF")
        with FileInput(files=(t1, t2), encoding="utf-8") as fi:
            lines = list(fi)
        self.assertEqual(lines, ["A\n", "B\n", "C", "D\n", "E\n", "F"])
        self.assertEqual(fi.filelineno(), 3)
        self.assertEqual(fi.lineno(), 6)
        self.assertEqual(fi._files, ())

    def test_close_on_exception(self):
        t1 = self.writeTmp("")
        try:
            with FileInput(files=t1, encoding="utf-8") as fi:
                raise OSError
        except OSError:
            self.assertEqual(fi._files, ())

    def test_empty_files_list_specified_to_constructor(self):
        with FileInput(files=[], encoding="utf-8") as fi:
            self.assertEqual(fi._files, ('-',))

    def test_nextfile_oserror_deleting_backup(self):
        """Tests invoking FileInput.nextfile() when the attempt to delete
           the backup file would raise OSError.  This error is expected to be
           silently ignored"""

        os_unlink_orig = os.unlink
        os_unlink_replacement = UnconditionallyRaise(OSError)
        try:
            t = self.writeTmp("\n")
            self.addCleanup(safe_unlink, t + '.bak')
            with FileInput(files=[t], inplace=True, encoding="utf-8") as fi:
                next(fi) # make sure the file is opened
                os.unlink = os_unlink_replacement
                fi.nextfile()
        finally:
            os.unlink = os_unlink_orig

        # sanity check to make sure that our test scenario was actually hit
        self.assertTrue(os_unlink_replacement.invoked,
                        "os.unlink() was not invoked")

    def test_readline_os_fstat_raises_OSError(self):
        """Tests invoking FileInput.readline() when os.fstat() raises OSError.
           This exception should be silently discarded."""

        os_fstat_orig = os.fstat
        os_fstat_replacement = UnconditionallyRaise(OSError)
        try:
            t = self.writeTmp("\n")
            with FileInput(files=[t], inplace=True, encoding="utf-8") as fi:
                os.fstat = os_fstat_replacement
                fi.readline()
        finally:
            os.fstat = os_fstat_orig

        # sanity check to make sure that our test scenario was actually hit
        self.assertTrue(os_fstat_replacement.invoked,
                        "os.fstat() was not invoked")

    def test_readline_os_chmod_raises_OSError(self):
        """Tests invoking FileInput.readline() when os.chmod() raises OSError.
           This exception should be silently discarded."""

        os_chmod_orig = os.chmod
        os_chmod_replacement = UnconditionallyRaise(OSError)
        try:
            t = self.writeTmp("\n")
            with FileInput(files=[t], inplace=True, encoding="utf-8") as fi:
                os.chmod = os_chmod_replacement
                fi.readline()
        finally:
            os.chmod = os_chmod_orig

        # sanity check to make sure that our test scenario was actually hit
        self.assertTrue(os_chmod_replacement.invoked,
                        "os.fstat() was not invoked")

    def test_fileno_when_ValueError_raised(self):
        class FilenoRaisesValueError(UnconditionallyRaise):
            def __init__(self):
                UnconditionallyRaise.__init__(self, ValueError)
            def fileno(self):
                self.__call__()

        unconditionally_raise_ValueError = FilenoRaisesValueError()
        t = self.writeTmp("\n")
        with FileInput(files=[t], encoding="utf-8") as fi:
            file_backup = fi._file
            try:
                fi._file = unconditionally_raise_ValueError
                result = fi.fileno()
            finally:
                fi._file = file_backup # make sure the file gets cleaned up

        # sanity check to make sure that our test scenario was actually hit
        self.assertTrue(unconditionally_raise_ValueError.invoked,
                        "_file.fileno() was not invoked")

        self.assertEqual(result, -1, "fileno() should return -1")

    def test_readline_buffering(self):
        src = LineReader()
        with FileInput(files=['line1\nline2', 'line3\n'],
                       openhook=src.openhook) as fi:
            self.assertEqual(src.linesread, [])
            self.assertEqual(fi.readline(), 'line1\n')
            self.assertEqual(src.linesread, ['line1\n'])
            self.assertEqual(fi.readline(), 'line2')
            self.assertEqual(src.linesread, ['line2'])
            self.assertEqual(fi.readline(), 'line3\n')
            self.assertEqual(src.linesread, ['', 'line3\n'])
            self.assertEqual(fi.readline(), '')
            self.assertEqual(src.linesread, [''])
            self.assertEqual(fi.readline(), '')
            self.assertEqual(src.linesread, [])

    def test_iteration_buffering(self):
        src = LineReader()
        with FileInput(files=['line1\nline2', 'line3\n'],
                       openhook=src.openhook) as fi:
            self.assertEqual(src.linesread, [])
            self.assertEqual(next(fi), 'line1\n')
            self.assertEqual(src.linesread, ['line1\n'])
            self.assertEqual(next(fi), 'line2')
            self.assertEqual(src.linesread, ['line2'])
            self.assertEqual(next(fi), 'line3\n')
            self.assertEqual(src.linesread, ['', 'line3\n'])
            self.assertRaises(StopIteration, next, fi)
            self.assertEqual(src.linesread, [''])
            self.assertRaises(StopIteration, next, fi)
            self.assertEqual(src.linesread, [])

    def test_pathlike_file(self):
        t1 = FakePath(self.writeTmp("Path-like file."))
        with FileInput(t1, encoding="utf-8") as fi:
            line = fi.readline()
            self.assertEqual(line, 'Path-like file.')
            self.assertEqual(fi.lineno(), 1)
            self.assertEqual(fi.filelineno(), 1)
            self.assertEqual(fi.filename(), os.fspath(t1))

    def test_pathlike_file_inplace(self):
        t1 = FakePath(self.writeTmp('Path-like file.'))
        with FileInput(t1, inplace=True, encoding="utf-8") as fi:
            line = fi.readline()
            self.assertEqual(line, 'Path-like file.')
            print('Modified %s' % line)
        with open(t1, encoding="utf-8") as f:
            self.assertEqual(f.read(), 'Modified Path-like file.\n')


class MockFileInput:
    """A class that mocks out fileinput.FileInput for use during unit tests"""

    def __init__(self, files=None, inplace=False, backup="", *,
                 mode="r", openhook=None, encoding=None, errors=None):
        self.files = files
        self.inplace = inplace
        self.backup = backup
        self.mode = mode
        self.openhook = openhook
        self.encoding = encoding
        self.errors = errors
        self._file = None
        self.invocation_counts = collections.defaultdict(lambda: 0)
        self.return_values = {}

    def close(self):
        self.invocation_counts["close"] += 1

    def nextfile(self):
        self.invocation_counts["nextfile"] += 1
        return self.return_values["nextfile"]

    def filename(self):
        self.invocation_counts["filename"] += 1
        return self.return_values["filename"]

    def lineno(self):
        self.invocation_counts["lineno"] += 1
        return self.return_values["lineno"]

    def filelineno(self):
        self.invocation_counts["filelineno"] += 1
        return self.return_values["filelineno"]

    def fileno(self):
        self.invocation_counts["fileno"] += 1
        return self.return_values["fileno"]

    def isfirstline(self):
        self.invocation_counts["isfirstline"] += 1
        return self.return_values["isfirstline"]

    def isstdin(self):
        self.invocation_counts["isstdin"] += 1
        return self.return_values["isstdin"]

class BaseFileInputGlobalMethodsTest(unittest.TestCase):
    """Base class for unit tests for the global function of
       the fileinput module."""

    def setUp(self):
        self._orig_state = fileinput._state
        self._orig_FileInput = fileinput.FileInput
        fileinput.FileInput = MockFileInput

    def tearDown(self):
        fileinput.FileInput = self._orig_FileInput
        fileinput._state = self._orig_state

    def assertExactlyOneInvocation(self, mock_file_input, method_name):
        # assert that the method with the given name was invoked once
        actual_count = mock_file_input.invocation_counts[method_name]
        self.assertEqual(actual_count, 1, method_name)
        # assert that no other unexpected methods were invoked
        actual_total_count = len(mock_file_input.invocation_counts)
        self.assertEqual(actual_total_count, 1)

class Test_fileinput_input(BaseFileInputGlobalMethodsTest):
    """Unit tests for fileinput.input()"""

    def test_state_is_not_None_and_state_file_is_not_None(self):
        """Tests invoking fileinput.input() when fileinput._state is not None
           and its _file attribute is also not None.  Expect RuntimeError to
           be raised with a meaningful error message and for fileinput._state
           to *not* be modified."""
        instance = MockFileInput()
        instance._file = object()
        fileinput._state = instance
        with self.assertRaises(RuntimeError) as cm:
            fileinput.input()
        self.assertEqual(("input() already active",), cm.exception.args)
        self.assertIs(instance, fileinput._state, "fileinput._state")

    def test_state_is_not_None_and_state_file_is_None(self):
        """Tests invoking fileinput.input() when fileinput._state is not None
           but its _file attribute *is* None.  Expect it to create and return
           a new fileinput.FileInput object with all method parameters passed
           explicitly to the __init__() method; also ensure that
           fileinput._state is set to the returned instance."""
        instance = MockFileInput()
        instance._file = None
        fileinput._state = instance
        self.do_test_call_input()

    def test_state_is_None(self):
        """Tests invoking fileinput.input() when fileinput._state is None
           Expect it to create and return a new fileinput.FileInput object
           with all method parameters passed explicitly to the __init__()
           method; also ensure that fileinput._state is set to the returned
           instance."""
        fileinput._state = None
        self.do_test_call_input()

    def do_test_call_input(self):
        """Tests that fileinput.input() creates a new fileinput.FileInput
           object, passing the given parameters unmodified to
           fileinput.FileInput.__init__().  Note that this test depends on the
           monkey patching of fileinput.FileInput done by setUp()."""
        files = object()
        inplace = object()
        backup = object()
        mode = object()
        openhook = object()
        encoding = object()

        # call fileinput.input() with different values for each argument
        result = fileinput.input(files=files, inplace=inplace, backup=backup,
            mode=mode, openhook=openhook, encoding=encoding)

        # ensure fileinput._state was set to the returned object
        self.assertIs(result, fileinput._state, "fileinput._state")

        # ensure the parameters to fileinput.input() were passed directly
        # to FileInput.__init__()
        self.assertIs(files, result.files, "files")
        self.assertIs(inplace, result.inplace, "inplace")
        self.assertIs(backup, result.backup, "backup")
        self.assertIs(mode, result.mode, "mode")
        self.assertIs(openhook, result.openhook, "openhook")

class Test_fileinput_close(BaseFileInputGlobalMethodsTest):
    """Unit tests for fileinput.close()"""

    def test_state_is_None(self):
        """Tests that fileinput.close() does nothing if fileinput._state
           is None"""
        fileinput._state = None
        fileinput.close()
        self.assertIsNone(fileinput._state)

    def test_state_is_not_None(self):
        """Tests that fileinput.close() invokes close() on fileinput._state
           and sets _state=None"""
        instance = MockFileInput()
        fileinput._state = instance
        fileinput.close()
        self.assertExactlyOneInvocation(instance, "close")
        self.assertIsNone(fileinput._state)

class Test_fileinput_nextfile(BaseFileInputGlobalMethodsTest):
    """Unit tests for fileinput.nextfile()"""

    def test_state_is_None(self):
        """Tests fileinput.nextfile() when fileinput._state is None.
           Ensure that it raises RuntimeError with a meaningful error message
           and does not modify fileinput._state"""
        fileinput._state = None
        with self.assertRaises(RuntimeError) as cm:
            fileinput.nextfile()
        self.assertEqual(("no active input()",), cm.exception.args)
        self.assertIsNone(fileinput._state)

    def test_state_is_not_None(self):
        """Tests fileinput.nextfile() when fileinput._state is not None.
           Ensure that it invokes fileinput._state.nextfile() exactly once,
           returns whatever it returns, and does not modify fileinput._state
           to point to a different object."""
        nextfile_retval = object()
        instance = MockFileInput()
        instance.return_values["nextfile"] = nextfile_retval
        fileinput._state = instance
        retval = fileinput.nextfile()
        self.assertExactlyOneInvocation(instance, "nextfile")
        self.assertIs(retval, nextfile_retval)
        self.assertIs(fileinput._state, instance)

class Test_fileinput_filename(BaseFileInputGlobalMethodsTest):
    """Unit tests for fileinput.filename()"""

    def test_state_is_None(self):
        """Tests fileinput.filename() when fileinput._state is None.
           Ensure that it raises RuntimeError with a meaningful error message
           and does not modify fileinput._state"""
        fileinput._state = None
        with self.assertRaises(RuntimeError) as cm:
            fileinput.filename()
        self.assertEqual(("no active input()",), cm.exception.args)
        self.assertIsNone(fileinput._state)

    def test_state_is_not_None(self):
        """Tests fileinput.filename() when fileinput._state is not None.
           Ensure that it invokes fileinput._state.filename() exactly once,
           returns whatever it returns, and does not modify fileinput._state
           to point to a different object."""
        filename_retval = object()
        instance = MockFileInput()
        instance.return_values["filename"] = filename_retval
        fileinput._state = instance
        retval = fileinput.filename()
        self.assertExactlyOneInvocation(instance, "filename")
        self.assertIs(retval, filename_retval)
        self.assertIs(fileinput._state, instance)

class Test_fileinput_lineno(BaseFileInputGlobalMethodsTest):
    """Unit tests for fileinput.lineno()"""

    def test_state_is_None(self):
        """Tests fileinput.lineno() when fileinput._state is None.
           Ensure that it raises RuntimeError with a meaningful error message
           and does not modify fileinput._state"""
        fileinput._state = None
        with self.assertRaises(RuntimeError) as cm:
            fileinput.lineno()
        self.assertEqual(("no active input()",), cm.exception.args)
        self.assertIsNone(fileinput._state)

    def test_state_is_not_None(self):
        """Tests fileinput.lineno() when fileinput._state is not None.
           Ensure that it invokes fileinput._state.lineno() exactly once,
           returns whatever it returns, and does not modify fileinput._state
           to point to a different object."""
        lineno_retval = object()
        instance = MockFileInput()
        instance.return_values["lineno"] = lineno_retval
        fileinput._state = instance
        retval = fileinput.lineno()
        self.assertExactlyOneInvocation(instance, "lineno")
        self.assertIs(retval, lineno_retval)
        self.assertIs(fileinput._state, instance)

class Test_fileinput_filelineno(BaseFileInputGlobalMethodsTest):
    """Unit tests for fileinput.filelineno()"""

    def test_state_is_None(self):
        """Tests fileinput.filelineno() when fileinput._state is None.
           Ensure that it raises RuntimeError with a meaningful error message
           and does not modify fileinput._state"""
        fileinput._state = None
        with self.assertRaises(RuntimeError) as cm:
            fileinput.filelineno()
        self.assertEqual(("no active input()",), cm.exception.args)
        self.assertIsNone(fileinput._state)

    def test_state_is_not_None(self):
        """Tests fileinput.filelineno() when fileinput._state is not None.
           Ensure that it invokes fileinput._state.filelineno() exactly once,
           returns whatever it returns, and does not modify fileinput._state
           to point to a different object."""
        filelineno_retval = object()
        instance = MockFileInput()
        instance.return_values["filelineno"] = filelineno_retval
        fileinput._state = instance
        retval = fileinput.filelineno()
        self.assertExactlyOneInvocation(instance, "filelineno")
        self.assertIs(retval, filelineno_retval)
        self.assertIs(fileinput._state, instance)

class Test_fileinput_fileno(BaseFileInputGlobalMethodsTest):
    """Unit tests for fileinput.fileno()"""

    def test_state_is_None(self):
        """Tests fileinput.fileno() when fileinput._state is None.
           Ensure that it raises RuntimeError with a meaningful error message
           and does not modify fileinput._state"""
        fileinput._state = None
        with self.assertRaises(RuntimeError) as cm:
            fileinput.fileno()
        self.assertEqual(("no active input()",), cm.exception.args)
        self.assertIsNone(fileinput._state)

    def test_state_is_not_None(self):
        """Tests fileinput.fileno() when fileinput._state is not None.
           Ensure that it invokes fileinput._state.fileno() exactly once,
           returns whatever it returns, and does not modify fileinput._state
           to point to a different object."""
        fileno_retval = object()
        instance = MockFileInput()
        instance.return_values["fileno"] = fileno_retval
        instance.fileno_retval = fileno_retval
        fileinput._state = instance
        retval = fileinput.fileno()
        self.assertExactlyOneInvocation(instance, "fileno")
        self.assertIs(retval, fileno_retval)
        self.assertIs(fileinput._state, instance)

class Test_fileinput_isfirstline(BaseFileInputGlobalMethodsTest):
    """Unit tests for fileinput.isfirstline()"""

    def test_state_is_None(self):
        """Tests fileinput.isfirstline() when fileinput._state is None.
           Ensure that it raises RuntimeError with a meaningful error message
           and does not modify fileinput._state"""
        fileinput._state = None
        with self.assertRaises(RuntimeError) as cm:
            fileinput.isfirstline()
        self.assertEqual(("no active input()",), cm.exception.args)
        self.assertIsNone(fileinput._state)

    def test_state_is_not_None(self):
        """Tests fileinput.isfirstline() when fileinput._state is not None.
           Ensure that it invokes fileinput._state.isfirstline() exactly once,
           returns whatever it returns, and does not modify fileinput._state
           to point to a different object."""
        isfirstline_retval = object()
        instance = MockFileInput()
        instance.return_values["isfirstline"] = isfirstline_retval
        fileinput._state = instance
        retval = fileinput.isfirstline()
        self.assertExactlyOneInvocation(instance, "isfirstline")
        self.assertIs(retval, isfirstline_retval)
        self.assertIs(fileinput._state, instance)

class Test_fileinput_isstdin(BaseFileInputGlobalMethodsTest):
    """Unit tests for fileinput.isstdin()"""

    def test_state_is_None(self):
        """Tests fileinput.isstdin() when fileinput._state is None.
           Ensure that it raises RuntimeError with a meaningful error message
           and does not modify fileinput._state"""
        fileinput._state = None
        with self.assertRaises(RuntimeError) as cm:
            fileinput.isstdin()
        self.assertEqual(("no active input()",), cm.exception.args)
        self.assertIsNone(fileinput._state)

    def test_state_is_not_None(self):
        """Tests fileinput.isstdin() when fileinput._state is not None.
           Ensure that it invokes fileinput._state.isstdin() exactly once,
           returns whatever it returns, and does not modify fileinput._state
           to point to a different object."""
        isstdin_retval = object()
        instance = MockFileInput()
        instance.return_values["isstdin"] = isstdin_retval
        fileinput._state = instance
        retval = fileinput.isstdin()
        self.assertExactlyOneInvocation(instance, "isstdin")
        self.assertIs(retval, isstdin_retval)
        self.assertIs(fileinput._state, instance)

class InvocationRecorder:

    def __init__(self):
        self.invocation_count = 0

    def __call__(self, *args, **kwargs):
        self.invocation_count += 1
        self.last_invocation = (args, kwargs)
        return io.BytesIO(b'some bytes')


class Test_hook_compressed(unittest.TestCase):
    """Unit tests for fileinput.hook_compressed()"""

    def setUp(self):
        self.fake_open = InvocationRecorder()

    def test_empty_string(self):
        self.do_test_use_builtin_open_text("", "r")

    def test_no_ext(self):
        self.do_test_use_builtin_open_text("abcd", "r")

    @unittest.skipUnless(gzip, "Requires gzip and zlib")
    def test_gz_ext_fake(self):
        original_open = gzip.open
        gzip.open = self.fake_open
        try:
            result = fileinput.hook_compressed("test.gz", "r")
        finally:
            gzip.open = original_open

        self.assertEqual(self.fake_open.invocation_count, 1)
        self.assertEqual(self.fake_open.last_invocation, (("test.gz", "r"), {}))

    @unittest.skipUnless(gzip, "Requires gzip and zlib")
    def test_gz_with_encoding_fake(self):
        original_open = gzip.open
        gzip.open = lambda filename, mode: io.BytesIO(b'Ex-binary string')
        try:
            result = fileinput.hook_compressed("test.gz", "r", encoding="utf-8")
        finally:
            gzip.open = original_open
        self.assertEqual(list(result), ['Ex-binary string'])

    @unittest.skipUnless(bz2, "Requires bz2")
    def test_bz2_ext_fake(self):
        original_open = bz2.BZ2File
        bz2.BZ2File = self.fake_open
        try:
            result = fileinput.hook_compressed("test.bz2", "r")
        finally:
            bz2.BZ2File = original_open

        self.assertEqual(self.fake_open.invocation_count, 1)
        self.assertEqual(self.fake_open.last_invocation, (("test.bz2", "r"), {}))

    def test_blah_ext(self):
        self.do_test_use_builtin_open_binary("abcd.blah", "rb")

    def test_gz_ext_builtin(self):
        self.do_test_use_builtin_open_binary("abcd.Gz", "rb")

    def test_bz2_ext_builtin(self):
        self.do_test_use_builtin_open_binary("abcd.Bz2", "rb")

    def test_binary_mode_encoding(self):
        self.do_test_use_builtin_open_binary("abcd", "rb")

    def test_text_mode_encoding(self):
        self.do_test_use_builtin_open_text("abcd", "r")

    def do_test_use_builtin_open_binary(self, filename, mode):
        original_open = self.replace_builtin_open(self.fake_open)
        try:
            result = fileinput.hook_compressed(filename, mode)
        finally:
            self.replace_builtin_open(original_open)

        self.assertEqual(self.fake_open.invocation_count, 1)
        self.assertEqual(self.fake_open.last_invocation,
                         ((filename, mode), {'encoding': None, 'errors': None}))

    def do_test_use_builtin_open_text(self, filename, mode):
        original_open = self.replace_builtin_open(self.fake_open)
        try:
            result = fileinput.hook_compressed(filename, mode)
        finally:
            self.replace_builtin_open(original_open)

        self.assertEqual(self.fake_open.invocation_count, 1)
        self.assertEqual(self.fake_open.last_invocation,
                         ((filename, mode), {'encoding': 'locale', 'errors': None}))

    @staticmethod
    def replace_builtin_open(new_open_func):
        original_open = builtins.open
        builtins.open = new_open_func
        return original_open

class Test_hook_encoded(unittest.TestCase):
    """Unit tests for fileinput.hook_encoded()"""

    def test(self):
        encoding = object()
        errors = object()
        result = fileinput.hook_encoded(encoding, errors=errors)

        fake_open = InvocationRecorder()
        original_open = builtins.open
        builtins.open = fake_open
        try:
            filename = object()
            mode = object()
            open_result = result(filename, mode)
        finally:
            builtins.open = original_open

        self.assertEqual(fake_open.invocation_count, 1)

        args, kwargs = fake_open.last_invocation
        self.assertIs(args[0], filename)
        self.assertIs(args[1], mode)
        self.assertIs(kwargs.pop('encoding'), encoding)
        self.assertIs(kwargs.pop('errors'), errors)
        self.assertFalse(kwargs)

    def test_errors(self):
        with open(TESTFN, 'wb') as f:
            f.write(b'\x80abc')
        self.addCleanup(safe_unlink, TESTFN)

        def check(errors, expected_lines):
            with FileInput(files=TESTFN, mode='r',
                           openhook=hook_encoded('utf-8', errors=errors)) as fi:
                lines = list(fi)
            self.assertEqual(lines, expected_lines)

        check('ignore', ['abc'])
        with self.assertRaises(UnicodeDecodeError):
            check('strict', ['abc'])
        check('replace', ['\ufffdabc'])
        check('backslashreplace', ['\\x80abc'])

    def test_modes(self):
        with open(TESTFN, 'wb') as f:
            # UTF-7 is a convenient, seldom used encoding
            f.write(b'A\nB\r\nC\rD+IKw-')
        self.addCleanup(safe_unlink, TESTFN)

        def check(mode, expected_lines):
            with FileInput(files=TESTFN, mode=mode,
                           openhook=hook_encoded('utf-7')) as fi:
                lines = list(fi)
            self.assertEqual(lines, expected_lines)

        check('r', ['A\n', 'B\n', 'C\n', 'D\u20ac'])
        with self.assertRaises(ValueError):
            check('rb', ['A\n', 'B\r\n', 'C\r', 'D\u20ac'])


class MiscTest(unittest.TestCase):

    def test_all(self):
        support.check__all__(self, fileinput)


if __name__ == "__main__":
    unittest.main()
