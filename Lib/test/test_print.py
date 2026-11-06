import unittest
import sys
from io import StringIO

from test import support

NotDefined = object()

# A dispatch table all 8 combinations of providing
# sep, end, and file.
# I use this machinery so that I'm not just passing default
# values to print, I'm either passing or not passing in the
# arguments.
dispatch = {
    (False, False, False):
        lambda args, sep, end, file: print(*args),
    (False, False, True):
        lambda args, sep, end, file: print(file=file, *args),
    (False, True,  False):
        lambda args, sep, end, file: print(end=end, *args),
    (False, True,  True):
        lambda args, sep, end, file: print(end=end, file=file, *args),
    (True,  False, False):
        lambda args, sep, end, file: print(sep=sep, *args),
    (True,  False, True):
        lambda args, sep, end, file: print(sep=sep, file=file, *args),
    (True,  True,  False):
        lambda args, sep, end, file: print(sep=sep, end=end, *args),
    (True,  True,  True):
        lambda args, sep, end, file: print(sep=sep, end=end, file=file, *args),
}


# Class used to test __str__ and print
class ClassWith__str__:
    def __init__(self, x):
        self.x = x

    def __str__(self):
        return self.x


class RecordingFile:
    def __init__(self):
        self.writes = []
        self.flushes = 0

    def write(self, text):
        self.writes.append(text)

    def flush(self):
        self.flushes += 1


class TestPrint(unittest.TestCase):
    """Test correct operation of the print function."""

    def check(self, expected, args,
              sep=NotDefined, end=NotDefined, file=NotDefined):
        # Capture sys.stdout in a StringIO.  Call print with args,
        # and with sep, end, and file, if they're defined.  Result
        # must match expected.

        # Look up the actual function to call, based on if sep, end,
        # and file are defined.
        fn = dispatch[(sep is not NotDefined,
                       end is not NotDefined,
                       file is not NotDefined)]

        with support.captured_stdout() as t:
            fn(args, sep, end, file)

        self.assertEqual(t.getvalue(), expected)

    def test_print(self):
        def x(expected, args, sep=NotDefined, end=NotDefined):
            # Run the test 2 ways: not using file, and using
            # file directed to a StringIO.

            self.check(expected, args, sep=sep, end=end)

            # When writing to a file, stdout is expected to be empty
            o = StringIO()
            self.check('', args, sep=sep, end=end, file=o)

            # And o will contain the expected output
            self.assertEqual(o.getvalue(), expected)

        x('\n', ())
        x('a\n', ('a',))
        x('None\n', (None,))
        x('1 2\n', (1, 2))
        x('1   2\n', (1, ' ', 2))
        x('1*2\n', (1, 2), sep='*')
        x('1 s', (1, 's'), end='')
        x('a\nb\n', ('a', 'b'), sep='\n')
        x('1.01', (1.0, 1), sep='', end='')
        x('1*a*1.3+', (1, 'a', 1.3), sep='*', end='+')
        x('a\n\nb\n', ('a\n', 'b'), sep='\n')
        x('\0+ +\0\n', ('\0', ' ', '\0'), sep='+')

        x('a\n b\n', ('a\n', 'b'))
        x('a\n b\n', ('a\n', 'b'), sep=None)
        x('a\n b\n', ('a\n', 'b'), end=None)
        x('a\n b\n', ('a\n', 'b'), sep=None, end=None)

        x('*\n', (ClassWith__str__('*'),))
        x('abc 1\n', (ClassWith__str__('abc'), 1))

        # errors
        self.assertRaises(TypeError, print, '', sep=3)
        self.assertRaises(TypeError, print, '', end=3)
        self.assertRaises(AttributeError, print, '', file='')

    def test_print_single_write(self):
        cases = [
            ((), {}, '\n'),
            ((), {'end': ''}, ''),
            (('',), {'end': ''}, ''),
            (('a', 1, None), {}, 'a 1 None\n'),
            (('a', 'b'), {'sep': None, 'end': None}, 'a b\n'),
            (('a', 'b'), {'sep': '', 'end': ''}, 'ab'),
            (('a\0', '\u20ac', '\U0001f40d'),
             {'sep': '\u3000', 'end': '!'}, 'a\0\u3000\u20ac\u3000\U0001f40d!'),
        ]
        for args, kwargs, expected in cases:
            with self.subTest(args=args, kwargs=kwargs):
                file = RecordingFile()
                self.assertIsNone(print(*args, file=file, **kwargs))
                self.assertEqual(file.writes, [expected])
                self.assertEqual(file.flushes, 0)

    def test_print_converts_before_writing(self):
        file = RecordingFile()
        converted = []

        class Value:
            def __str__(value):
                self.assertEqual(file.writes, [])
                converted.append(True)
                return 'value'

        print(Value(), Value(), file=file)
        self.assertEqual(converted, [True, True])
        self.assertEqual(file.writes, ['value value\n'])

    def test_print_str_subclass(self):
        class Str(str):
            def __str__(self):
                raise AssertionError('unexpected conversion')

        file = RecordingFile()
        print(ClassWith__str__(Str('value')), 'other',
              sep=Str('|'), end=Str('!'), file=file)
        self.assertEqual(file.writes, ['value|other!'])

    def test_print_str_error(self):
        error = ValueError('cannot convert')

        class BadStr:
            def __str__(self):
                raise error

        for prefix in [(), ('',), ('first',), ('first', 'second')]:
            for sep in [' ', '', '::']:
                for flush in [False, True]:
                    with self.subTest(prefix=prefix, sep=sep, flush=flush):
                        file = RecordingFile()
                        with self.assertRaises(ValueError) as cm:
                            print(*prefix, BadStr(), 'not printed', sep=sep,
                                  end='END', file=file, flush=flush)
                        self.assertIs(cm.exception, error)
                        expected = sep.join((*prefix, ''))
                        self.assertEqual(file.writes,
                                         [expected] if prefix else [])
                        self.assertEqual(file.flushes,
                                         int(flush and bool(prefix)))

        file = RecordingFile()
        with self.assertRaises(TypeError):
            print('first', ClassWith__str__(42), file=file)
        self.assertEqual(file.writes, ['first '])

    def test_print_write_and_flush_errors(self):
        for operation in ['write', 'flush']:
            for str_error in [False, True]:
                with self.subTest(operation=operation, str_error=str_error):
                    conversion_error = ValueError('cannot convert')
                    output_error = OSError('cannot output')
                    handled = []

                    class Value:
                        def __str__(self):
                            if str_error:
                                raise conversion_error
                            return 'value'

                    class File(RecordingFile):
                        def write(self, text):
                            super().write(text)
                            handled.append(sys.exception())
                            if operation == 'write':
                                raise output_error

                        def flush(self):
                            super().flush()
                            if operation == 'flush':
                                raise output_error

                    file = File()
                    previous_exception = sys.exception()
                    with self.assertRaises(OSError) as cm:
                        print('first', Value(), file=file, flush=True)
                    self.assertIs(cm.exception, output_error)
                    expected_context = (conversion_error if str_error
                                        else previous_exception)
                    self.assertIs(output_error.__context__, expected_context)
                    self.assertEqual(handled, [expected_context])
                    self.assertIs(sys.exception(), previous_exception)
                    expected = 'first ' if str_error else 'first value\n'
                    self.assertEqual(file.writes, [expected])
                    self.assertEqual(file.flushes, int(operation == 'flush'))

    @support.nomemtest
    def test_print_join_error(self):
        import _testcapi

        class Value:
            def __str__(self):
                # The parts list has room for this string and the terminator.
                # Fail the allocation of the joined string only.
                _testcapi.set_nomemory(0, 1)
                return 'second'

        file = RecordingFile()
        try:
            with self.assertRaises(MemoryError):
                print('first', Value(), file=file, flush=True)
        finally:
            _testcapi.remove_mem_hooks()
        self.assertEqual(file.writes, ['first', ' ', 'second', '\n'])
        self.assertEqual(file.flushes, 1)

    def test_print_flush(self):
        # operation of the flush flag
        class filelike:
            def __init__(self):
                self.written = ''
                self.flushed = 0

            def write(self, str):
                self.written += str

            def flush(self):
                self.flushed += 1

        f = filelike()
        print(1, file=f, end='', flush=True)
        print(2, file=f, end='', flush=True)
        print(3, file=f, flush=False)
        self.assertEqual(f.written, '123\n')
        self.assertEqual(f.flushed, 2)

        # ensure exceptions from flush are passed through
        class noflush:
            def write(self, str):
                pass

            def flush(self):
                raise RuntimeError
        self.assertRaises(RuntimeError, print, 1, file=noflush(), flush=True)

    def test_gh130163(self):
        class X:
            def __str__(self):
                sys.stdout = StringIO()
                support.gc_collect()
                return 'foo'

        with support.swap_attr(sys, 'stdout', None):
            sys.stdout = StringIO()  # the only reference
            print(X())  # should not crash


class TestPy2MigrationHint(unittest.TestCase):
    """Test that correct hint is produced analogous to Python3 syntax,
    if print statement is executed as in Python 2.
    """

    def test_normal_string(self):
        python2_print_str = 'print "Hello World"'
        with self.assertRaises(SyntaxError) as context:
            exec(python2_print_str)

        self.assertIn("Missing parentheses in call to 'print'. Did you mean print(...)",
                str(context.exception))

    def test_string_with_soft_space(self):
        python2_print_str = 'print "Hello World",'
        with self.assertRaises(SyntaxError) as context:
            exec(python2_print_str)

        self.assertIn("Missing parentheses in call to 'print'. Did you mean print(...)",
                str(context.exception))

    def test_string_with_excessive_whitespace(self):
        python2_print_str = 'print  "Hello World", '
        with self.assertRaises(SyntaxError) as context:
            exec(python2_print_str)

        self.assertIn("Missing parentheses in call to 'print'. Did you mean print(...)",
                str(context.exception))

    def test_string_with_leading_whitespace(self):
        python2_print_str = '''if 1:
            print "Hello World"
        '''
        with self.assertRaises(SyntaxError) as context:
            exec(python2_print_str)

        self.assertIn("Missing parentheses in call to 'print'. Did you mean print(...)",
                str(context.exception))

    # bpo-32685: Suggestions for print statement should be proper when
    # it is in the same line as the header of a compound statement
    # and/or followed by a semicolon
    def test_string_with_semicolon(self):
        python2_print_str = 'print p;'
        with self.assertRaises(SyntaxError) as context:
            exec(python2_print_str)

        self.assertIn("Missing parentheses in call to 'print'. Did you mean print(...)",
                str(context.exception))

    def test_string_in_loop_on_same_line(self):
        python2_print_str = 'for i in s: print i'
        with self.assertRaises(SyntaxError) as context:
            exec(python2_print_str)

        self.assertIn("Missing parentheses in call to 'print'. Did you mean print(...)",
                str(context.exception))


if __name__ == "__main__":
    unittest.main()
