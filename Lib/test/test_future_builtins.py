import test.test_support, unittest

# we're testing the behavior of these future builtins:
from future_builtins import hex, oct

class BuiltinTest(unittest.TestCase):
    def test_hex(self):
        self.assertEqual(hex(0), '0x0')
        self.assertEqual(hex(16), '0x10')
        self.assertEqual(hex(16L), '0x10')
        self.assertEqual(hex(-16), '-0x10')
        self.assertEqual(hex(-16L), '-0x10')
        self.assertRaises(TypeError, hex, {})

    def test_oct(self):
        self.assertEqual(oct(0), '0o0')
        self.assertEqual(oct(100), '0o144')
        self.assertEqual(oct(100L), '0o144')
        self.assertEqual(oct(-100), '-0o144')
        self.assertEqual(oct(-100L), '-0o144')
        self.assertRaises(TypeError, oct, ())

def test_main(verbose=None):
    test.test_support.run_unittest(BuiltinTest)

if __name__ == "__main__":
    test_main(verbose=True)
