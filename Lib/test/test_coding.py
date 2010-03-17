
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
        with open(filename) as fp:
            text = fp.read()
        self.assertRaises(SyntaxError, compile, text, filename, 'exec')

    def test_error_from_string(self):
        # See http://bugs.python.org/issue6289
        input = u"# coding: ascii\n\N{SNOWMAN}".encode('utf-8')
        with self.assertRaises(SyntaxError) as c:
            compile(input, "<string>", "exec")
        expected = "'ascii' codec can't decode byte 0xe2 in position 16: " \
                   "ordinal not in range(128)"
        self.assertTrue(c.exception.args[0].startswith(expected))


def test_main():
    test.test_support.run_unittest(CodingTest)

if __name__ == "__main__":
    test_main()
