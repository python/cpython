
import test.test_support, unittest
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
        fp = open(filename, encoding='utf-8')
        text = fp.read()
        fp.close()
        self.assertRaises(SyntaxError, compile, text, filename, 'exec')

    def test_exec_valid_coding(self):
        d = {}
        exec('# coding: cp949\na = 5\n', d)
        self.assertEqual(d['a'], 5)

    def test_file_parse(self):
        # issue1134: all encodings outside latin-1 and utf-8 fail on
        # multiline strings and long lines (>512 columns)
        sys.path.insert(0, ".")
        filename = test.test_support.TESTFN+".py"
        f = open(filename, "w")
        try:
            f.write("# -*- coding: cp1252 -*-\n")
            f.write("'''A short string\n")
            f.write("'''\n")
            f.write("'A very long string %s'\n" % ("X" * 1000))
            f.close()

            __import__(test.test_support.TESTFN)
        finally:
            f.close()
            os.remove(test.test_support.TESTFN+".py")
            os.remove(test.test_support.TESTFN+".pyc")
            sys.path.pop(0)

def test_main():
    test.test_support.run_unittest(CodingTest)

if __name__ == "__main__":
    test_main()
