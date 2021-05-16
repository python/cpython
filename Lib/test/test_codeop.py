"""
   Test cases for codeop.py
   Nick Mathewson
"""
import sys
import unittest
import warnings
from test import support
from test.support import warnings_helper

from codeop import compile_command, PyCF_DONT_IMPLY_DEDENT
import io

if support.is_jython:

    def unify_callables(d):
        for n,v in d.items():
            if hasattr(v, '__call__'):
                d[n] = True
        return d

class CodeopTests(unittest.TestCase):

    def assertValid(self, str, symbol='single'):
        '''succeed iff str is a valid piece of code'''
        if support.is_jython:
            code = compile_command(str, "<input>", symbol)
            self.assertTrue(code)
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
            self.assertEqual(unify_callables(r),unify_callables(d))
        else:
            expected = compile(str, "<input>", symbol, PyCF_DONT_IMPLY_DEDENT)
            self.assertEqual(compile_command(str, "<input>", symbol), expected)

    def assertIncomplete(self, str, symbol='single'):
        '''succeed iff str is the start of a valid piece of code'''
        self.assertEqual(compile_command(str, symbol=symbol), None)

    def assertInvalid(self, str, symbol='single', is_syntax=1):
        '''succeed iff str is the start of an invalid piece of code'''
        try:
            compile_command(str,symbol=symbol)
            self.fail("No exception raised for invalid code")
        except SyntaxError:
            self.assertTrue(is_syntax)
        except OverflowError:
            self.assertTrue(not is_syntax)

    def test_valid(self):
        av = self.assertValid

        # special case
        if not support.is_jython:
            self.assertEqual(compile_command(""),
                             compile("pass", "<input>", 'single',
                                     PyCF_DONT_IMPLY_DEDENT))
            self.assertEqual(compile_command("\n"),
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

        av("\n\na = 1\n\n")
        av("\n\nif 1: a=1\n\n")

        av("if 1:\n pass\n if 1:\n  pass\n else:\n  pass\n")
        av("#a\n\n   \na=3\n\n")

        av("\n\na**3","eval")
        av("\n \na**3","eval")
        av("#a\n#b\na**3","eval")

        av("def f():\n try: pass\n finally: [x for x in (1,2)]\n")
        av("def f():\n pass\n#foo\n")
        av("@a.b.c\ndef f():\n pass\n")

    def test_incomplete(self):
        ai = self.assertIncomplete

        ai("(a **")
        ai("(a,b,")
        ai("(a,b,(")
        ai("(a,b,(")
        ai("a = (")
        ai("a = {")
        ai("b + {")

        ai("print([1,\n2,")
        ai("print({1:1,\n2:3,")
        ai("print((1,\n2,")

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
        ai("(9+","eval")
        ai("9+ \\","eval")
        ai("lambda z: \\","eval")

        ai("if True:\n if True:\n  if True:   \n")

        ai("@a(")
        ai("@a(b")
        ai("@a(b,")
        ai("@a(b,c")
        ai("@a(b,c,")

        ai("from a import (")
        ai("from a import (b")
        ai("from a import (b,")
        ai("from a import (b,c")
        ai("from a import (b,c,")

        ai("[");
        ai("[a");
        ai("[a,");
        ai("[a,b");
        ai("[a,b,");

        ai("{");
        ai("{a");
        ai("{a:");
        ai("{a:b");
        ai("{a:b,");
        ai("{a:b,c");
        ai("{a:b,c:");
        ai("{a:b,c:d");
        ai("{a:b,c:d,");

        ai("a(")
        ai("a(b")
        ai("a(b,")
        ai("a(b,c")
        ai("a(b,c,")

        ai("a[")
        ai("a[b")
        ai("a[b,")
        ai("a[b:")
        ai("a[b:c")
        ai("a[b:c:")
        ai("a[b:c:d")

        ai("def a(")
        ai("def a(b")
        ai("def a(b,")
        ai("def a(b,c")
        ai("def a(b,c,")

        ai("(")
        ai("(a")
        ai("(a,")
        ai("(a,b")
        ai("(a,b,")

        ai("if a:\n pass\nelif b:")
        ai("if a:\n pass\nelif b:\n pass\nelse:")

        ai("while a:")
        ai("while a:\n pass\nelse:")

        ai("for a in b:")
        ai("for a in b:\n pass\nelse:")

        ai("try:")
        ai("try:\n pass\nexcept:")
        ai("try:\n pass\nfinally:")
        ai("try:\n pass\nexcept:\n pass\nfinally:")

        ai("with a:")
        ai("with a as b:")

        ai("class a:")
        ai("class a(")
        ai("class a(b")
        ai("class a(b,")
        ai("class a():")

        ai("[x for")
        ai("[x for x in")
        ai("[x for x in (")

        ai("(x for")
        ai("(x for x in")
        ai("(x for x in (")

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
        ai("]","eval")
        ai("())","eval")
        ai("[}","eval")
        ai("9+","eval")
        ai("lambda z:","eval")
        ai("a b","eval")

        ai("return 2.3")
        ai("if (a == 1 and b = 2): pass")

        ai("del 1")
        ai("del (1,)")
        ai("del [1]")
        ai("del '1'")

        ai("[i for i in range(10)] = (1, 2, 3)")

    def test_invalid_exec(self):
        ai = self.assertInvalid
        ai("raise = 4", symbol="exec")
        ai('def a-b', symbol='exec')
        ai('await?', symbol='exec')
        ai('=!=', symbol='exec')
        ai('a await raise b', symbol='exec')
        ai('a await raise b?+1', symbol='exec')

    def test_filename(self):
        self.assertEqual(compile_command("a = 1\n", "abc").co_filename,
                         compile("a = 1\n", "abc", 'single').co_filename)
        self.assertNotEqual(compile_command("a = 1\n", "abc").co_filename,
                            compile("a = 1\n", "def", 'single').co_filename)

    def test_warning(self):
        # Test that the warning is only returned once.
        with warnings_helper.check_warnings(
                (".*literal", SyntaxWarning),
                (".*invalid", DeprecationWarning),
                ) as w:
            compile_command(r"'\e' is 0")
            self.assertEqual(len(w.warnings), 2)

        # bpo-41520: check SyntaxWarning treated as an SyntaxError
        with warnings.catch_warnings(), self.assertRaises(SyntaxError):
            warnings.simplefilter('error', SyntaxWarning)
            compile_command('1 is 1', symbol='exec')


if __name__ == "__main__":
    unittest.main()
