'''
Tests for fileinput module.
Nick Mathewson
'''

import unittest
from test.test_support import verbose, TESTFN, run_unittest
from test.test_support import unlink as safe_unlink, check_warnings
import sys, re
from StringIO import StringIO
from fileinput import FileInput, hook_encoded

# The fileinput module has 2 interfaces: the FileInput class which does
# all the work, and a few functions (input, etc.) that use a global _state
# variable.  We only test the FileInput class, since the other functions
# only provide a thin facade over FileInput.

# Write lines (a list of lines) to temp file number i, and return the
# temp file's name.
def writeTmp(i, lines, mode='w'):  # opening in text mode is the default
    name = TESTFN + str(i)
    f = open(name, mode)
    f.writelines(lines)
    f.close()
    return name

def remove_tempfiles(*names):
    for name in names:
        safe_unlink(name)

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

class BufferSizesTests(unittest.TestCase):
    def test_buffer_sizes(self):
        # First, run the tests with default and teeny buffer size.
        for round, bs in (0, 0), (1, 30):
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
            print '%s. Simple iteration (bs=%s)' % (start+0, bs)
        fi = FileInput(files=(t1, t2, t3, t4), bufsize=bs)
        lines = list(fi)
        fi.close()
        self.assertEqual(len(lines), 31)
        self.assertEqual(lines[4], 'Line 5 of file 1\n')
        self.assertEqual(lines[30], 'Line 1 of file 4\n')
        self.assertEqual(fi.lineno(), 31)
        self.assertEqual(fi.filename(), t4)

        if verbose:
            print '%s. Status variables (bs=%s)' % (start+1, bs)
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
            print '%s. Nextfile (bs=%s)' % (start+2, bs)
        fi.nextfile()
        self.assertEqual(fi.readline(), 'Line 1 of file 3\n')
        self.assertEqual(fi.lineno(), 22)
        fi.close()

        if verbose:
            print '%s. Stdin (bs=%s)' % (start+3, bs)
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
            print '%s. Boundary conditions (bs=%s)' % (start+4, bs)
        fi = FileInput(files=(t1, t2, t3, t4), bufsize=bs)
        self.assertEqual(fi.lineno(), 0)
        self.assertEqual(fi.filename(), None)
        fi.nextfile()
        self.assertEqual(fi.lineno(), 0)
        self.assertEqual(fi.filename(), None)

        if verbose:
            print '%s. Inplace (bs=%s)' % (start+5, bs)
        savestdout = sys.stdout
        try:
            fi = FileInput(files=(t1, t2, t3, t4), inplace=1, bufsize=bs)
            for line in fi:
                line = line[:-1].upper()
                print line
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

class FileInputTests(unittest.TestCase):
    def test_zero_byte_files(self):
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

    def test_unicode_filenames(self):
        try:
            t1 = writeTmp(1, ["A\nB"])
            encoding = sys.getfilesystemencoding()
            if encoding is None:
                encoding = 'ascii'
            fi = FileInput(files=unicode(t1, encoding))
            lines = list(fi)
            self.assertEqual(lines, ["A\n", "B"])
        finally:
            remove_tempfiles(t1)

    def test_fileno(self):
        try:
            t1 = writeTmp(1, ["A\nB"])
            t2 = writeTmp(2, ["C\nD"])
            fi = FileInput(files=(t1, t2))
            self.assertEqual(fi.fileno(), -1)
            line = fi.next()
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
        try:
            # try opening in universal newline mode
            t1 = writeTmp(1, ["A\nB\r\nC\rD"], mode="wb")
            fi = FileInput(files=t1, mode="U")
            lines = list(fi)
            self.assertEqual(lines, ["A\n", "B\n", "C\n", "D"])
        finally:
            remove_tempfiles(t1)

    def test_file_opening_hook(self):
        try:
            # cannot use openhook and inplace mode
            fi = FileInput(inplace=1, openhook=lambda f,m: None)
            self.fail("FileInput should raise if both inplace "
                             "and openhook arguments are given")
        except ValueError:
            pass
        try:
            fi = FileInput(openhook=1)
            self.fail("FileInput should check openhook for being callable")
        except ValueError:
            pass
        try:
            # UTF-7 is a convenient, seldom used encoding
            t1 = writeTmp(1, ['+AEE-\n+AEI-'], mode="wb")
            fi = FileInput(files=t1, openhook=hook_encoded("utf-7"))
            lines = list(fi)
            self.assertEqual(lines, [u'A\n', u'B'])
        finally:
            remove_tempfiles(t1)

    def test_readline(self):
        with open(TESTFN, 'wb') as f:
            f.write('A\nB\r\nC\r')
            # Fill TextIOWrapper buffer.
            f.write('123456789\n' * 1000)
            # Issue #20501: readline() shouldn't read whole file.
            f.write('\x80')
        self.addCleanup(safe_unlink, TESTFN)

        fi = FileInput(files=TESTFN, openhook=hook_encoded('ascii'))
        # The most likely failure is a UnicodeDecodeError due to the entire
        # file being read when it shouldn't have been.
        self.assertEqual(fi.readline(), u'A\n')
        self.assertEqual(fi.readline(), u'B\r\n')
        self.assertEqual(fi.readline(), u'C\r')
        with self.assertRaises(UnicodeDecodeError):
            # Read to the end of file.
            list(fi)
        fi.close()

    def test_readline_buffering(self):
        src = LineReader()
        fi = FileInput(files=['line1\nline2', 'line3\n'], openhook=src.openhook)
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
        fi.close()

    def test_iteration_buffering(self):
        src = LineReader()
        fi = FileInput(files=['line1\nline2', 'line3\n'], openhook=src.openhook)
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
        fi.close()

class Test_hook_encoded(unittest.TestCase):
    """Unit tests for fileinput.hook_encoded()"""

    def test_modes(self):
        with open(TESTFN, 'wb') as f:
            # UTF-7 is a convenient, seldom used encoding
            f.write('A\nB\r\nC\rD+IKw-')
        self.addCleanup(safe_unlink, TESTFN)

        def check(mode, expected_lines):
            fi = FileInput(files=TESTFN, mode=mode,
                           openhook=hook_encoded('utf-7'))
            lines = list(fi)
            fi.close()
            self.assertEqual(lines, expected_lines)

        check('r', [u'A\n', u'B\r\n', u'C\r', u'D\u20ac'])
        check('rU', [u'A\n', u'B\r\n', u'C\r', u'D\u20ac'])
        check('U', [u'A\n', u'B\r\n', u'C\r', u'D\u20ac'])
        check('rb', [u'A\n', u'B\r\n', u'C\r', u'D\u20ac'])

def test_main():
    run_unittest(BufferSizesTests, FileInputTests, Test_hook_encoded)

if __name__ == "__main__":
    test_main()
