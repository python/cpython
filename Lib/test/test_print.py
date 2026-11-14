import codecs
import io
import sys
import unittest
from io import StringIO

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


class RecordingFile:
    def __init__(self):
        self.writes = []
        self.flushes = 0

    def write(self, text):
        self.writes.append(text)

    def flush(self):
        self.flushes += 1


class RecordingBytesIO(io.BytesIO):
    def __init__(self):
        super().__init__()
        self.writes = []
        self.flushes = 0

    def write(self, data):
        self.writes.append(data)
        return super().write(data)

    def flush(self):
        self.flushes += 1
        return super().flush()


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
            (tuple(range(100)), {'sep': '|', 'end': '!'},
             '|'.join(map(str, range(100))) + '!'),
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

    def test_print_textio_encodings(self):
        cases = [
            ('ascii', ('first', 'second', 'a\0b')),
            ('latin-1', ('caf\xe9', '\xff', 'a\0b')),
            ('utf-8', ('\xe9', '\u65e5\u672c\u8a9e', '\U0001f40d')),
            ('utf-8-sig', ('\xe9', '\u65e5\u672c\u8a9e', '\U0001f40d')),
            ('utf-16', ('\xe9', '\u65e5\u672c\u8a9e', '\U0001f40d')),
            ('utf-32', ('\xe9', '\u65e5\u672c\u8a9e', '\U0001f40d')),
            ('iso2022_jp', ('\u65e5\u672c', '\u8a9e')),
            ('utf-7', ('\u65e5\u672c', '\u8a9e')),
        ]
        for encoding, args in cases:
            for newline in [None, '', '\n', '\r', '\r\n']:
                with self.subTest(encoding=encoding, newline=newline):
                    actual = RecordingBytesIO()
                    expected = io.BytesIO()
                    with (io.TextIOWrapper(actual, encoding=encoding,
                                           newline=newline,
                                           line_buffering=True) as file,
                          io.TextIOWrapper(expected, encoding=encoding,
                                           newline=newline,
                                           line_buffering=True) as reference):
                        for values in [(), args, ('',), args]:
                            print(*values, sep='|', file=file)
                            reference.write('|'.join(values) + '\n')
                            self.assertEqual(actual.getvalue(),
                                             expected.getvalue())
                        self.assertEqual(len(actual.writes), 4)
                        self.assertTrue(all(isinstance(b, bytes)
                                            for b in actual.writes))

    @support.cpython_only
    def test_print_textio_utf8_cache(self):
        _testcapi = import_helper.import_module('_testcapi')
        for cached in [False, True]:
            for size in [1, 100, 10000]:
                with self.subTest(cached=cached, size=size):
                    # Decoding creates new strings without a UTF-8 cache.
                    args = [(s * size).encode().decode()
                            for s in ['a\0', '\xe9', '\u20ac', '\U0001f40d']]
                    if cached:
                        for arg in args:
                            _testcapi.unicode_asutf8(arg, len(arg.encode()))
                    sizes = [sys.getsizeof(arg) for arg in args]
                    raw = RecordingBytesIO()
                    with io.TextIOWrapper(raw, encoding='utf-8', newline='\n',
                                          line_buffering=True) as file:
                        print(*args, file=file)
                        self.assertEqual(raw.writes,
                                         [(' '.join(args) + '\n').encode()])
                    # Writing must not populate a previously absent cache.
                    self.assertEqual([sys.getsizeof(arg) for arg in args],
                                     sizes)

    def test_print_textio_buffering(self):
        for line_buffering in [False, True]:
            for write_through in [False, True]:
                for end in ['', '\n', '\r', '\r\n']:
                    for flush in [False, True]:
                        with self.subTest(line_buffering=line_buffering,
                                          write_through=write_through,
                                          end=end, flush=flush):
                            raw = RecordingBytesIO()
                            with io.TextIOWrapper(
                                    raw, encoding='utf-8', newline='\n',
                                    line_buffering=line_buffering,
                                    write_through=write_through) as file:
                                print('first', 'second', end=end,
                                      file=file, flush=flush)
                                expected = ('first second' + end).encode()
                                flushes = (int(flush)
                                           + int(line_buffering and bool(end)))
                                self.assertEqual(raw.flushes, flushes)
                                self.assertEqual(
                                    raw.writes,
                                    [expected] if flushes or write_through
                                    else [])
                                file.flush()
                                self.assertEqual(raw.getvalue(), expected)

    def test_print_textio_pending_bytes(self):
        for size in [0, 10, 10000]:
            with self.subTest(size=size):
                raw = io.BytesIO()
                with io.TextIOWrapper(raw, encoding='utf-8',
                                      newline='\n') as file:
                    file.write('prefix')
                    file.write('\xe9')
                    print('a' * size, '\u20ac', file=file)
                    file.flush()
                    expected = ('prefix\xe9' + 'a' * size + ' \u20ac\n').encode()
                    self.assertEqual(raw.getvalue(), expected)
                    file.seek(0)
                    self.assertEqual(file.read(), raw.getvalue().decode())

    def test_print_textio_newline_in_parts(self):
        for args, sep, end in [(('first\n', 'second'), ' ', ''),
                               (('first', 'second'), '\r', ''),
                               (('first', 'second'), '\n', ''),
                               (('first\r',), '', '')]:
            with self.subTest(args=args, sep=sep, end=end):
                raw = RecordingBytesIO()
                with io.TextIOWrapper(raw, encoding='utf-8', newline='\n',
                                      line_buffering=True) as file:
                    print(*args, sep=sep, end=end, file=file)
                    self.assertEqual(raw.writes,
                                     [(sep.join(args) + end).encode()])
                    self.assertEqual(raw.flushes, 1)

    def test_print_textio_str_subclass(self):
        class Str(str):
            def __str__(self):
                raise AssertionError('unexpected conversion')

        raw = io.BytesIO()
        with io.TextIOWrapper(raw, encoding='utf-8') as file:
            print(ClassWith__str__(Str('\u20ac')), 'other', sep=Str('|'),
                  end=Str('!'), file=file, flush=True)
            self.assertEqual(raw.getvalue(), '\u20ac|other!'.encode())

    def test_print_textio_write_override(self):
        class TextIO(io.TextIOWrapper):
            def write(self, text):
                writes.append(text)

        for cls in [io.TextIOWrapper, TextIO]:
            with self.subTest(cls=cls):
                writes = []
                with cls(io.BytesIO(), encoding='utf-8') as file:
                    if cls is io.TextIOWrapper:
                        file.write = writes.append
                    print('first', 'second', file=file)
                    self.assertEqual(writes, ['first second\n'])

    def test_print_textio_encoding_error(self):
        for encoding in ['ascii', 'latin-1', 'utf-8']:
            for errors in ['strict', 'replace', 'ignore', 'backslashreplace',
                           'surrogateescape', 'surrogatepass']:
                with self.subTest(encoding=encoding, errors=errors):
                    actual = io.BytesIO()
                    expected = io.BytesIO()
                    with (io.TextIOWrapper(actual, encoding=encoding,
                                           errors=errors) as file,
                          io.TextIOWrapper(expected, encoding=encoding,
                                           errors=errors) as reference):
                        text = 'first \udc80\udcff\ud800 last\n'
                        try:
                            reference.write(text)
                        except UnicodeEncodeError as error:
                            with self.assertRaises(UnicodeEncodeError) as cm:
                                print('first', '\udc80\udcff\ud800', 'last',
                                      file=file)
                            self.assertEqual(cm.exception.args, error.args)
                        else:
                            print('first', '\udc80\udcff\ud800', 'last',
                                  file=file)
                        file.flush()
                        reference.flush()
                        self.assertEqual(actual.getvalue(),
                                         expected.getvalue())

    def test_print_textio_error_handler(self):
        calls = []

        def handler(error):
            calls.append((error.object, error.start, error.end))
            return ('?', error.end)

        codecs.register_error('test_print_textio', handler)
        for encoding, value in [('ascii', '\xe9'), ('latin-1', '\u20ac'),
                                ('utf-8', '\ud800')]:
            with self.subTest(encoding=encoding):
                calls.clear()
                raw = io.BytesIO()
                with io.TextIOWrapper(raw, encoding=encoding, newline='\n',
                                      errors='test_print_textio') as file:
                    print('first', value, 'last', file=file, flush=True)
                    self.assertEqual(calls,
                                     [('first ' + value + ' last\n', 6, 7)])
                    self.assertEqual(raw.getvalue(), b'first ? last\n')

    @support.thread_unsafe("modifies the global 'strict' error handler")
    def test_print_textio_strict_error_handler(self):
        def handler(error):
            calls.append((error.object, error.start, error.end))
            file.reconfigure(newline='\r')
            return ('!', error.end)

        self.addCleanup(codecs.register_error, 'strict',
                        codecs.lookup_error('strict'))
        codecs.register_error('strict', handler)
        for value in ['\ud800', '\udfff',
                      '\U0001f40d\ud800', '\U0001f40d\udfff']:
            for errors in ['replace', 'strict']:
                with self.subTest(value=ascii(value), errors=errors):
                    calls = []
                    raw = io.BytesIO()
                    with io.TextIOWrapper(raw, encoding='utf-8', errors=errors,
                                          newline='\r\n') as file:
                        print('first', value, 'last', file=file, flush=True)
                        if errors == 'strict':
                            text = 'first ' + value + ' last\r\n'
                            start = 6 + len(value) - 1
                            self.assertEqual(calls, [(text, start, start + 1)])
                            replacement = '!'
                            newline = '\r'
                        else:
                            self.assertEqual(calls, [])
                            replacement = '?'
                            newline = '\r\n'
                        expected = ('first ' + value[:-1] + replacement
                                    + ' last\r\n')
                        self.assertEqual(raw.getvalue(), expected.encode())
                        print('next', file=file, flush=True)
                        self.assertEqual(raw.getvalue(),
                                         (expected + 'next' + newline).encode())

    def test_print_textio_closed_callback(self):
        class Buffer(io.BytesIO):
            @property
            def closed(self):
                calls.append(True)
                return super().closed

        for encoding in ['utf-8', 'utf-16']:
            for value in ['second', '\ud800']:
                with self.subTest(encoding=encoding, value=value):
                    calls = []
                    with io.TextIOWrapper(Buffer(), encoding=encoding,
                                          newline='\n',
                                          errors='replace') as file:
                        calls.clear()
                        print('first', value, file=file)
                        # Falling back to encoding the whole string should not
                        # check closed a second time and repeat the callback.
                        self.assertEqual(calls, [True])

    def test_print_textio_write_error(self):
        error = OSError('cannot write')
        conversion_error = ValueError('cannot convert')

        class Buffer(io.BytesIO):
            def write(self, data):
                raise error

        class BadStr:
            def __str__(self):
                raise conversion_error

        with io.TextIOWrapper(Buffer(), encoding='utf-8') as file:
            with self.assertRaises(OSError) as cm:
                print('first', BadStr(), file=file, flush=True)
            self.assertIs(cm.exception, error)
            self.assertIs(error.__context__, conversion_error)

    def test_print_textio_str_error(self):
        class BadStr:
            def __str__(self):
                raise ValueError('cannot convert')

        for prefix in [(), ('first',)]:
            with self.subTest(prefix=prefix):
                raw = io.BytesIO()
                with io.TextIOWrapper(raw, encoding='utf-8') as file:
                    with self.assertRaisesRegex(ValueError, 'cannot convert'):
                        print(*prefix, BadStr(), file=file, flush=True)
                    self.assertEqual(raw.getvalue(),
                                     ' '.join((*prefix, '')).encode())

    def test_print_textio_str_error_read_ahead(self):
        error = ValueError('cannot convert')

        class BadStr:
            def __str__(self):
                raise error

        for flush in [False, True]:
            with self.subTest(flush=flush):
                with io.TextIOWrapper(io.BytesIO(b'abcdef'),
                                      encoding='utf-8') as file:
                    self.assertEqual(file.read(1), 'a')
                    with self.assertRaises(ValueError) as cm:
                        print(BadStr(), file=file, flush=flush)
                    self.assertIs(cm.exception, error)
                    self.assertEqual(file.read(), 'bcdef')

    def test_print_textio_unusable(self):
        uninitialized = io.TextIOWrapper.__new__(io.TextIOWrapper)
        closed = io.TextIOWrapper(io.BytesIO(), encoding='utf-8')
        closed.close()
        detached = io.TextIOWrapper(io.BytesIO(), encoding='utf-8')
        detached.detach()
        for file in [uninitialized, closed, detached]:
            with self.subTest(file=file):
                with self.assertRaises(ValueError):
                    print('first', 'second', file=file)

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
