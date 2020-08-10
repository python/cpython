"Test run, coverage 49%."

from idlelib import run
import unittest
from unittest import mock
from idlelib.idle_test.mock_idle import Func
from test.support import captured_output, captured_stderr

import io
import sys


class RunTest(unittest.TestCase):

    def test_print_exception_unhashable(self):
        class UnhashableException(Exception):
            def __eq__(self, other):
                return True

        ex1 = UnhashableException('ex1')
        ex2 = UnhashableException('ex2')
        try:
            raise ex2 from ex1
        except UnhashableException:
            try:
                raise ex1
            except UnhashableException:
                with captured_stderr() as output:
                    with mock.patch.object(run,
                                           'cleanup_traceback') as ct:
                        ct.side_effect = lambda t, e: t
                        run.print_exception()

        tb = output.getvalue().strip().splitlines()
        self.assertEqual(11, len(tb))
        self.assertIn('UnhashableException: ex2', tb[3])
        self.assertIn('UnhashableException: ex1', tb[10])


# StdioFile tests.

class S(str):
    def __str__(self):
        return '%s:str' % type(self).__name__
    def __unicode__(self):
        return '%s:unicode' % type(self).__name__
    def __len__(self):
        return 3
    def __iter__(self):
        return iter('abc')
    def __getitem__(self, *args):
        return '%s:item' % type(self).__name__
    def __getslice__(self, *args):
        return '%s:slice' % type(self).__name__


class MockShell:
    def __init__(self):
        self.reset()
    def write(self, *args):
        self.written.append(args)
    def readline(self):
        return self.lines.pop()
    def close(self):
        pass
    def reset(self):
        self.written = []
    def push(self, lines):
        self.lines = list(lines)[::-1]


class StdInputFilesTest(unittest.TestCase):

    def test_misc(self):
        shell = MockShell()
        f = run.StdInputFile(shell, 'stdin')
        self.assertIsInstance(f, io.TextIOBase)
        self.assertEqual(f.encoding, 'utf-8')
        self.assertEqual(f.errors, 'strict')
        self.assertIsNone(f.newlines)
        self.assertEqual(f.name, '<stdin>')
        self.assertFalse(f.closed)
        self.assertTrue(f.isatty())
        self.assertTrue(f.readable())
        self.assertFalse(f.writable())
        self.assertFalse(f.seekable())

    def test_unsupported(self):
        shell = MockShell()
        f = run.StdInputFile(shell, 'stdin')
        self.assertRaises(OSError, f.fileno)
        self.assertRaises(OSError, f.tell)
        self.assertRaises(OSError, f.seek, 0)
        self.assertRaises(OSError, f.write, 'x')
        self.assertRaises(OSError, f.writelines, ['x'])

    def test_read(self):
        shell = MockShell()
        f = run.StdInputFile(shell, 'stdin')
        shell.push(['one\n', 'two\n', ''])
        self.assertEqual(f.read(), 'one\ntwo\n')
        shell.push(['one\n', 'two\n', ''])
        self.assertEqual(f.read(-1), 'one\ntwo\n')
        shell.push(['one\n', 'two\n', ''])
        self.assertEqual(f.read(None), 'one\ntwo\n')
        shell.push(['one\n', 'two\n', 'three\n', ''])
        self.assertEqual(f.read(2), 'on')
        self.assertEqual(f.read(3), 'e\nt')
        self.assertEqual(f.read(10), 'wo\nthree\n')

        shell.push(['one\n', 'two\n'])
        self.assertEqual(f.read(0), '')
        self.assertRaises(TypeError, f.read, 1.5)
        self.assertRaises(TypeError, f.read, '1')
        self.assertRaises(TypeError, f.read, 1, 1)

    def test_readline(self):
        shell = MockShell()
        f = run.StdInputFile(shell, 'stdin')
        shell.push(['one\n', 'two\n', 'three\n', 'four\n'])
        self.assertEqual(f.readline(), 'one\n')
        self.assertEqual(f.readline(-1), 'two\n')
        self.assertEqual(f.readline(None), 'three\n')
        shell.push(['one\ntwo\n'])
        self.assertEqual(f.readline(), 'one\n')
        self.assertEqual(f.readline(), 'two\n')
        shell.push(['one', 'two', 'three'])
        self.assertEqual(f.readline(), 'one')
        self.assertEqual(f.readline(), 'two')
        shell.push(['one\n', 'two\n', 'three\n'])
        self.assertEqual(f.readline(2), 'on')
        self.assertEqual(f.readline(1), 'e')
        self.assertEqual(f.readline(1), '\n')
        self.assertEqual(f.readline(10), 'two\n')

        shell.push(['one\n', 'two\n'])
        self.assertEqual(f.readline(0), '')
        self.assertRaises(TypeError, f.readlines, 1.5)
        self.assertRaises(TypeError, f.readlines, '1')
        self.assertRaises(TypeError, f.readlines, 1, 1)

    def test_readlines(self):
        shell = MockShell()
        f = run.StdInputFile(shell, 'stdin')
        shell.push(['one\n', 'two\n', ''])
        self.assertEqual(f.readlines(), ['one\n', 'two\n'])
        shell.push(['one\n', 'two\n', ''])
        self.assertEqual(f.readlines(-1), ['one\n', 'two\n'])
        shell.push(['one\n', 'two\n', ''])
        self.assertEqual(f.readlines(None), ['one\n', 'two\n'])
        shell.push(['one\n', 'two\n', ''])
        self.assertEqual(f.readlines(0), ['one\n', 'two\n'])
        shell.push(['one\n', 'two\n', ''])
        self.assertEqual(f.readlines(3), ['one\n'])
        shell.push(['one\n', 'two\n', ''])
        self.assertEqual(f.readlines(4), ['one\n', 'two\n'])

        shell.push(['one\n', 'two\n', ''])
        self.assertRaises(TypeError, f.readlines, 1.5)
        self.assertRaises(TypeError, f.readlines, '1')
        self.assertRaises(TypeError, f.readlines, 1, 1)

    def test_close(self):
        shell = MockShell()
        f = run.StdInputFile(shell, 'stdin')
        shell.push(['one\n', 'two\n', ''])
        self.assertFalse(f.closed)
        self.assertEqual(f.readline(), 'one\n')
        f.close()
        self.assertFalse(f.closed)
        self.assertEqual(f.readline(), 'two\n')
        self.assertRaises(TypeError, f.close, 1)


class StdOutputFilesTest(unittest.TestCase):

    def test_misc(self):
        shell = MockShell()
        f = run.StdOutputFile(shell, 'stdout')
        self.assertIsInstance(f, io.TextIOBase)
        self.assertEqual(f.encoding, 'utf-8')
        self.assertEqual(f.errors, 'strict')
        self.assertIsNone(f.newlines)
        self.assertEqual(f.name, '<stdout>')
        self.assertFalse(f.closed)
        self.assertTrue(f.isatty())
        self.assertFalse(f.readable())
        self.assertTrue(f.writable())
        self.assertFalse(f.seekable())

    def test_unsupported(self):
        shell = MockShell()
        f = run.StdOutputFile(shell, 'stdout')
        self.assertRaises(OSError, f.fileno)
        self.assertRaises(OSError, f.tell)
        self.assertRaises(OSError, f.seek, 0)
        self.assertRaises(OSError, f.read, 0)
        self.assertRaises(OSError, f.readline, 0)

    def test_write(self):
        shell = MockShell()
        f = run.StdOutputFile(shell, 'stdout')
        f.write('test')
        self.assertEqual(shell.written, [('test', 'stdout')])
        shell.reset()
        f.write('t\xe8\u015b\U0001d599')
        self.assertEqual(shell.written, [('t\xe8\u015b\U0001d599', 'stdout')])
        shell.reset()

        f.write(S('t\xe8\u015b\U0001d599'))
        self.assertEqual(shell.written, [('t\xe8\u015b\U0001d599', 'stdout')])
        self.assertEqual(type(shell.written[0][0]), str)
        shell.reset()

        self.assertRaises(TypeError, f.write)
        self.assertEqual(shell.written, [])
        self.assertRaises(TypeError, f.write, b'test')
        self.assertRaises(TypeError, f.write, 123)
        self.assertEqual(shell.written, [])
        self.assertRaises(TypeError, f.write, 'test', 'spam')
        self.assertEqual(shell.written, [])

    def test_write_stderr_nonencodable(self):
        shell = MockShell()
        f = run.StdOutputFile(shell, 'stderr', 'iso-8859-15', 'backslashreplace')
        f.write('t\xe8\u015b\U0001d599\xa4')
        self.assertEqual(shell.written, [('t\xe8\\u015b\\U0001d599\\xa4', 'stderr')])
        shell.reset()

        f.write(S('t\xe8\u015b\U0001d599\xa4'))
        self.assertEqual(shell.written, [('t\xe8\\u015b\\U0001d599\\xa4', 'stderr')])
        self.assertEqual(type(shell.written[0][0]), str)
        shell.reset()

        self.assertRaises(TypeError, f.write)
        self.assertEqual(shell.written, [])
        self.assertRaises(TypeError, f.write, b'test')
        self.assertRaises(TypeError, f.write, 123)
        self.assertEqual(shell.written, [])
        self.assertRaises(TypeError, f.write, 'test', 'spam')
        self.assertEqual(shell.written, [])

    def test_writelines(self):
        shell = MockShell()
        f = run.StdOutputFile(shell, 'stdout')
        f.writelines([])
        self.assertEqual(shell.written, [])
        shell.reset()
        f.writelines(['one\n', 'two'])
        self.assertEqual(shell.written,
                         [('one\n', 'stdout'), ('two', 'stdout')])
        shell.reset()
        f.writelines(['on\xe8\n', 'tw\xf2'])
        self.assertEqual(shell.written,
                         [('on\xe8\n', 'stdout'), ('tw\xf2', 'stdout')])
        shell.reset()

        f.writelines([S('t\xe8st')])
        self.assertEqual(shell.written, [('t\xe8st', 'stdout')])
        self.assertEqual(type(shell.written[0][0]), str)
        shell.reset()

        self.assertRaises(TypeError, f.writelines)
        self.assertEqual(shell.written, [])
        self.assertRaises(TypeError, f.writelines, 123)
        self.assertEqual(shell.written, [])
        self.assertRaises(TypeError, f.writelines, [b'test'])
        self.assertRaises(TypeError, f.writelines, [123])
        self.assertEqual(shell.written, [])
        self.assertRaises(TypeError, f.writelines, [], [])
        self.assertEqual(shell.written, [])

    def test_close(self):
        shell = MockShell()
        f = run.StdOutputFile(shell, 'stdout')
        self.assertFalse(f.closed)
        f.write('test')
        f.close()
        self.assertTrue(f.closed)
        self.assertRaises(ValueError, f.write, 'x')
        self.assertEqual(shell.written, [('test', 'stdout')])
        f.close()
        self.assertRaises(TypeError, f.close, 1)


class RecursionLimitTest(unittest.TestCase):
    # Test (un)install_recursionlimit_wrappers and fixdoc.

    def test_bad_setrecursionlimit_calls(self):
        run.install_recursionlimit_wrappers()
        self.addCleanup(run.uninstall_recursionlimit_wrappers)
        f = sys.setrecursionlimit
        self.assertRaises(TypeError, f, limit=100)
        self.assertRaises(TypeError, f, 100, 1000)
        self.assertRaises(ValueError, f, 0)

    def test_roundtrip(self):
        run.install_recursionlimit_wrappers()
        self.addCleanup(run.uninstall_recursionlimit_wrappers)

        # Check that setting the recursion limit works.
        orig_reclimit = sys.getrecursionlimit()
        self.addCleanup(sys.setrecursionlimit, orig_reclimit)
        sys.setrecursionlimit(orig_reclimit + 3)

        # Check that the new limit is returned by sys.getrecursionlimit().
        new_reclimit = sys.getrecursionlimit()
        self.assertEqual(new_reclimit, orig_reclimit + 3)

    def test_default_recursion_limit_preserved(self):
        orig_reclimit = sys.getrecursionlimit()
        run.install_recursionlimit_wrappers()
        self.addCleanup(run.uninstall_recursionlimit_wrappers)
        new_reclimit = sys.getrecursionlimit()
        self.assertEqual(new_reclimit, orig_reclimit)

    def test_fixdoc(self):
        # Put here until better place for miscellaneous test.
        def func(): "docstring"
        run.fixdoc(func, "more")
        self.assertEqual(func.__doc__, "docstring\n\nmore")
        func.__doc__ = None
        run.fixdoc(func, "more")
        self.assertEqual(func.__doc__, "more")


class HandleErrorTest(unittest.TestCase):
    # Method of MyRPCServer
    func = Func()
    @mock.patch('idlelib.run.thread.interrupt_main', new=func)
    def test_error(self):
        eq = self.assertEqual
        with captured_output('__stderr__') as err:
            try:
                raise EOFError
            except EOFError:
                run.MyRPCServer.handle_error(None, 'abc', '123')
            eq(run.exit_now, True)
            run.exit_now = False
            eq(err.getvalue(), '')

            try:
                raise IndexError
            except IndexError:
                run.MyRPCServer.handle_error(None, 'abc', '123')
            eq(run.quitting, True)
            run.quitting = False
            msg = err.getvalue()
            self.assertIn('abc', msg)
            self.assertIn('123', msg)
            self.assertIn('IndexError', msg)
            eq(self.func.called, 2)

if __name__ == '__main__':
    unittest.main(verbosity=2)
