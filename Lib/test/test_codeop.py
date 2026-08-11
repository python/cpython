"""
   Test cases for codeop.py
   Nick Mathewson
"""
import unittest
import warnings
from test.support import subTests, warnings_helper
from textwrap import dedent
import functools

from codeop import compile_command, CommandCompiler, Compile
from codeop import PyCF_DONT_IMPLY_DEDENT, PyCF_ONLY_AST
import ast


WRAPPING_COMPILERS = [compile_command, CommandCompiler()]
RAW_COMPILERS = [Compile()]
COMPILERS = WRAPPING_COMPILERS + RAW_COMPILERS


class CodeopTests(unittest.TestCase):
    def assertValid(self, str, symbol='single', *, compiler):
        '''succeed iff str is a valid piece of code'''
        expected = compile(str, "<input>", symbol, PyCF_DONT_IMPLY_DEDENT)
        self.assertEqual(compiler(str, "<input>", symbol), expected)

    def assertIncomplete(self, str, symbol='single', *, compiler):
        '''succeed iff str is the start of a valid piece of code'''
        if compiler in WRAPPING_COMPILERS:
            self.assertEqual(compiler(str, "<input>", symbol=symbol), None)
        else:
            # Compile has should raise like built-in compile
            with self.assertRaises(SyntaxError) as cm_original_error:
                compile(str, "<input>", symbol, compiler.flags)
            expected_error = cm_original_error.exception
            with self.assertRaises(type(expected_error)) as cm_wrapped_error:
                compiler(str, "<input>", symbol=symbol)
            self.assertEqual(
                expected_error.args,
                cm_wrapped_error.exception.args
            )

    def assertInvalid(self, str, symbol='single', is_syntax=1, *, compiler):
        '''succeed iff str is the start of an invalid piece of code'''
        try:
            compiler(str,"<input>", symbol=symbol)
            self.fail("No exception raised for invalid code")
        except SyntaxError:
            self.assertTrue(is_syntax)
        except OverflowError:
            self.assertTrue(not is_syntax)

    @subTests('compiler', WRAPPING_COMPILERS)
    def test_empty(self, compiler):
        self.assertEqual(
            compiler("", "<input>", 'single'),
            compile("pass", "<input>", 'single', PyCF_DONT_IMPLY_DEDENT))
        self.assertEqual(
            compiler("\n", "<input>", 'single'),
            compile("pass", "<input>", 'single', PyCF_DONT_IMPLY_DEDENT))

    @subTests('compiler', COMPILERS)
    def test_valid(self, compiler):
        av = functools.partial(self.assertValid, compiler=compiler)
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

    @subTests('compiler', COMPILERS)
    def test_incomplete(self, compiler):
        ai = functools.partial(self.assertIncomplete, compiler=compiler)

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

        ai("[")
        ai("[a")
        ai("[a,")
        ai("[a,b")
        ai("[a,b,")

        ai("{")
        ai("{a")
        ai("{a:")
        ai("{a:b")
        ai("{a:b,")
        ai("{a:b,c")
        ai("{a:b,c:")
        ai("{a:b,c:d")
        ai("{a:b,c:d,")

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

        ai('a = f"""')
        ai('a = \\')

    @subTests('compiler', COMPILERS)
    def test_invalid(self, compiler):
        ai = functools.partial(self.assertInvalid, compiler=compiler)
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

    @subTests('compiler', COMPILERS)
    def test_invalid_exec(self, compiler):
        ai = functools.partial(self.assertInvalid, compiler=compiler)
        ai("raise = 4", symbol="exec")
        ai('def a-b', symbol='exec')
        ai('await?', symbol='exec')
        ai('=!=', symbol='exec')
        ai('a await raise b', symbol='exec')
        ai('a await raise b?+1', symbol='exec')

    @subTests('compiler', COMPILERS)
    def test_filename(self, compiler):
        self.assertEqual(
            compiler("a = 1\n", "abc", "single").co_filename,
            compile("a = 1\n", "abc", 'single').co_filename
        )
        self.assertNotEqual(
            compiler("a = 1\n", "abc", "single").co_filename,
            compile("a = 1\n", "def", 'single').co_filename
        )

    def assertReturnsModule(self, code, compiler):
        retval = compiler(code, "<input>", 'exec', PyCF_ONLY_AST)
        self.assertIsInstance(retval, ast.Module)

    @subTests('compiler', RAW_COMPILERS)
    def test_ast_return_value(self, compiler):
        validate_ast = self.assertReturnsModule
        validate_ast("x = 5", compiler)
        validate_ast("\nx = 5", compiler)
        validate_ast("x = 5\n", compiler)
        validate_ast("x = 5\n\n", compiler)
        validate_ast("\n\nx = 5\n\n", compiler)

    @subTests('compiler', COMPILERS)
    def test_warning(self, compiler):
        # Test that the warning is only returned once.
        with warnings_helper.check_warnings(
                ('"is" with \'str\' literal', SyntaxWarning),
                ('"\\\\e" is an invalid escape sequence', SyntaxWarning),
                ) as w:
            compiler(r"'\e' is 0", "<input>", "single")
        self.assertEqual(len(w.warnings), 2)

        # bpo-41520: check SyntaxWarning treated as an SyntaxError
        with warnings.catch_warnings(), self.assertRaises(SyntaxError):
            warnings.simplefilter('error', SyntaxWarning)
            compiler('1 is 1', "<input>", 'exec')

        # Check SyntaxWarning treated as an SyntaxError
        with warnings.catch_warnings(), self.assertRaises(SyntaxError):
            warnings.simplefilter('error', SyntaxWarning)
            compiler(r"'\e'", "<input>", 'exec')

    @subTests('compiler', WRAPPING_COMPILERS)
    def test_incomplete_warning(self, compiler):
        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter('always')
        compiler("'\\e' + (")
        self.assertEqual(w, [])

    @subTests('compiler', RAW_COMPILERS)
    def test_raw_raises_error(self, compiler):
        warnings_cm = warnings_helper.check_warnings(
            ('"\\\\e" is an invalid esceape sequence', SyntaxWarning)
        )
        with self.assertRaises(SyntaxError), warnings_cm as w:
            compiler("'\\e' + (", "<input>", 'single')
        self.assertEqual(len(w.warnings), 1)

    @subTests('compiler', COMPILERS)
    def test_invalid_warning(self, compiler):
        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter('always')
            self.assertInvalid("'\\e' 1", compiler=compiler)
        self.assertEqual(len(w), 1)
        for warning in w:
            self.assertEqual(warning.category, SyntaxWarning)
            self.assertRegex(str(warning), 'invalid escape sequence')
            self.assertEqual(warning.filename, '<input>')

    @subTests('compiler', COMPILERS)
    def test_syntax_errors(self, compiler):
        code = dedent("""\
                def foo(x,x):
                   pass
            """)
        message = "duplicate parameter 'x' in function definition"
        with self.assertRaisesRegex(SyntaxError, message):
            compiler(code, "<input>", 'exec')

    @subTests('compiler', RAW_COMPILERS)
    def test_future_imports(self, compiler):
        original_flags = compiler.flags
        compiler('from __future__ import annotations', "<input>", 'single')
        self.assertGreater(compiler.flags, original_flags)
        # reset flags to ensure test has no side-effects
        compiler.flags = original_flags


if __name__ == "__main__":
    unittest.main()
