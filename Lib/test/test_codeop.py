"""
   Test cases for codeop.py
   Nick Mathewson
"""
import unittest
from test_support import run_unittest

from codeop import compile_command

class CodeopTests(unittest.TestCase):

    def assertValid(self, str, symbol='single'):
        '''succeed iff str is a valid piece of code'''
        expected = compile(str, "<input>", symbol)
        self.assertEquals( compile_command(str, "<input>", symbol), expected)


    def assertIncomplete(self, str, symbol='single'):
        '''succeed iff str is the start of a valid piece of code'''
        self.assertEquals( compile_command(str, symbol=symbol), None)

    def assertInvalid(self, str, symbol='single', is_syntax=1):
        '''succeed iff str is the start of an invalid piece of code'''
        try:
            compile_command(str,symbol=symbol)
            self.fail("No exception thrown for invalid code")
        except SyntaxError:
            self.assert_(is_syntax)
        except OverflowError:
            self.assert_(not is_syntax)

    def test_valid(self):
        av = self.assertValid
        av("a = 1\n")
        av("def x():\n  pass\n")
        av("pass\n")
        av("3**3\n")
        av("if 9==3:\n   pass\nelse:\n   pass\n")
        av("#a\n#b\na = 3\n")
        av("#a\n\n   \na=3\n")
        av("a=3\n\n")

        # special case
        self.assertEquals(compile_command(""),
                          compile("pass", "<input>", 'single'))

        av("3**3","eval")
        av("(lambda z: \n z**3)","eval")
        av("#a\n#b\na**3","eval")

    def test_incomplete(self):
        ai = self.assertIncomplete
        ai("(a **")
        ai("def x():\n")
        ai("(a,b,")
        ai("(a,b,(")
        ai("(a,b,(")
        ai("if 9==3:\n   pass\nelse:\n")
        ai("if 9==3:\n   pass\nelse:\n   pass")
        ai("a = (")
        ai("a = 9+ \\")

        ai("(","eval")
        ai("(\n\n\n","eval")
        ai("(9+","eval")
        ai("9+ \\","eval")
        ai("lambda z: \\","eval")

    def test_invalid(self):
        ai = self.assertInvalid
        ai("a b")
        ai("a = ")
        ai("a = 9 +")

        ai("a = 1","eval")
        ai("a = (","eval")
        ai("]","eval")
        ai("())","eval")
        ai("[}","eval")
        ai("9+","eval")
        ai("lambda z:","eval")
        ai("a b","eval")

    def test_filename(self):
        self.assertEquals(compile_command("a = 1\n", "abc").co_filename,
                          compile("a = 1\n", "abc", 'single').co_filename)
        self.assertNotEquals(compile_command("a = 1\n", "abc").co_filename,
                             compile("a = 1\n", "def", 'single').co_filename)


def test_main():
    run_unittest(CodeopTests)


if __name__ == "__main__":
    test_main()
