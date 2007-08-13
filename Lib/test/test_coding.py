
import test.test_support, unittest
import os

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

def test_main():
    test.test_support.run_unittest(CodingTest)

if __name__ == "__main__":
    test_main()
