import unittest
import warnings
import sys
from test import test_support

class TestSpecifics(unittest.TestCase):

    def test_debug_assignment(self):
        # catch assignments to __debug__
        self.assertRaises(SyntaxError, compile, '__debug__ = 1', '?', 'single')
        import __builtin__
        prev = __builtin__.__debug__
        setattr(__builtin__, '__debug__', 'sure')
        setattr(__builtin__, '__debug__', prev)

    def test_argument_handling(self):
        # detect duplicate positional and keyword arguments
        self.assertRaises(SyntaxError, eval, 'lambda a,a:0')
        self.assertRaises(SyntaxError, eval, 'lambda a,a=1:0')
        self.assertRaises(SyntaxError, eval, 'lambda a=1,a=1:0')
        try:
            exec 'def f(a, a): pass'
            self.fail("duplicate arguments")
        except SyntaxError:
            pass
        try:
            exec 'def f(a = 0, a = 1): pass'
            self.fail("duplicate keyword arguments")
        except SyntaxError:
            pass
        try:
            exec 'def f(a): global a; a = 1'
            self.fail("variable is global and local")
        except SyntaxError:
            pass

    def test_syntax_error(self):
        self.assertRaises(SyntaxError, compile, "1+*3", "filename", "exec")

    def test_duplicate_global_local(self):
        try:
            exec 'def f(a): global a; a = 1'
            self.fail("variable is global and local")
        except SyntaxError:
            pass

    def test_complex_args(self):

        def comp_args((a, b)):
            return a,b
        self.assertEqual(comp_args((1, 2)), (1, 2))

        def comp_args((a, b)=(3, 4)):
            return a, b
        self.assertEqual(comp_args((1, 2)), (1, 2))
        self.assertEqual(comp_args(), (3, 4))

        def comp_args(a, (b, c)):
            return a, b, c
        self.assertEqual(comp_args(1, (2, 3)), (1, 2, 3))

        def comp_args(a=2, (b, c)=(3, 4)):
            return a, b, c
        self.assertEqual(comp_args(1, (2, 3)), (1, 2, 3))
        self.assertEqual(comp_args(), (2, 3, 4))

    def test_argument_order(self):
        try:
            exec 'def f(a=1, (b, c)): pass'
            self.fail("non-default args after default")
        except SyntaxError:
            pass

    def test_float_literals(self):
        # testing bad float literals
        self.assertRaises(SyntaxError, eval, "2e")
        self.assertRaises(SyntaxError, eval, "2.0e+")
        self.assertRaises(SyntaxError, eval, "1e-")
        self.assertRaises(SyntaxError, eval, "3-4e/21")

    def test_indentation(self):
        # testing compile() of indented block w/o trailing newline"
        s = """
if 1:
    if 2:
        pass"""
        compile(s, "<string>", "exec")

    def test_literals_with_leading_zeroes(self):
        for arg in ["077787", "0xj", "0x.", "0e",  "090000000000000",
                    "080000000000000", "000000000000009", "000000000000008"]:
            self.assertRaises(SyntaxError, eval, arg)

        self.assertEqual(eval("0777"), 511)
        self.assertEqual(eval("0777L"), 511)
        self.assertEqual(eval("000777"), 511)
        self.assertEqual(eval("0xff"), 255)
        self.assertEqual(eval("0xffL"), 255)
        self.assertEqual(eval("0XfF"), 255)
        self.assertEqual(eval("0777."), 777)
        self.assertEqual(eval("0777.0"), 777)
        self.assertEqual(eval("000000000000000000000000000000000000000000000000000777e0"), 777)
        self.assertEqual(eval("0777e1"), 7770)
        self.assertEqual(eval("0e0"), 0)
        self.assertEqual(eval("0000E-012"), 0)
        self.assertEqual(eval("09.5"), 9.5)
        self.assertEqual(eval("0777j"), 777j)
        self.assertEqual(eval("00j"), 0j)
        self.assertEqual(eval("00.0"), 0)
        self.assertEqual(eval("0e3"), 0)
        self.assertEqual(eval("090000000000000."), 90000000000000.)
        self.assertEqual(eval("090000000000000.0000000000000000000000"), 90000000000000.)
        self.assertEqual(eval("090000000000000e0"), 90000000000000.)
        self.assertEqual(eval("090000000000000e-0"), 90000000000000.)
        self.assertEqual(eval("090000000000000j"), 90000000000000j)
        self.assertEqual(eval("000000000000007"), 7)
        self.assertEqual(eval("000000000000008."), 8.)
        self.assertEqual(eval("000000000000009."), 9.)

    def test_unary_minus(self):
        # Verify treatment of unary minus on negative numbers SF bug #660455
        warnings.filterwarnings("ignore", "hex/oct constants", FutureWarning)
        warnings.filterwarnings("ignore", "hex.* of negative int", FutureWarning)
        # XXX Of course the following test will have to be changed in Python 2.4
        # This test is in a <string> so the filterwarnings() can affect it
        all_one_bits = '0xffffffff'
        if sys.maxint != 2147483647:
            all_one_bits = '0xffffffffffffffff'
        self.assertEqual(eval(all_one_bits), -1)
        self.assertEqual(eval("-" + all_one_bits), 1)

    def test_sequence_unpacking_error(self):
        # Verify sequence packing/unpacking with "or".  SF bug #757818
        i,j = (1, -1) or (-1, 1)
        self.assertEqual(i, 1)
        self.assertEqual(j, -1)


def test_main():
    test_support.run_unittest(TestSpecifics)

if __name__ == "__main__":
    test_main()
