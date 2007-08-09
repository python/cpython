"""
   Test cases for codeop.py
   Nick Mathewson
"""
import unittest
from test.test_support import run_unittest, is_jython

from codeop import compile_command, PyCF_DONT_IMPLY_DEDENT
import io

if is_jython:
    import sys

    def unify_callables(d):
        for n,v in d.items():
            if hasattr(v, '__call__'):
                d[n] = True
        return d

class CodeopTests(unittest.TestCase):

    def assertValid(self, str, symbol='single'):
        '''succeed iff str is a valid piece of code'''
        if is_jython:
            code = compile_command(str, "<input>", symbol)
            self.assert_(code)
            if symbol == "single":
                d,r = {},{}
                saved_stdout = sys.stdout
                sys.stdout = io.StringIO()
                try:
                    exec(code, d)
                    exec(compile(str,"<input>","single"), r)
                finally:
                    sys.stdout = saved_stdout
            elif symbol == 'eval':
                ctx = {'a': 2}
                d = { 'value': eval(code,ctx) }
                r = { 'value': eval(str,ctx) }
            self.assertEquals(unify_callables(r),unify_callables(d))
        else:
            expected = compile(str, "<input>", symbol, PyCF_DONT_IMPLY_DEDENT)
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

        # special case
        if not is_jython:
            self.assertEquals(compile_command(""),
                            compile("pass", "<input>", 'single',
                                    PyCF_DONT_IMPLY_DEDENT))
            self.assertEquals(compile_command("\n"),
                            compile("pass", "<input>", 'single',
                                    PyCF_DONT_IMPLY_DEDENT))
        else:
            av("")
            av("\n")

        av("a = 1")
        av("\na = 1")
        av("a = 1\n")
        av("a = 1\n\n")
        av("\n\na = 1\n\n")

        av("def x():\n  pass\n")
        av("if 1:\n pass\n")

        av("\n\nif 1: pass\n")
        av("\n\nif 1: pass\n\n")

        av("def x():\n\n pass\n")
        av("def x():\n  pass\n  \n")
        av("def x():\n  pass\n \n")

        av("pass\n")
        av("3**3\n")

        av("if 9==3:\n   pass\nelse:\n   pass\n")
        av("if 1:\n pass\n if 1:\n  pass\n else:\n  pass\n")

        av("#a\n#b\na = 3\n")
        av("#a\n\n   \na=3\n")
        av("a=3\n\n")
        av("a = 9+ \\\n3")

        av("3**3","eval")
        av("(lambda z: \n z**3)","eval")

        av("9+ \\\n3","eval")
        av("9+ \\\n3\n","eval")

        av("\n\na**3","eval")
        av("\n \na**3","eval")
        av("#a\n#b\na**3","eval")

    def test_incomplete(self):
        ai = self.assertIncomplete

        ai("(a **")
        ai("(a,b,")
        ai("(a,b,(")
        ai("(a,b,(")
        ai("a = (")
        ai("a = {")
        ai("b + {")

        ai("if 9==3:\n   pass\nelse:")
        ai("if 9==3:\n   pass\nelse:\n")
        ai("if 9==3:\n   pass\nelse:\n   pass")
        ai("if 1:")
        ai("if 1:\n")
        ai("if 1:\n pass\n if 1:\n  pass\n else:")
        ai("if 1:\n pass\n if 1:\n  pass\n else:\n")
        ai("if 1:\n pass\n if 1:\n  pass\n else:\n  pass")

        ai("def x():")
        ai("def x():\n")
        ai("def x():\n\n")

        ai("def x():\n  pass")
        ai("def x():\n  pass\n ")
        ai("def x():\n  pass\n  ")
        ai("\n\ndef x():\n  pass")

        ai("a = 9+ \\")
        ai("a = 'a\\")
        ai("a = '''xy")

        ai("","eval")
        ai("\n","eval")
        ai("(","eval")
        ai("(\n\n\n","eval")
        ai("(9+","eval")
        ai("9+ \\","eval")
        ai("lambda z: \\","eval")

    def test_invalid(self):
        ai = self.assertInvalid
        ai("a b")

        ai("a @")
        ai("a b @")
        ai("a ** @")

        ai("a = ")
        ai("a = 9 +")

        ai("def x():\n\npass\n")

        ai("\n\n if 1: pass\n\npass")

        ai("a = 9+ \\\n")
        ai("a = 'a\\ ")
        ai("a = 'a\\\n")

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
