import warnings
warnings.filterwarnings("ignore", "strop functions are obsolete;",
                        DeprecationWarning,
                        r'test.test_strop|unittest')
import strop
import unittest
import sys
from test import test_support


class StropFunctionTestCase(unittest.TestCase):

    def test_atoi(self):
        self.assertTrue(strop.atoi(" 1 ") == 1)
        self.assertRaises(ValueError, strop.atoi, " 1x")
        self.assertRaises(ValueError, strop.atoi, " x1 ")

    def test_atol(self):
        self.assertTrue(strop.atol(" 1 ") == 1L)
        self.assertRaises(ValueError, strop.atol, " 1x")
        self.assertRaises(ValueError, strop.atol, " x1 ")

    def test_atof(self):
        self.assertTrue(strop.atof(" 1 ") == 1.0)
        self.assertRaises(ValueError, strop.atof, " 1x")
        self.assertRaises(ValueError, strop.atof, " x1 ")

    def test_capitalize(self):
        self.assertTrue(strop.capitalize(" hello ") == " hello ")
        self.assertTrue(strop.capitalize("hello ") == "Hello ")

    def test_find(self):
        self.assertTrue(strop.find("abcdefghiabc", "abc") == 0)
        self.assertTrue(strop.find("abcdefghiabc", "abc", 1) == 9)
        self.assertTrue(strop.find("abcdefghiabc", "def", 4) == -1)

    def test_rfind(self):
        self.assertTrue(strop.rfind("abcdefghiabc", "abc") == 9)

    def test_lower(self):
        self.assertTrue(strop.lower("HeLLo") == "hello")

    def test_upper(self):
        self.assertTrue(strop.upper("HeLLo") == "HELLO")

    def test_swapcase(self):
        self.assertTrue(strop.swapcase("HeLLo cOmpUteRs") == "hEllO CoMPuTErS")

    def test_strip(self):
        self.assertTrue(strop.strip(" \t\n hello \t\n ") == "hello")

    def test_lstrip(self):
        self.assertTrue(strop.lstrip(" \t\n hello \t\n ") == "hello \t\n ")

    def test_rstrip(self):
        self.assertTrue(strop.rstrip(" \t\n hello \t\n ") == " \t\n hello")

    def test_replace(self):
        replace = strop.replace
        self.assertTrue(replace("one!two!three!", '!', '@', 1)
                     == "one@two!three!")
        self.assertTrue(replace("one!two!three!", '!', '@', 2)
                     == "one@two@three!")
        self.assertTrue(replace("one!two!three!", '!', '@', 3)
                     == "one@two@three@")
        self.assertTrue(replace("one!two!three!", '!', '@', 4)
                     == "one@two@three@")

        # CAUTION: a replace count of 0 means infinity only to strop,
        # not to the string .replace() method or to the
        # string.replace() function.

        self.assertTrue(replace("one!two!three!", '!', '@', 0)
                     == "one@two@three@")
        self.assertTrue(replace("one!two!three!", '!', '@')
                     == "one@two@three@")
        self.assertTrue(replace("one!two!three!", 'x', '@')
                     == "one!two!three!")
        self.assertTrue(replace("one!two!three!", 'x', '@', 2)
                     == "one!two!three!")

    def test_split(self):
        split = strop.split
        self.assertTrue(split("this is the split function")
                     == ['this', 'is', 'the', 'split', 'function'])
        self.assertTrue(split("a|b|c|d", '|') == ['a', 'b', 'c', 'd'])
        self.assertTrue(split("a|b|c|d", '|', 2) == ['a', 'b', 'c|d'])
        self.assertTrue(split("a b c d", None, 1) == ['a', 'b c d'])
        self.assertTrue(split("a b c d", None, 2) == ['a', 'b', 'c d'])
        self.assertTrue(split("a b c d", None, 3) == ['a', 'b', 'c', 'd'])
        self.assertTrue(split("a b c d", None, 4) == ['a', 'b', 'c', 'd'])
        self.assertTrue(split("a b c d", None, 0) == ['a', 'b', 'c', 'd'])
        self.assertTrue(split("a  b  c  d", None, 2) ==  ['a', 'b', 'c  d'])

    def test_join(self):
        self.assertTrue(strop.join(['a', 'b', 'c', 'd']) == 'a b c d')
        self.assertTrue(strop.join(('a', 'b', 'c', 'd'), '') == 'abcd')
        self.assertTrue(strop.join(Sequence()) == 'w x y z')

        # try a few long ones
        self.assertTrue(strop.join(['x' * 100] * 100, ':')
                     == (('x' * 100) + ":") * 99 + "x" * 100)
        self.assertTrue(strop.join(('x' * 100,) * 100, ':')
                     == (('x' * 100) + ":") * 99 + "x" * 100)

    def test_maketrans(self):
        self.assertTrue(strop.maketrans("abc", "xyz") == transtable)
        self.assertRaises(ValueError, strop.maketrans, "abc", "xyzq")

    def test_translate(self):
        self.assertTrue(strop.translate("xyzabcdef", transtable, "def")
                     == "xyzxyz")

    def test_data_attributes(self):
        strop.lowercase
        strop.uppercase
        strop.whitespace

    @unittest.skipUnless(sys.maxsize == 2147483647, "only for 32-bit")
    def test_expandtabs_overflow(self):
        s = '\t\n' * 0x10000 + 'A' * 0x1000000
        self.assertRaises(OverflowError, strop.expandtabs, s, 0x10001)

    @test_support.precisionbigmemtest(size=test_support._2G - 1, memuse=5)
    def test_stropjoin_huge_list(self, size):
        a = "A" * size
        try:
            r = strop.join([a, a], a)
        except OverflowError:
            pass
        else:
            self.assertEqual(len(r), len(a) * 3)

    @test_support.precisionbigmemtest(size=test_support._2G - 1, memuse=1)
    def test_stropjoin_huge_tup(self, size):
        a = "A" * size
        try:
            r = strop.join((a, a), a)
        except OverflowError:
            pass # acceptable on 32-bit
        else:
            self.assertEqual(len(r), len(a) * 3)

transtable = '\000\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037 !"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`xyzdefghijklmnopqrstuvwxyz{|}~\177\200\201\202\203\204\205\206\207\210\211\212\213\214\215\216\217\220\221\222\223\224\225\226\227\230\231\232\233\234\235\236\237\240\241\242\243\244\245\246\247\250\251\252\253\254\255\256\257\260\261\262\263\264\265\266\267\270\271\272\273\274\275\276\277\300\301\302\303\304\305\306\307\310\311\312\313\314\315\316\317\320\321\322\323\324\325\326\327\330\331\332\333\334\335\336\337\340\341\342\343\344\345\346\347\350\351\352\353\354\355\356\357\360\361\362\363\364\365\366\367\370\371\372\373\374\375\376\377'


# join() now works with any sequence type.
class Sequence:
    def __init__(self): self.seq = 'wxyz'
    def __len__(self): return len(self.seq)
    def __getitem__(self, i): return self.seq[i]


def test_main():
    test_support.run_unittest(StropFunctionTestCase)


if __name__ == "__main__":
    test_main()
