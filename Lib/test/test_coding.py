import unittest
from test.support import TESTFN, unlink, unload
import importlib, os, sys

class CodingTest(unittest.TestCase):
    def test_bad_coding(self):
        module_name = 'bad_coding'
        self.verify_bad_module(module_name)

    def test_bad_coding2(self):
        module_name = 'bad_coding2'
        self.verify_bad_module(module_name)

    def verify_bad_module(self, module_name):
        self.assertRaises(SyntaxError, __import__, 'test.' + module_name)

        path = os.path.dirname(__file__)
        filename = os.path.join(path, module_name + '.py')
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

    def test_error_from_string(self):
        # See http://bugs.python.org/issue6289
        input = "# coding: ascii\n\N{SNOWMAN}".encode('utf-8')
        with self.assertRaises(SyntaxError) as c:
            compile(input, "<string>", "exec")
        expected = "'ascii' codec can't decode byte 0xe2 in position 16: " \
                   "ordinal not in range(128)"
        self.assertTrue(c.exception.args[0].startswith(expected),
                        msg=c.exception.args[0])


if __name__ == "__main__":
    unittest.main()
