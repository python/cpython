
import test.support, unittest
from test.support import TESTFN, unlink
import os, sys

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
        fp = open(filename, "rb")
        bytes = fp.read()
        fp.close()
        self.assertRaises(SyntaxError, compile, bytes, filename, 'exec')

    def test_exec_valid_coding(self):
        d = {}
        exec('# coding: cp949\na = 5\n', d)
        self.assertEqual(d['a'], 5)

    def test_file_parse(self):
        # issue1134: all encodings outside latin-1 and utf-8 fail on
        # multiline strings and long lines (>512 columns)
        if TESTFN in sys.modules:
            del sys.modules[TESTFN]
        sys.path.insert(0, ".")
        filename = TESTFN + ".py"
        f = open(filename, "w")
        try:
            f.write("# -*- coding: cp1252 -*-\n")
            f.write("'''A short string\n")
            f.write("'''\n")
            f.write("'A very long string %s'\n" % ("X" * 1000))
            f.close()

            __import__(TESTFN)
        finally:
            f.close()
            unlink(TESTFN+".py")
            unlink(TESTFN+".pyc")
            sys.path.pop(0)

    def test_error_from_string(self):
        # See http://bugs.python.org/issue6289
        input = "# coding: ascii\n\N{SNOWMAN}".encode('utf-8')
        try:
            compile(input, "<string>", "exec")
        except SyntaxError as e:
            expected = "'ascii' codec can't decode byte 0xe2 in position 16: " \
                "ordinal not in range(128)"
            self.assertTrue(str(e).startswith(expected))
        else:
            self.fail("didn't raise")

def test_main():
    test.support.run_unittest(CodingTest)

if __name__ == "__main__":
    test_main()
