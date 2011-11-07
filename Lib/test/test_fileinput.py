'''
Tests for fileinput module.
Nick Mathewson
'''
import os
import sys
import re
import fileinput
import collections
import builtins
import unittest

try:
    import bz2
except ImportError:
    bz2 = None
try:
    import gzip
except ImportError:
    gzip = None

from io import StringIO
from fileinput import FileInput, hook_encoded

from test.support import verbose, TESTFN, run_unittest
from test.support import unlink as safe_unlink


# The fileinput module has 2 interfaces: the FileInput class which does
# all the work, and a few functions (input, etc.) that use a global _state
# variable.

# Write lines (a list of lines) to temp file number i, and return the
# temp file's name.
def writeTmp(i, lines, mode='w'):  # opening in text mode is the default
    name = TESTFN + str(i)
    f = open(name, mode)
    for line in lines:
        f.write(line)
    f.close()
    return name

def remove_tempfiles(*names):
    for name in names:
        if name:
            safe_unlink(name)

class BufferSizesTests(unittest.TestCase):
    def test_buffer_sizes(self):
        # First, run the tests with default and teeny buffer size.
        for round, bs in (0, 0), (1, 30):
            t1 = t2 = t3 = t4 = None
            try:
                t1 = writeTmp(1, ["Line %s of file 1\n" % (i+1) for i in range(15)])
                t2 = writeTmp(2, ["Line %s of file 2\n" % (i+1) for i in range(10)])
                t3 = writeTmp(3, ["Line %s of file 3\n" % (i+1) for i in range(5)])
                t4 = writeTmp(4, ["Line %s of file 4\n" % (i+1) for i in range(1)])
                self.buffer_size_test(t1, t2, t3, t4, bs, round)
            finally:
                remove_tempfiles(t1, t2, t3, t4)

    def buffer_size_test(self, t1, t2, t3, t4, bs=0, round=0):
        pat = re.compile(r'LINE (\d+) OF FILE (\d+)')

        start = 1 + round*6
        if verbose:
            print('%s. Simple iteration (bs=%s)' % (start+0, bs))
        fi = FileInput(files=(t1, t2, t3, t4), bufsize=bs)
        lines = list(fi)
        fi.close()
        self.assertEqual(len(lines), 31)
        self.assertEqual(lines[4], 'Line 5 of file 1\n')
        self.assertEqual(lines[30], 'Line 1 of file 4\n')
        self.assertEqual(fi.lineno(), 31)
        self.assertEqual(fi.filename(), t4)

        if verbose:
            print('%s. Status variables (bs=%s)' % (start+1, bs))
        fi = FileInput(files=(t1, t2, t3, t4), bufsize=bs)
        s = "x"
        while s and s != 'Line 6 of file 2\n':
            s = fi.readline()
        self.assertEqual(fi.filename(), t2)
        self.assertEqual(fi.lineno(), 21)
        self.assertEqual(fi.filelineno(), 6)
        self.assertFalse(fi.isfirstline())
        self.assertFalse(fi.isstdin())

        if verbose:
            print('%s. Nextfile (bs=%s)' % (start+2, bs))
        fi.nextfile()
        self.assertEqual(fi.readline(), 'Line 1 of file 3\n')
        self.assertEqual(fi.lineno(), 22)
        fi.close()

        if verbose:
            print('%s. Stdin (bs=%s)' % (start+3, bs))
        fi = FileInput(files=(t1, t2, t3, t4, '-'), bufsize=bs)
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
            print('%s. Boundary conditions (bs=%s)' % (start+4, bs))
        fi = FileInput(files=(t1, t2, t3, t4), bufsize=bs)
        self.assertEqual(fi.lineno(), 0)
        self.assertEqual(fi.filename(), None)
        fi.nextfile()
        self.assertEqual(fi.lineno(), 0)
        self.assertEqual(fi.filename(), None)

        if verbose:
            print('%s. Inplace (bs=%s)' % (start+5, bs))
        savestdout = sys.stdout
        try:
            fi = FileInput(files=(t1, t2, t3, t4), inplace=1, bufsize=bs)
            for line in fi:
                line = line[:-1].upper()
                print(line)
            fi.close()
        finally:
            sys.stdout = savestdout

        fi = FileInput(files=(t1, t2, t3, t4), bufsize=bs)
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

class FileInputTests(unittest.TestCase):

    def test_zero_byte_files(self):
        t1 = t2 = t3 = t4 = None
        try:
            t1 = writeTmp(1, [""])
            t2 = writeTmp(2, [""])
            t3 = writeTmp(3, ["The only line there is.\n"])
            t4 = writeTmp(4, [""])
            fi = FileInput(files=(t1, t2, t3, t4))

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
        finally:
            remove_tempfiles(t1, t2, t3, t4)

    def test_files_that_dont_end_with_newline(self):
        t1 = t2 = None
        try:
            t1 = writeTmp(1, ["A\nB\nC"])
            t2 = writeTmp(2, ["D\nE\nF"])
            fi = FileInput(files=(t1, t2))
            lines = list(fi)
            self.assertEqual(lines, ["A\n", "B\n", "C", "D\n", "E\n", "F"])
            self.assertEqual(fi.filelineno(), 3)
            self.assertEqual(fi.lineno(), 6)
        finally:
            remove_tempfiles(t1, t2)

##     def test_unicode_filenames(self):
##         # XXX A unicode string is always returned by writeTmp.
##         #     So is this needed?
##         try:
##             t1 = writeTmp(1, ["A\nB"])
##             encoding = sys.getfilesystemencoding()
##             if encoding is None:
##                 encoding = 'ascii'
##             fi = FileInput(files=str(t1, encoding))
##             lines = list(fi)
##             self.assertEqual(lines, ["A\n", "B"])
##         finally:
##             remove_tempfiles(t1)

    def test_fileno(self):
        t1 = t2 = None
        try:
            t1 = writeTmp(1, ["A\nB"])
            t2 = writeTmp(2, ["C\nD"])
            fi = FileInput(files=(t1, t2))
            self.assertEqual(fi.fileno(), -1)
            line =next( fi)
            self.assertNotEqual(fi.fileno(), -1)
            fi.nextfile()
            self.assertEqual(fi.fileno(), -1)
            line = list(fi)
            self.assertEqual(fi.fileno(), -1)
        finally:
            remove_tempfiles(t1, t2)

    def test_opening_mode(self):
        try:
            # invalid mode, should raise ValueError
            fi = FileInput(mode="w")
            self.fail("FileInput should reject invalid mode argument")
        except ValueError:
            pass
        t1 = None
        try:
            # try opening in universal newline mode
            t1 = writeTmp(1, [b"A\nB\r\nC\rD"], mode="wb")
            fi = FileInput(files=t1, mode="U")
            lines = list(fi)
            self.assertEqual(lines, ["A\n", "B\n", "C\n", "D"])
        finally:
            remove_tempfiles(t1)

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
            def __call__(self, *args):
                self.invoked = True
                return open(*args)

        t = writeTmp(1, ["\n"])
        self.addCleanup(remove_tempfiles, t)
        custom_open_hook = CustomOpenHook()
        with FileInput([t], openhook=custom_open_hook) as fi:
            fi.readline()
        self.assertTrue(custom_open_hook.invoked, "openhook not invoked")

    def test_context_manager(self):
        try:
            t1 = writeTmp(1, ["A\nB\nC"])
            t2 = writeTmp(2, ["D\nE\nF"])
            with FileInput(files=(t1, t2)) as fi:
                lines = list(fi)
            self.assertEqual(lines, ["A\n", "B\n", "C", "D\n", "E\n", "F"])
            self.assertEqual(fi.filelineno(), 3)
            self.assertEqual(fi.lineno(), 6)
            self.assertEqual(fi._files, ())
        finally:
            remove_tempfiles(t1, t2)

    def test_close_on_exception(self):
        try:
            t1 = writeTmp(1, [""])
            with FileInput(files=t1) as fi:
                raise IOError
        except IOError:
            self.assertEqual(fi._files, ())
        finally:
            remove_tempfiles(t1)

    def test_empty_files_list_specified_to_constructor(self):
        with FileInput(files=[]) as fi:
            self.assertEqual(fi._files, ('-',))

    def test__getitem__(self):
        """Tests invoking FileInput.__getitem__() with the current
           line number"""
        t = writeTmp(1, ["line1\n", "line2\n"])
        self.addCleanup(remove_tempfiles, t)
        with FileInput(files=[t]) as fi:
            retval1 = fi[0]
            self.assertEqual(retval1, "line1\n")
            retval2 = fi[1]
            self.assertEqual(retval2, "line2\n")

    def test__getitem__invalid_key(self):
        """Tests invoking FileInput.__getitem__() with an index unequal to
           the line number"""
        t = writeTmp(1, ["line1\n", "line2\n"])
        self.addCleanup(remove_tempfiles, t)
        with FileInput(files=[t]) as fi:
            with self.assertRaises(RuntimeError) as cm:
                fi[1]
        self.assertEqual(cm.exception.args, ("accessing lines out of order",))

    def test__getitem__eof(self):
        """Tests invoking FileInput.__getitem__() with the line number but at
           end-of-input"""
        t = writeTmp(1, [])
        self.addCleanup(remove_tempfiles, t)
        with FileInput(files=[t]) as fi:
            with self.assertRaises(IndexError) as cm:
                fi[0]
        self.assertEqual(cm.exception.args, ("end of input reached",))

    def test_nextfile_oserror_deleting_backup(self):
        """Tests invoking FileInput.nextfile() when the attempt to delete
           the backup file would raise OSError.  This error is expected to be
           silently ignored"""

        os_unlink_orig = os.unlink
        os_unlink_replacement = UnconditionallyRaise(OSError)
        try:
            t = writeTmp(1, ["\n"])
            self.addCleanup(remove_tempfiles, t)
            with FileInput(files=[t], inplace=True) as fi:
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
            t = writeTmp(1, ["\n"])
            self.addCleanup(remove_tempfiles, t)
            with FileInput(files=[t], inplace=True) as fi:
                os.fstat = os_fstat_replacement
                fi.readline()
        finally:
            os.fstat = os_fstat_orig

        # sanity check to make sure that our test scenario was actually hit
        self.assertTrue(os_fstat_replacement.invoked,
                        "os.fstat() was not invoked")

    @unittest.skipIf(not hasattr(os, "chmod"), "os.chmod does not exist")
    def test_readline_os_chmod_raises_OSError(self):
        """Tests invoking FileInput.readline() when os.chmod() raises OSError.
           This exception should be silently discarded."""

        os_chmod_orig = os.chmod
        os_chmod_replacement = UnconditionallyRaise(OSError)
        try:
            t = writeTmp(1, ["\n"])
            self.addCleanup(remove_tempfiles, t)
            with FileInput(files=[t], inplace=True) as fi:
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
        t = writeTmp(1, ["\n"])
        self.addCleanup(remove_tempfiles, t)
        with FileInput(files=[t]) as fi:
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

class MockFileInput:
    """A class that mocks out fileinput.FileInput for use during unit tests"""

    def __init__(self, files=None, inplace=False, backup="", bufsize=0,
                 mode="r", openhook=None):
        self.files = files
        self.inplace = inplace
        self.backup = backup
        self.bufsize = bufsize
        self.mode = mode
        self.openhook = openhook
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
        bufsize = object()
        mode = object()
        openhook = object()

        # call fileinput.input() with different values for each argument
        result = fileinput.input(files=files, inplace=inplace, backup=backup,
                                 bufsize=bufsize,
            mode=mode, openhook=openhook)

        # ensure fileinput._state was set to the returned object
        self.assertIs(result, fileinput._state, "fileinput._state")

        # ensure the parameters to fileinput.input() were passed directly
        # to FileInput.__init__()
        self.assertIs(files, result.files, "files")
        self.assertIs(inplace, result.inplace, "inplace")
        self.assertIs(backup, result.backup, "backup")
        self.assertIs(bufsize, result.bufsize, "bufsize")
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

class Test_hook_compressed(unittest.TestCase):
    """Unit tests for fileinput.hook_compressed()"""

    def setUp(self):
        self.fake_open = InvocationRecorder()

    def test_empty_string(self):
        self.do_test_use_builtin_open("", 1)

    def test_no_ext(self):
        self.do_test_use_builtin_open("abcd", 2)

    @unittest.skipUnless(gzip, "Requires gzip and zlib")
    def test_gz_ext_fake(self):
        original_open = gzip.open
        gzip.open = self.fake_open
        try:
            result = fileinput.hook_compressed("test.gz", 3)
        finally:
            gzip.open = original_open

        self.assertEqual(self.fake_open.invocation_count, 1)
        self.assertEqual(self.fake_open.last_invocation, (("test.gz", 3), {}))

    @unittest.skipUnless(bz2, "Requires bz2")
    def test_bz2_ext_fake(self):
        original_open = bz2.BZ2File
        bz2.BZ2File = self.fake_open
        try:
            result = fileinput.hook_compressed("test.bz2", 4)
        finally:
            bz2.BZ2File = original_open

        self.assertEqual(self.fake_open.invocation_count, 1)
        self.assertEqual(self.fake_open.last_invocation, (("test.bz2", 4), {}))

    def test_blah_ext(self):
        self.do_test_use_builtin_open("abcd.blah", 5)

    def test_gz_ext_builtin(self):
        self.do_test_use_builtin_open("abcd.Gz", 6)

    def test_bz2_ext_builtin(self):
        self.do_test_use_builtin_open("abcd.Bz2", 7)

    def do_test_use_builtin_open(self, filename, mode):
        original_open = self.replace_builtin_open(self.fake_open)
        try:
            result = fileinput.hook_compressed(filename, mode)
        finally:
            self.replace_builtin_open(original_open)

        self.assertEqual(self.fake_open.invocation_count, 1)
        self.assertEqual(self.fake_open.last_invocation,
                         ((filename, mode), {}))

    @staticmethod
    def replace_builtin_open(new_open_func):
        original_open = builtins.open
        builtins.open = new_open_func
        return original_open

class Test_hook_encoded(unittest.TestCase):
    """Unit tests for fileinput.hook_encoded()"""

    def test(self):
        encoding = object()
        result = fileinput.hook_encoded(encoding)

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
        self.assertFalse(kwargs)

def test_main():
    run_unittest(
        BufferSizesTests,
        FileInputTests,
        Test_fileinput_input,
        Test_fileinput_close,
        Test_fileinput_nextfile,
        Test_fileinput_filename,
        Test_fileinput_lineno,
        Test_fileinput_filelineno,
        Test_fileinput_fileno,
        Test_fileinput_isfirstline,
        Test_fileinput_isstdin,
        Test_hook_compressed,
        Test_hook_encoded,
    )

if __name__ == "__main__":
    test_main()
