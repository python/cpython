
import test.test_support, unittest
import os

class CodingTest(unittest.TestCase):
    def test_bad_coding(self):
        module_name = 'bad_coding'
        self.assertRaises(SyntaxError, __import__, 'test.' + module_name)

        path = os.path.dirname(__file__)
        filename = os.path.join(path, module_name + '.py')
        fp = open(filename)
        text = fp.read()
        fp.close()
        self.assertRaises(SyntaxError, compile, text, filename, 'exec')

def test_main():
    test.test_support.run_unittest(CodingTest)

if __name__ == "__main__":
    test_main()
