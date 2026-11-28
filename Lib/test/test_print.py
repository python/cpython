import codecs
import io
import sys
import unittest
from io import StringIO
from unittest.mock import Mock, call

from test import support
from test.support import import_helper

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
        class Str(str):
            def __str__(self):
                raise AssertionError('unexpected conversion')

        file = Mock()
        print(ClassWith__str__(Str('value')), 2, sep=Str('|'), end=Str('!'),
              file=file)
        file.write.assert_called_once_with('value|2!')

    def test_print_str_error(self):
        file = Mock()
        with self.assertRaises(TypeError):
            print('first', ClassWith__str__(42), file=file, flush=True)
        self.assertEqual(file.method_calls,
                         [call.write('first '), call.flush()])

        file.write.side_effect = OSError
        with self.assertRaises(OSError) as cm:
            print('first', ClassWith__str__(42), file=file)
        self.assertIsInstance(cm.exception.__context__, TypeError)

    def test_print_str_error_read_ahead(self):
        with io.TextIOWrapper(io.BytesIO(b'abcdef'), encoding='utf-8') as file:
            self.assertEqual(file.read(1), 'a')
            file.flush = Mock(wraps=file.flush)
            with self.assertRaises(TypeError):
                print(ClassWith__str__(42), file=file, flush=True)
            file.flush.assert_not_called()
            self.assertEqual(file.read(), 'bcdef')

    @support.nomemtest
    def test_print_allocation_error(self):
        import _testcapi

        class Value:
            def __str__(self):
                # Fail the first allocation after collecting the parts.
                _testcapi.set_nomemory(0, 1)
                return 'second'

        file = Mock()
        raw = io.BytesIO()
        with io.TextIOWrapper(raw, encoding='utf-8') as text:
            for dest in (file, text):
                try:
                    with self.assertRaises(MemoryError):
                        print('first', Value(), file=dest, flush=True)
                finally:
                    _testcapi.remove_mem_hooks()
            self.assertEqual(file.method_calls, [
                call.write('first'), call.write(' '), call.write('second'),
                call.write('\n'), call.flush(),
            ])
            self.assertEqual(raw.getvalue(), b'first second\n')

    def test_print_textio_write_error(self):
        raw = io.BytesIO()
        raw.write = Mock(side_effect=MemoryError)
        with io.TextIOWrapper(raw, encoding='utf-8',
                              line_buffering=True) as file:
            with self.assertRaises(MemoryError):
                print('first', 'second', file=file)
            # Retrying individual parts could duplicate output.
            raw.write.assert_called_once_with(b'first second\n')

    def test_print_textio_newline(self):
        for encoding in ('utf-8', 'latin-1', 'utf-16'):
            with self.subTest(encoding=encoding):
                raw = io.BytesIO()
                with io.TextIOWrapper(raw, encoding=encoding, newline='\r\n',
                                      line_buffering=True) as file:
                    print('café', 2, sep='\n', end='', file=file)
                    self.assertEqual(raw.getvalue(),
                                     'café\r\n2'.encode(encoding))

    def test_print_textio_write_override(self):
        with io.TextIOWrapper(io.BytesIO(), encoding='utf-8') as file:
            file.write = Mock()
            print('first', 'second', file=file)
            file.write.assert_called_once_with('first second\n')

    @support.cpython_only
    def test_print_textio_utf8_cache(self):
        _testcapi = import_helper.import_module('_testcapi')
        data = 'a\0€'.encode()
        cached, uncached = data.decode(), data.decode()
        _testcapi.unicode_asutf8(cached, len(data))
        size = sys.getsizeof(uncached)
        raw = io.BytesIO()
        with io.TextIOWrapper(raw, encoding='utf-8') as file:
            print(cached, uncached, file=file, flush=True)
            self.assertEqual(raw.getvalue(), data + b' ' + data + b'\n')
        self.assertEqual(sys.getsizeof(uncached), size)

    @support.thread_unsafe("modifies the global 'strict' error handler")
    def test_print_textio_strict_error_handler(self):
        def handler(error):
            self.assertEqual(error.object, 'first \ud800\r\n')
            file.reconfigure(newline='\r')
            return '!', error.end

        self.addCleanup(codecs.register_error, 'strict',
                        codecs.lookup_error('strict'))
        codecs.register_error('strict', handler)
        for errors, expected in [('replace', b'first ?\r\n'),
                                 ('strict', b'first !\r\n')]:
            with self.subTest(errors=errors):
                raw = io.BytesIO()
                with io.TextIOWrapper(raw, encoding='utf-8', errors=errors,
                                      newline='\r\n') as file:
                    print('first', '\ud800', file=file, flush=True)
                    self.assertEqual(raw.getvalue(), expected)

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
