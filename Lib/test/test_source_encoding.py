# -*- coding: utf-8 -*-

import unittest
from test.support import script_helper, captured_stdout, requires_subprocess, requires_resource
from test.support.os_helper import TESTFN, unlink, rmtree
from test.support.import_helper import unload
import importlib
import os
import sys
import subprocess
import tempfile

class MiscSourceEncodingTest(unittest.TestCase):

    def test_import_encoded_module(self):
        from test.encoded_modules import test_strings
        # Make sure we're actually testing something
        self.assertGreaterEqual(len(test_strings), 1)
        for modname, encoding, teststr in test_strings:
            mod = importlib.import_module('test.encoded_modules.'
                                          'module_' + modname)
            self.assertEqual(teststr, mod.test)

    def test_compilestring(self):
        # see #1882
        c = compile(b"\n# coding: utf-8\nu = '\xc3\xb3'\n", "dummy", "exec")
        d = {}
        exec(c, d)
        self.assertEqual(d['u'], '\xf3')

    def test_issue2301(self):
        try:
            compile(b"# coding: cp932\nprint '\x94\x4e'", "dummy", "exec")
        except SyntaxError as v:
            self.assertEqual(v.text.rstrip('\n'), "print '\u5e74'")
        else:
            self.fail()

    def test_issue4626(self):
        c = compile("# coding=latin-1\n\u00c6 = '\u00c6'", "dummy", "exec")
        d = {}
        exec(c, d)
        self.assertEqual(d['\xc6'], '\xc6')

    def test_issue3297(self):
        c = compile("a, b = '\U0001010F', '\\U0001010F'", "dummy", "exec")
        d = {}
        exec(c, d)
        self.assertEqual(d['a'], d['b'])
        self.assertEqual(len(d['a']), len(d['b']))
        self.assertEqual(ascii(d['a']), ascii(d['b']))

    def test_issue7820(self):
        # Ensure that check_bom() restores all bytes in the right order if
        # check_bom() fails in pydebug mode: a buffer starts with the first
        # byte of a valid BOM, but next bytes are different

        # one byte in common with the UTF-16-LE BOM
        self.assertRaises(SyntaxError, eval, b'\xff\x20')

        # one byte in common with the UTF-8 BOM
        self.assertRaises(SyntaxError, eval, b'\xef\x20')

        # two bytes in common with the UTF-8 BOM
        self.assertRaises(SyntaxError, eval, b'\xef\xbb\x20')

    @requires_subprocess()
    def test_20731(self):
        sub = subprocess.Popen([sys.executable,
                        os.path.join(os.path.dirname(__file__),
                                     'tokenizedata',
                                     'coding20731.py')],
                        stderr=subprocess.PIPE)
        err = sub.communicate()[1]
        self.assertEqual(sub.returncode, 0)
        self.assertNotIn(b'SyntaxError', err)

    def test_error_message(self):
        compile(b'# -*- coding: iso-8859-15 -*-\n', 'dummy', 'exec')
        compile(b'\xef\xbb\xbf\n', 'dummy', 'exec')
        compile(b'\xef\xbb\xbf# -*- coding: utf-8 -*-\n', 'dummy', 'exec')
        with self.assertRaisesRegex(SyntaxError, 'fake'):
            compile(b'# -*- coding: fake -*-\n', 'dummy', 'exec')
        with self.assertRaisesRegex(SyntaxError, 'iso-8859-15'):
            compile(b'\xef\xbb\xbf# -*- coding: iso-8859-15 -*-\n',
                    'dummy', 'exec')
        with self.assertRaisesRegex(SyntaxError, 'BOM'):
            compile(b'\xef\xbb\xbf# -*- coding: iso-8859-15 -*-\n',
                    'dummy', 'exec')
        with self.assertRaisesRegex(SyntaxError, 'fake'):
            compile(b'\xef\xbb\xbf# -*- coding: fake -*-\n', 'dummy', 'exec')
        with self.assertRaisesRegex(SyntaxError, 'BOM'):
            compile(b'\xef\xbb\xbf# -*- coding: fake -*-\n', 'dummy', 'exec')

    def test_bad_coding(self):
        module_name = 'bad_coding'
        self.verify_bad_module(module_name)

    def test_bad_coding2(self):
        module_name = 'bad_coding2'
        self.verify_bad_module(module_name)

    def verify_bad_module(self, module_name):
        self.assertRaises(SyntaxError, __import__, 'test.tokenizedata.' + module_name)

        path = os.path.dirname(__file__)
        filename = os.path.join(path, 'tokenizedata', module_name + '.py')
        with open(filename, "rb") as fp:
            bytes = fp.read()
        self.assertRaises(SyntaxError, compile, bytes, filename, 'exec')

    def test_exec_valid_coding(self):
        d = {}
        exec(b'# coding: cp949\na = "\xaa\xa7"\n', d)
        self.assertEqual(d['a'], '\u3047')

    def test_file_parse(self):
        # issue1134: all encodings outside latin-1 and utf-8 fail on
        # multiline strings and long lines (>512 columns)
        unload(TESTFN)
        filename = TESTFN + ".py"
        f = open(filename, "w", encoding="cp1252")
        sys.path.insert(0, os.curdir)
        try:
            with f:
                f.write("# -*- coding: cp1252 -*-\n")
                f.write("'''A short string\n")
                f.write("'''\n")
                f.write("'A very long string %s'\n" % ("X" * 1000))

            importlib.invalidate_caches()
            __import__(TESTFN)
        finally:
            del sys.path[0]
            unlink(filename)
            unlink(filename + "c")
            unlink(filename + "o")
            unload(TESTFN)
            rmtree('__pycache__')

    def test_error_from_string(self):
        # See http://bugs.python.org/issue6289
        input = "# coding: ascii\n\N{SNOWMAN}".encode('utf-8')
        with self.assertRaises(SyntaxError) as c:
            compile(input, "<string>", "exec")
        expected = "'ascii' codec can't decode byte 0xe2 in position 16: " \
                   "ordinal not in range(128)"
        self.assertStartsWith(c.exception.args[0], expected)

    def test_file_parse_error_multiline(self):
        # gh96611:
        with open(TESTFN, "wb") as fd:
            fd.write(b'print("""\n\xb1""")\n')

        try:
            retcode, stdout, stderr = script_helper.assert_python_failure(TESTFN)

            self.assertGreater(retcode, 0)
            self.assertIn(b"Non-UTF-8 code starting with '\\xb1'", stderr)
        finally:
            os.unlink(TESTFN)

    def test_tokenizer_fstring_warning_in_first_line(self):
        source = "0b1and 2"
        with open(TESTFN, "w") as fd:
            fd.write("{}".format(source))
        try:
            retcode, stdout, stderr = script_helper.assert_python_ok(TESTFN)
            self.assertIn(b"SyntaxWarning: invalid binary litera", stderr)
            self.assertEqual(stderr.count(source.encode()), 1)
        finally:
            os.unlink(TESTFN)


class AbstractSourceEncodingTest:

    def test_default_coding(self):
        src = (b'print(ascii("\xc3\xa4"))\n')
        self.check_script_output(src, br"'\xe4'")

    def test_first_coding_line(self):
        src = (b'#coding:iso8859-15\n'
               b'print(ascii("\xc3\xa4"))\n')
        self.check_script_output(src, br"'\xc3\u20ac'")

    def test_second_coding_line(self):
        src = (b'#\n'
               b'#coding:iso8859-15\n'
               b'print(ascii("\xc3\xa4"))\n')
        self.check_script_output(src, br"'\xc3\u20ac'")

    def test_third_coding_line(self):
        # Only first two lines are tested for a magic comment.
        src = (b'#\n'
               b'#\n'
               b'#coding:iso8859-15\n'
               b'print(ascii("\xc3\xa4"))\n')
        self.check_script_output(src, br"'\xe4'")

    def test_double_coding_line(self):
        # If the first line matches the second line is ignored.
        src = (b'#coding:iso8859-15\n'
               b'#coding:latin1\n'
               b'print(ascii("\xc3\xa4"))\n')
        self.check_script_output(src, br"'\xc3\u20ac'")

    def test_double_coding_same_line(self):
        src = (b'#coding:iso8859-15 coding:latin1\n'
               b'print(ascii("\xc3\xa4"))\n')
        self.check_script_output(src, br"'\xc3\u20ac'")

    def test_first_non_utf8_coding_line(self):
        src = (b'#coding:iso-8859-15 \xa4\n'
               b'print(ascii("\xc3\xa4"))\n')
        self.check_script_output(src, br"'\xc3\u20ac'")

    def test_second_non_utf8_coding_line(self):
        src = (b'\n'
               b'#coding:iso-8859-15 \xa4\n'
               b'print(ascii("\xc3\xa4"))\n')
        self.check_script_output(src, br"'\xc3\u20ac'")

    def test_utf8_bom(self):
        src = (b'\xef\xbb\xbfprint(ascii("\xc3\xa4"))\n')
        self.check_script_output(src, br"'\xe4'")

    def test_utf8_bom_and_utf8_coding_line(self):
        src = (b'\xef\xbb\xbf#coding:utf-8\n'
               b'print(ascii("\xc3\xa4"))\n')
        self.check_script_output(src, br"'\xe4'")

    def test_crlf(self):
        src = (b'print(ascii("""\r\n"""))\n')
        out = self.check_script_output(src, br"'\n'")

    def test_crcrlf(self):
        src = (b'print(ascii("""\r\r\n"""))\n')
        out = self.check_script_output(src, br"'\n\n'")

    def test_crcrcrlf(self):
        src = (b'print(ascii("""\r\r\r\n"""))\n')
        out = self.check_script_output(src, br"'\n\n\n'")

    def test_crcrcrlf2(self):
        src = (b'#coding:iso-8859-1\n'
               b'print(ascii("""\r\r\r\n"""))\n')
        out = self.check_script_output(src, br"'\n\n\n'")


class UTF8ValidatorTest(unittest.TestCase):
    @unittest.skipIf(not sys.platform.startswith("linux"),
                     "Too slow to run on non-Linux platforms")
    @requires_resource('cpu')
    def test_invalid_utf8(self):
        # This is a port of test_utf8_decode_invalid_sequences in
        # test_unicode.py to exercise the separate utf8 validator in
        # Parser/tokenizer/helpers.c used when reading source files.

        # That file is written using low-level C file I/O, so the only way to
        # test it is to write actual files to disk.

        # Each example is put inside a string at the top of the file so
        # it's an otherwise valid Python source file. Put some newlines
        # beforehand so we can assert that the error is reported on the
        # correct line.
        template = b'\n\n\n"%s"\n'

        fn = TESTFN
        self.addCleanup(unlink, fn)

        def check(content):
            with open(fn, 'wb') as fp:
                fp.write(template % content)
            rc, stdout, stderr = script_helper.assert_python_failure(fn)
            # We want to assert that the python subprocess failed gracefully,
            # not via a signal.
            self.assertGreaterEqual(rc, 1)
            self.assertIn(b"Non-UTF-8 code starting with", stderr)
            self.assertIn(b"on line 4", stderr)

        # continuation bytes in a sequence of 2, 3, or 4 bytes
        continuation_bytes = [bytes([x]) for x in range(0x80, 0xC0)]
        # start bytes of a 2-byte sequence equivalent to code points < 0x7F
        invalid_2B_seq_start_bytes = [bytes([x]) for x in range(0xC0, 0xC2)]
        # start bytes of a 4-byte sequence equivalent to code points > 0x10FFFF
        invalid_4B_seq_start_bytes = [bytes([x]) for x in range(0xF5, 0xF8)]
        invalid_start_bytes = (
            continuation_bytes + invalid_2B_seq_start_bytes +
            invalid_4B_seq_start_bytes + [bytes([x]) for x in range(0xF7, 0x100)]
        )

        for byte in invalid_start_bytes:
            check(byte)

        for sb in invalid_2B_seq_start_bytes:
            for cb in continuation_bytes:
                check(sb + cb)

        for sb in invalid_4B_seq_start_bytes:
            for cb1 in continuation_bytes[:3]:
                for cb3 in continuation_bytes[:3]:
                    check(sb+cb1+b'\x80'+cb3)

        for cb in [bytes([x]) for x in range(0x80, 0xA0)]:
            check(b'\xE0'+cb+b'\x80')
            check(b'\xE0'+cb+b'\xBF')
            # surrogates
        for cb in [bytes([x]) for x in range(0xA0, 0xC0)]:
            check(b'\xED'+cb+b'\x80')
            check(b'\xED'+cb+b'\xBF')
        for cb in [bytes([x]) for x in range(0x80, 0x90)]:
            check(b'\xF0'+cb+b'\x80\x80')
            check(b'\xF0'+cb+b'\xBF\xBF')
        for cb in [bytes([x]) for x in range(0x90, 0xC0)]:
            check(b'\xF4'+cb+b'\x80\x80')
            check(b'\xF4'+cb+b'\xBF\xBF')


class BytesSourceEncodingTest(AbstractSourceEncodingTest, unittest.TestCase):

    def check_script_output(self, src, expected):
        with captured_stdout() as stdout:
            exec(src)
        out = stdout.getvalue().encode('latin1')
        self.assertEqual(out.rstrip(), expected)


class FileSourceEncodingTest(AbstractSourceEncodingTest, unittest.TestCase):

    def check_script_output(self, src, expected):
        with tempfile.TemporaryDirectory() as tmpd:
            fn = os.path.join(tmpd, 'test.py')
            with open(fn, 'wb') as fp:
                fp.write(src)
            res = script_helper.assert_python_ok(fn)
        self.assertEqual(res.out.rstrip(), expected)


if __name__ == "__main__":
    unittest.main()
