# -*- coding: utf-8 -*-
# There are tests here with unicode string literals and
# identifiers. There's a code in ast.c that was added because of a
# failure with a non-ascii-only expression.  So, I have tests for
# that.  There are workarounds that would let me run tests for that
# code without unicode identifiers and strings, but just using them
# directly seems like the easiest and therefore safest thing to do.
# Unicode identifiers in tests is allowed by PEP 3131.

import ast
import os
import re
import types
import decimal
import unittest
from test.support.os_helper import temp_cwd
from test.support.script_helper import assert_python_failure

a_global = 'global variable'

# You could argue that I'm too strict in looking for specific error
#  values with assertRaisesRegex, but without it it's way too easy to
#  make a syntax error in the test strings. Especially with all of the
#  triple quotes, raw strings, backslashes, etc. I think it's a
#  worthwhile tradeoff. When I switched to this method, I found many
#  examples where I wasn't testing what I thought I was.

class TestCase(unittest.TestCase):
    def assertAllRaise(self, exception_type, regex, error_strings):
        for str in error_strings:
            with self.subTest(str=str):
                with self.assertRaisesRegex(exception_type, regex):
                    eval(str)

    def test__format__lookup(self):
        # Make sure __format__ is looked up on the type, not the instance.
        class X:
            def __format__(self, spec):
                return 'class'

        x = X()

        # Add a bound __format__ method to the 'y' instance, but not
        #  the 'x' instance.
        y = X()
        y.__format__ = types.MethodType(lambda self, spec: 'instance', y)

        self.assertEqual(f'{y}', format(y))
        self.assertEqual(f'{y}', 'class')
        self.assertEqual(format(x), format(y))

        # __format__ is not called this way, but still make sure it
        #  returns what we expect (so we can make sure we're bypassing
        #  it).
        self.assertEqual(x.__format__(''), 'class')
        self.assertEqual(y.__format__(''), 'instance')

        # This is how __format__ is actually called.
        self.assertEqual(type(x).__format__(x, ''), 'class')
        self.assertEqual(type(y).__format__(y, ''), 'class')

    def test_ast(self):
        # Inspired by http://bugs.python.org/issue24975
        class X:
            def __init__(self):
                self.called = False
            def __call__(self):
                self.called = True
                return 4
        x = X()
        expr = """
a = 10
f'{a * x()}'"""
        t = ast.parse(expr)
        c = compile(t, '', 'exec')

        # Make sure x was not called.
        self.assertFalse(x.called)

        # Actually run the code.
        exec(c)

        # Make sure x was called.
        self.assertTrue(x.called)

    def test_ast_line_numbers(self):
        expr = """
a = 10
f'{a * x()}'"""
        t = ast.parse(expr)
        self.assertEqual(type(t), ast.Module)
        self.assertEqual(len(t.body), 2)
        # check `a = 10`
        self.assertEqual(type(t.body[0]), ast.Assign)
        self.assertEqual(t.body[0].lineno, 2)
        # check `f'...'`
        self.assertEqual(type(t.body[1]), ast.Expr)
        self.assertEqual(type(t.body[1].value), ast.JoinedStr)
        self.assertEqual(len(t.body[1].value.values), 1)
        self.assertEqual(type(t.body[1].value.values[0]), ast.FormattedValue)
        self.assertEqual(t.body[1].lineno, 3)
        self.assertEqual(t.body[1].value.lineno, 3)
        self.assertEqual(t.body[1].value.values[0].lineno, 3)
        # check the binop location
        binop = t.body[1].value.values[0].value
        self.assertEqual(type(binop), ast.BinOp)
        self.assertEqual(type(binop.left), ast.Name)
        self.assertEqual(type(binop.op), ast.Mult)
        self.assertEqual(type(binop.right), ast.Call)
        self.assertEqual(binop.lineno, 3)
        self.assertEqual(binop.left.lineno, 3)
        self.assertEqual(binop.right.lineno, 3)
        self.assertEqual(binop.col_offset, 3)
        self.assertEqual(binop.left.col_offset, 3)
        self.assertEqual(binop.right.col_offset, 7)

    def test_ast_line_numbers_multiple_formattedvalues(self):
        expr = """
f'no formatted values'
f'eggs {a * x()} spam {b + y()}'"""
        t = ast.parse(expr)
        self.assertEqual(type(t), ast.Module)
        self.assertEqual(len(t.body), 2)
        # check `f'no formatted value'`
        self.assertEqual(type(t.body[0]), ast.Expr)
        self.assertEqual(type(t.body[0].value), ast.JoinedStr)
        self.assertEqual(t.body[0].lineno, 2)
        # check `f'...'`
        self.assertEqual(type(t.body[1]), ast.Expr)
        self.assertEqual(type(t.body[1].value), ast.JoinedStr)
        self.assertEqual(len(t.body[1].value.values), 4)
        self.assertEqual(type(t.body[1].value.values[0]), ast.Constant)
        self.assertEqual(type(t.body[1].value.values[0].value), str)
        self.assertEqual(type(t.body[1].value.values[1]), ast.FormattedValue)
        self.assertEqual(type(t.body[1].value.values[2]), ast.Constant)
        self.assertEqual(type(t.body[1].value.values[2].value), str)
        self.assertEqual(type(t.body[1].value.values[3]), ast.FormattedValue)
        self.assertEqual(t.body[1].lineno, 3)
        self.assertEqual(t.body[1].value.lineno, 3)
        self.assertEqual(t.body[1].value.values[0].lineno, 3)
        self.assertEqual(t.body[1].value.values[1].lineno, 3)
        self.assertEqual(t.body[1].value.values[2].lineno, 3)
        self.assertEqual(t.body[1].value.values[3].lineno, 3)
        # check the first binop location
        binop1 = t.body[1].value.values[1].value
        self.assertEqual(type(binop1), ast.BinOp)
        self.assertEqual(type(binop1.left), ast.Name)
        self.assertEqual(type(binop1.op), ast.Mult)
        self.assertEqual(type(binop1.right), ast.Call)
        self.assertEqual(binop1.lineno, 3)
        self.assertEqual(binop1.left.lineno, 3)
        self.assertEqual(binop1.right.lineno, 3)
        self.assertEqual(binop1.col_offset, 8)
        self.assertEqual(binop1.left.col_offset, 8)
        self.assertEqual(binop1.right.col_offset, 12)
        # check the second binop location
        binop2 = t.body[1].value.values[3].value
        self.assertEqual(type(binop2), ast.BinOp)
        self.assertEqual(type(binop2.left), ast.Name)
        self.assertEqual(type(binop2.op), ast.Add)
        self.assertEqual(type(binop2.right), ast.Call)
        self.assertEqual(binop2.lineno, 3)
        self.assertEqual(binop2.left.lineno, 3)
        self.assertEqual(binop2.right.lineno, 3)
        self.assertEqual(binop2.col_offset, 23)
        self.assertEqual(binop2.left.col_offset, 23)
        self.assertEqual(binop2.right.col_offset, 27)

    def test_ast_line_numbers_nested(self):
        expr = """
a = 10
f'{a * f"-{x()}-"}'"""
        t = ast.parse(expr)
        self.assertEqual(type(t), ast.Module)
        self.assertEqual(len(t.body), 2)
        # check `a = 10`
        self.assertEqual(type(t.body[0]), ast.Assign)
        self.assertEqual(t.body[0].lineno, 2)
        # check `f'...'`
        self.assertEqual(type(t.body[1]), ast.Expr)
        self.assertEqual(type(t.body[1].value), ast.JoinedStr)
        self.assertEqual(len(t.body[1].value.values), 1)
        self.assertEqual(type(t.body[1].value.values[0]), ast.FormattedValue)
        self.assertEqual(t.body[1].lineno, 3)
        self.assertEqual(t.body[1].value.lineno, 3)
        self.assertEqual(t.body[1].value.values[0].lineno, 3)
        # check the binop location
        binop = t.body[1].value.values[0].value
        self.assertEqual(type(binop), ast.BinOp)
        self.assertEqual(type(binop.left), ast.Name)
        self.assertEqual(type(binop.op), ast.Mult)
        self.assertEqual(type(binop.right), ast.JoinedStr)
        self.assertEqual(binop.lineno, 3)
        self.assertEqual(binop.left.lineno, 3)
        self.assertEqual(binop.right.lineno, 3)
        self.assertEqual(binop.col_offset, 3)
        self.assertEqual(binop.left.col_offset, 3)
        self.assertEqual(binop.right.col_offset, 7)
        # check the nested call location
        self.assertEqual(len(binop.right.values), 3)
        self.assertEqual(type(binop.right.values[0]), ast.Constant)
        self.assertEqual(type(binop.right.values[0].value), str)
        self.assertEqual(type(binop.right.values[1]), ast.FormattedValue)
        self.assertEqual(type(binop.right.values[2]), ast.Constant)
        self.assertEqual(type(binop.right.values[2].value), str)
        self.assertEqual(binop.right.values[0].lineno, 3)
        self.assertEqual(binop.right.values[1].lineno, 3)
        self.assertEqual(binop.right.values[2].lineno, 3)
        call = binop.right.values[1].value
        self.assertEqual(type(call), ast.Call)
        self.assertEqual(call.lineno, 3)
        self.assertEqual(call.col_offset, 11)

    def test_ast_line_numbers_duplicate_expression(self):
        expr = """
a = 10
f'{a * x()} {a * x()} {a * x()}'
"""
        t = ast.parse(expr)
        self.assertEqual(type(t), ast.Module)
        self.assertEqual(len(t.body), 2)
        # check `a = 10`
        self.assertEqual(type(t.body[0]), ast.Assign)
        self.assertEqual(t.body[0].lineno, 2)
        # check `f'...'`
        self.assertEqual(type(t.body[1]), ast.Expr)
        self.assertEqual(type(t.body[1].value), ast.JoinedStr)
        self.assertEqual(len(t.body[1].value.values), 5)
        self.assertEqual(type(t.body[1].value.values[0]), ast.FormattedValue)
        self.assertEqual(type(t.body[1].value.values[1]), ast.Constant)
        self.assertEqual(type(t.body[1].value.values[1].value), str)
        self.assertEqual(type(t.body[1].value.values[2]), ast.FormattedValue)
        self.assertEqual(type(t.body[1].value.values[3]), ast.Constant)
        self.assertEqual(type(t.body[1].value.values[3].value), str)
        self.assertEqual(type(t.body[1].value.values[4]), ast.FormattedValue)
        self.assertEqual(t.body[1].lineno, 3)
        self.assertEqual(t.body[1].value.lineno, 3)
        self.assertEqual(t.body[1].value.values[0].lineno, 3)
        self.assertEqual(t.body[1].value.values[1].lineno, 3)
        self.assertEqual(t.body[1].value.values[2].lineno, 3)
        self.assertEqual(t.body[1].value.values[3].lineno, 3)
        self.assertEqual(t.body[1].value.values[4].lineno, 3)
        # check the first binop location
        binop = t.body[1].value.values[0].value
        self.assertEqual(type(binop), ast.BinOp)
        self.assertEqual(type(binop.left), ast.Name)
        self.assertEqual(type(binop.op), ast.Mult)
        self.assertEqual(type(binop.right), ast.Call)
        self.assertEqual(binop.lineno, 3)
        self.assertEqual(binop.left.lineno, 3)
        self.assertEqual(binop.right.lineno, 3)
        self.assertEqual(binop.col_offset, 3)
        self.assertEqual(binop.left.col_offset, 3)
        self.assertEqual(binop.right.col_offset, 7)
        # check the second binop location
        binop = t.body[1].value.values[2].value
        self.assertEqual(type(binop), ast.BinOp)
        self.assertEqual(type(binop.left), ast.Name)
        self.assertEqual(type(binop.op), ast.Mult)
        self.assertEqual(type(binop.right), ast.Call)
        self.assertEqual(binop.lineno, 3)
        self.assertEqual(binop.left.lineno, 3)
        self.assertEqual(binop.right.lineno, 3)
        self.assertEqual(binop.col_offset, 13)
        self.assertEqual(binop.left.col_offset, 13)
        self.assertEqual(binop.right.col_offset, 17)
        # check the third binop location
        binop = t.body[1].value.values[4].value
        self.assertEqual(type(binop), ast.BinOp)
        self.assertEqual(type(binop.left), ast.Name)
        self.assertEqual(type(binop.op), ast.Mult)
        self.assertEqual(type(binop.right), ast.Call)
        self.assertEqual(binop.lineno, 3)
        self.assertEqual(binop.left.lineno, 3)
        self.assertEqual(binop.right.lineno, 3)
        self.assertEqual(binop.col_offset, 23)
        self.assertEqual(binop.left.col_offset, 23)
        self.assertEqual(binop.right.col_offset, 27)

    def test_ast_numbers_fstring_with_formatting(self):

        t = ast.parse('f"Here is that pesky {xxx:.3f} again"')
        self.assertEqual(len(t.body), 1)
        self.assertEqual(t.body[0].lineno, 1)

        self.assertEqual(type(t.body[0]), ast.Expr)
        self.assertEqual(type(t.body[0].value), ast.JoinedStr)
        self.assertEqual(len(t.body[0].value.values), 3)

        self.assertEqual(type(t.body[0].value.values[0]), ast.Constant)
        self.assertEqual(type(t.body[0].value.values[1]), ast.FormattedValue)
        self.assertEqual(type(t.body[0].value.values[2]), ast.Constant)

        _, expr, _ = t.body[0].value.values

        name = expr.value
        self.assertEqual(type(name), ast.Name)
        self.assertEqual(name.lineno, 1)
        self.assertEqual(name.end_lineno, 1)
        self.assertEqual(name.col_offset, 22)
        self.assertEqual(name.end_col_offset, 25)

    def test_ast_line_numbers_multiline_fstring(self):
        # See bpo-30465 for details.
        expr = """
a = 10
f'''
  {a
     *
       x()}
non-important content
'''
"""
        t = ast.parse(expr)
        self.assertEqual(type(t), ast.Module)
        self.assertEqual(len(t.body), 2)
        # check `a = 10`
        self.assertEqual(type(t.body[0]), ast.Assign)
        self.assertEqual(t.body[0].lineno, 2)
        # check `f'...'`
        self.assertEqual(type(t.body[1]), ast.Expr)
        self.assertEqual(type(t.body[1].value), ast.JoinedStr)
        self.assertEqual(len(t.body[1].value.values), 3)
        self.assertEqual(type(t.body[1].value.values[0]), ast.Constant)
        self.assertEqual(type(t.body[1].value.values[0].value), str)
        self.assertEqual(type(t.body[1].value.values[1]), ast.FormattedValue)
        self.assertEqual(type(t.body[1].value.values[2]), ast.Constant)
        self.assertEqual(type(t.body[1].value.values[2].value), str)
        self.assertEqual(t.body[1].lineno, 3)
        self.assertEqual(t.body[1].value.lineno, 3)
        self.assertEqual(t.body[1].value.values[0].lineno, 3)
        self.assertEqual(t.body[1].value.values[1].lineno, 3)
        self.assertEqual(t.body[1].value.values[2].lineno, 3)
        self.assertEqual(t.body[1].col_offset, 0)
        self.assertEqual(t.body[1].value.col_offset, 0)
        self.assertEqual(t.body[1].value.values[0].col_offset, 0)
        self.assertEqual(t.body[1].value.values[1].col_offset, 0)
        self.assertEqual(t.body[1].value.values[2].col_offset, 0)
        # NOTE: the following lineno information and col_offset is correct for
        # expressions within FormattedValues.
        binop = t.body[1].value.values[1].value
        self.assertEqual(type(binop), ast.BinOp)
        self.assertEqual(type(binop.left), ast.Name)
        self.assertEqual(type(binop.op), ast.Mult)
        self.assertEqual(type(binop.right), ast.Call)
        self.assertEqual(binop.lineno, 4)
        self.assertEqual(binop.left.lineno, 4)
        self.assertEqual(binop.right.lineno, 6)
        self.assertEqual(binop.col_offset, 4)
        self.assertEqual(binop.left.col_offset, 4)
        self.assertEqual(binop.right.col_offset, 7)

    def test_ast_line_numbers_with_parentheses(self):
        expr = """
x = (
    f" {test(t)}"
)"""
        t = ast.parse(expr)
        self.assertEqual(type(t), ast.Module)
        self.assertEqual(len(t.body), 1)
        # check the test(t) location
        call = t.body[0].value.values[1].value
        self.assertEqual(type(call), ast.Call)
        self.assertEqual(call.lineno, 3)
        self.assertEqual(call.end_lineno, 3)
        self.assertEqual(call.col_offset, 8)
        self.assertEqual(call.end_col_offset, 15)

        expr = """
x = (
        'PERL_MM_OPT', (
            f'wat'
            f'some_string={f(x)} '
            f'wat'
        ),
)
"""
        t = ast.parse(expr)
        self.assertEqual(type(t), ast.Module)
        self.assertEqual(len(t.body), 1)
        # check the fstring
        fstring = t.body[0].value.elts[1]
        self.assertEqual(type(fstring), ast.JoinedStr)
        self.assertEqual(len(fstring.values), 3)
        wat1, middle, wat2 = fstring.values
        # check the first wat
        self.assertEqual(type(wat1), ast.Constant)
        self.assertEqual(wat1.lineno, 4)
        self.assertEqual(wat1.end_lineno, 6)
        self.assertEqual(wat1.col_offset, 12)
        self.assertEqual(wat1.end_col_offset, 18)
        # check the call
        call = middle.value
        self.assertEqual(type(call), ast.Call)
        self.assertEqual(call.lineno, 5)
        self.assertEqual(call.end_lineno, 5)
        self.assertEqual(call.col_offset, 27)
        self.assertEqual(call.end_col_offset, 31)
        # check the second wat
        self.assertEqual(type(wat2), ast.Constant)
        self.assertEqual(wat2.lineno, 4)
        self.assertEqual(wat2.end_lineno, 6)
        self.assertEqual(wat2.col_offset, 12)
        self.assertEqual(wat2.end_col_offset, 18)

    def test_docstring(self):
        def f():
            f'''Not a docstring'''
        self.assertIsNone(f.__doc__)
        def g():
            '''Not a docstring''' \
            f''
        self.assertIsNone(g.__doc__)

    def test_literal_eval(self):
        with self.assertRaisesRegex(ValueError, 'malformed node or string'):
            ast.literal_eval("f'x'")

    def test_ast_compile_time_concat(self):
        x = ['']

        expr = """x[0] = 'foo' f'{3}'"""
        t = ast.parse(expr)
        c = compile(t, '', 'exec')
        exec(c)
        self.assertEqual(x[0], 'foo3')

    def test_compile_time_concat_errors(self):
        self.assertAllRaise(SyntaxError,
                            'cannot mix bytes and nonbytes literals',
                            [r"""f'' b''""",
                             r"""b'' f''""",
                             ])

    def test_literal(self):
        self.assertEqual(f'', '')
        self.assertEqual(f'a', 'a')
        self.assertEqual(f' ', ' ')

    def test_unterminated_string(self):
        self.assertAllRaise(SyntaxError, 'f-string: unterminated string',
                            [r"""f'{"x'""",
                             r"""f'{"x}'""",
                             r"""f'{("x'""",
                             r"""f'{("x}'""",
                             ])

    def test_mismatched_parens(self):
        self.assertAllRaise(SyntaxError, r"f-string: closing parenthesis '\}' "
                            r"does not match opening parenthesis '\('",
                            ["f'{((}'",
                             ])
        self.assertAllRaise(SyntaxError, r"f-string: closing parenthesis '\)' "
                            r"does not match opening parenthesis '\['",
                            ["f'{a[4)}'",
                            ])
        self.assertAllRaise(SyntaxError, r"f-string: closing parenthesis '\]' "
                            r"does not match opening parenthesis '\('",
                            ["f'{a(4]}'",
                            ])
        self.assertAllRaise(SyntaxError, r"f-string: closing parenthesis '\}' "
                            r"does not match opening parenthesis '\['",
                            ["f'{a[4}'",
                            ])
        self.assertAllRaise(SyntaxError, r"f-string: closing parenthesis '\}' "
                            r"does not match opening parenthesis '\('",
                            ["f'{a(4}'",
                            ])
        self.assertRaises(SyntaxError, eval, "f'{" + "("*500 + "}'")

    def test_double_braces(self):
        self.assertEqual(f'{{', '{')
        self.assertEqual(f'a{{', 'a{')
        self.assertEqual(f'{{b', '{b')
        self.assertEqual(f'a{{b', 'a{b')
        self.assertEqual(f'}}', '}')
        self.assertEqual(f'a}}', 'a}')
        self.assertEqual(f'}}b', '}b')
        self.assertEqual(f'a}}b', 'a}b')
        self.assertEqual(f'{{}}', '{}')
        self.assertEqual(f'a{{}}', 'a{}')
        self.assertEqual(f'{{b}}', '{b}')
        self.assertEqual(f'{{}}c', '{}c')
        self.assertEqual(f'a{{b}}', 'a{b}')
        self.assertEqual(f'a{{}}c', 'a{}c')
        self.assertEqual(f'{{b}}c', '{b}c')
        self.assertEqual(f'a{{b}}c', 'a{b}c')

        self.assertEqual(f'{{{10}', '{10')
        self.assertEqual(f'}}{10}', '}10')
        self.assertEqual(f'}}{{{10}', '}{10')
        self.assertEqual(f'}}a{{{10}', '}a{10')

        self.assertEqual(f'{10}{{', '10{')
        self.assertEqual(f'{10}}}', '10}')
        self.assertEqual(f'{10}}}{{', '10}{')
        self.assertEqual(f'{10}}}a{{' '}', '10}a{}')

        # Inside of strings, don't interpret doubled brackets.
        self.assertEqual(f'{"{{}}"}', '{{}}')

        self.assertAllRaise(TypeError, 'unhashable type',
                            ["f'{ {{}} }'", # dict in a set
                             ])

    def test_compile_time_concat(self):
        x = 'def'
        self.assertEqual('abc' f'## {x}ghi', 'abc## defghi')
        self.assertEqual('abc' f'{x}' 'ghi', 'abcdefghi')
        self.assertEqual('abc' f'{x}' 'gh' f'i{x:4}', 'abcdefghidef ')
        self.assertEqual('{x}' f'{x}', '{x}def')
        self.assertEqual('{x' f'{x}', '{xdef')
        self.assertEqual('{x}' f'{x}', '{x}def')
        self.assertEqual('{{x}}' f'{x}', '{{x}}def')
        self.assertEqual('{{x' f'{x}', '{{xdef')
        self.assertEqual('x}}' f'{x}', 'x}}def')
        self.assertEqual(f'{x}' 'x}}', 'defx}}')
        self.assertEqual(f'{x}' '', 'def')
        self.assertEqual('' f'{x}' '', 'def')
        self.assertEqual('' f'{x}', 'def')
        self.assertEqual(f'{x}' '2', 'def2')
        self.assertEqual('1' f'{x}' '2', '1def2')
        self.assertEqual('1' f'{x}', '1def')
        self.assertEqual(f'{x}' f'-{x}', 'def-def')
        self.assertEqual('' f'', '')
        self.assertEqual('' f'' '', '')
        self.assertEqual('' f'' '' f'', '')
        self.assertEqual(f'', '')
        self.assertEqual(f'' '', '')
        self.assertEqual(f'' '' f'', '')
        self.assertEqual(f'' '' f'' '', '')

        self.assertAllRaise(SyntaxError, "f-string: expecting '}'",
                            ["f'{3' f'}'",  # can't concat to get a valid f-string
                             ])

    def test_comments(self):
        # These aren't comments, since they're in strings.
        d = {'#': 'hash'}
        self.assertEqual(f'{"#"}', '#')
        self.assertEqual(f'{d["#"]}', 'hash')

        self.assertAllRaise(SyntaxError, "f-string expression part cannot include '#'",
                            ["f'{1#}'",   # error because the expression becomes "(1#)"
                             "f'{3(#)}'",
                             "f'{#}'",
                             ])
        self.assertAllRaise(SyntaxError, r"f-string: unmatched '\)'",
                            ["f'{)#}'",   # When wrapped in parens, this becomes
                                          #  '()#)'.  Make sure that doesn't compile.
                             ])

    def test_many_expressions(self):
        # Create a string with many expressions in it. Note that
        #  because we have a space in here as a literal, we're actually
        #  going to use twice as many ast nodes: one for each literal
        #  plus one for each expression.
        def build_fstr(n, extra=''):
            return "f'" + ('{x} ' * n) + extra + "'"

        x = 'X'
        width = 1

        # Test around 256.
        for i in range(250, 260):
            self.assertEqual(eval(build_fstr(i)), (x+' ')*i)

        # Test concatenating 2 largs fstrings.
        self.assertEqual(eval(build_fstr(255)*256), (x+' ')*(255*256))

        s = build_fstr(253, '{x:{width}} ')
        self.assertEqual(eval(s), (x+' ')*254)

        # Test lots of expressions and constants, concatenated.
        s = "f'{1}' 'x' 'y'" * 1024
        self.assertEqual(eval(s), '1xy' * 1024)

    def test_format_specifier_expressions(self):
        width = 10
        precision = 4
        value = decimal.Decimal('12.34567')
        self.assertEqual(f'result: {value:{width}.{precision}}', 'result:      12.35')
        self.assertEqual(f'result: {value:{width!r}.{precision}}', 'result:      12.35')
        self.assertEqual(f'result: {value:{width:0}.{precision:1}}', 'result:      12.35')
        self.assertEqual(f'result: {value:{1}{0:0}.{precision:1}}', 'result:      12.35')
        self.assertEqual(f'result: {value:{ 1}{ 0:0}.{ precision:1}}', 'result:      12.35')
        self.assertEqual(f'{10:#{1}0x}', '       0xa')
        self.assertEqual(f'{10:{"#"}1{0}{"x"}}', '       0xa')
        self.assertEqual(f'{-10:-{"#"}1{0}x}', '      -0xa')
        self.assertEqual(f'{-10:{"-"}#{1}0{"x"}}', '      -0xa')
        self.assertEqual(f'{10:#{3 != {4:5} and width}x}', '       0xa')

        self.assertAllRaise(SyntaxError, "f-string: expecting '}'",
                            ["""f'{"s"!r{":10"}}'""",

                             # This looks like a nested format spec.
                             ])

        self.assertAllRaise(SyntaxError, "f-string: invalid syntax",
                            [# Invalid syntax inside a nested spec.
                             "f'{4:{/5}}'",
                             ])

        self.assertAllRaise(SyntaxError, "f-string: expressions nested too deeply",
                            [# Can't nest format specifiers.
                             "f'result: {value:{width:{0}}.{precision:1}}'",
                             ])

        self.assertAllRaise(SyntaxError, 'f-string: invalid conversion character',
                            [# No expansion inside conversion or for
                             #  the : or ! itself.
                             """f'{"s"!{"r"}}'""",
                             ])

    def test_side_effect_order(self):
        class X:
            def __init__(self):
                self.i = 0
            def __format__(self, spec):
                self.i += 1
                return str(self.i)

        x = X()
        self.assertEqual(f'{x} {x}', '1 2')

    def test_missing_expression(self):
        self.assertAllRaise(SyntaxError, 'f-string: empty expression not allowed',
                            ["f'{}'",
                             "f'{ }'"
                             "f' {} '",
                             "f'{!r}'",
                             "f'{ !r}'",
                             "f'{10:{ }}'",
                             "f' { } '",

                             # The Python parser ignores also the following
                             # whitespace characters in additional to a space.
                             "f'''{\t\f\r\n}'''",

                             # Catch the empty expression before the
                             #  invalid conversion.
                             "f'{!x}'",
                             "f'{ !xr}'",
                             "f'{!x:}'",
                             "f'{!x:a}'",
                             "f'{ !xr:}'",
                             "f'{ !xr:a}'",

                             "f'{!}'",
                             "f'{:}'",

                             # We find the empty expression before the
                             #  missing closing brace.
                             "f'{!'",
                             "f'{!s:'",
                             "f'{:'",
                             "f'{:x'",
                             ])

        # Different error message is raised for other whitespace characters.
        self.assertAllRaise(SyntaxError, r"invalid non-printable character U\+00A0",
                            ["f'''{\xa0}'''",
                             "\xa0",
                             ])

    def test_parens_in_expressions(self):
        self.assertEqual(f'{3,}', '(3,)')

        # Add these because when an expression is evaluated, parens
        #  are added around it. But we shouldn't go from an invalid
        #  expression to a valid one. The added parens are just
        #  supposed to allow whitespace (including newlines).
        self.assertAllRaise(SyntaxError, 'f-string: invalid syntax',
                            ["f'{,}'",
                             "f'{,}'",  # this is (,), which is an error
                             ])

        self.assertAllRaise(SyntaxError, r"f-string: unmatched '\)'",
                            ["f'{3)+(4}'",
                             ])

        self.assertAllRaise(SyntaxError, 'unterminated string literal',
                            ["f'{\n}'",
                             ])
    def test_newlines_before_syntax_error(self):
        self.assertAllRaise(SyntaxError, "invalid syntax",
                ["f'{.}'", "\nf'{.}'", "\n\nf'{.}'"])

    def test_backslashes_in_string_part(self):
        self.assertEqual(f'\t', '\t')
        self.assertEqual(r'\t', '\\t')
        self.assertEqual(rf'\t', '\\t')
        self.assertEqual(f'{2}\t', '2\t')
        self.assertEqual(f'{2}\t{3}', '2\t3')
        self.assertEqual(f'\t{3}', '\t3')

        self.assertEqual(f'\u0394', '\u0394')
        self.assertEqual(r'\u0394', '\\u0394')
        self.assertEqual(rf'\u0394', '\\u0394')
        self.assertEqual(f'{2}\u0394', '2\u0394')
        self.assertEqual(f'{2}\u0394{3}', '2\u03943')
        self.assertEqual(f'\u0394{3}', '\u03943')

        self.assertEqual(f'\U00000394', '\u0394')
        self.assertEqual(r'\U00000394', '\\U00000394')
        self.assertEqual(rf'\U00000394', '\\U00000394')
        self.assertEqual(f'{2}\U00000394', '2\u0394')
        self.assertEqual(f'{2}\U00000394{3}', '2\u03943')
        self.assertEqual(f'\U00000394{3}', '\u03943')

        self.assertEqual(f'\N{GREEK CAPITAL LETTER DELTA}', '\u0394')
        self.assertEqual(f'{2}\N{GREEK CAPITAL LETTER DELTA}', '2\u0394')
        self.assertEqual(f'{2}\N{GREEK CAPITAL LETTER DELTA}{3}', '2\u03943')
        self.assertEqual(f'\N{GREEK CAPITAL LETTER DELTA}{3}', '\u03943')
        self.assertEqual(f'2\N{GREEK CAPITAL LETTER DELTA}', '2\u0394')
        self.assertEqual(f'2\N{GREEK CAPITAL LETTER DELTA}3', '2\u03943')
        self.assertEqual(f'\N{GREEK CAPITAL LETTER DELTA}3', '\u03943')

        self.assertEqual(f'\x20', ' ')
        self.assertEqual(r'\x20', '\\x20')
        self.assertEqual(rf'\x20', '\\x20')
        self.assertEqual(f'{2}\x20', '2 ')
        self.assertEqual(f'{2}\x20{3}', '2 3')
        self.assertEqual(f'\x20{3}', ' 3')

        self.assertEqual(f'2\x20', '2 ')
        self.assertEqual(f'2\x203', '2 3')
        self.assertEqual(f'\x203', ' 3')

        with self.assertWarns(DeprecationWarning):  # invalid escape sequence
            value = eval(r"f'\{6*7}'")
        self.assertEqual(value, '\\42')
        self.assertEqual(f'\\{6*7}', '\\42')
        self.assertEqual(fr'\{6*7}', '\\42')

        AMPERSAND = 'spam'
        # Get the right unicode character (&), or pick up local variable
        # depending on the number of backslashes.
        self.assertEqual(f'\N{AMPERSAND}', '&')
        self.assertEqual(f'\\N{AMPERSAND}', '\\Nspam')
        self.assertEqual(fr'\N{AMPERSAND}', '\\Nspam')
        self.assertEqual(f'\\\N{AMPERSAND}', '\\&')

    def test_misformed_unicode_character_name(self):
        # These test are needed because unicode names are parsed
        # differently inside f-strings.
        self.assertAllRaise(SyntaxError, r"\(unicode error\) 'unicodeescape' codec can't decode bytes in position .*: malformed \\N character escape",
                            [r"f'\N'",
                             r"f'\N '",
                             r"f'\N  '",  # See bpo-46503.
                             r"f'\N{'",
                             r"f'\N{GREEK CAPITAL LETTER DELTA'",

                             # Here are the non-f-string versions,
                             #  which should give the same errors.
                             r"'\N'",
                             r"'\N '",
                             r"'\N  '",
                             r"'\N{'",
                             r"'\N{GREEK CAPITAL LETTER DELTA'",
                             ])

    def test_no_backslashes_in_expression_part(self):
        self.assertAllRaise(SyntaxError, 'f-string expression part cannot include a backslash',
                            [r"f'{\'a\'}'",
                             r"f'{\t3}'",
                             r"f'{\}'",
                             r"rf'{\'a\'}'",
                             r"rf'{\t3}'",
                             r"rf'{\}'",
                             r"""rf'{"\N{LEFT CURLY BRACKET}"}'""",
                             r"f'{\n}'",
                             ])

    def test_no_escapes_for_braces(self):
        """
        Only literal curly braces begin an expression.
        """
        # \x7b is '{'.
        self.assertEqual(f'\x7b1+1}}', '{1+1}')
        self.assertEqual(f'\x7b1+1', '{1+1')
        self.assertEqual(f'\u007b1+1', '{1+1')
        self.assertEqual(f'\N{LEFT CURLY BRACKET}1+1\N{RIGHT CURLY BRACKET}', '{1+1}')

    def test_newlines_in_expressions(self):
        self.assertEqual(f'{0}', '0')
        self.assertEqual(rf'''{3+
4}''', '7')

    def test_lambda(self):
        x = 5
        self.assertEqual(f'{(lambda y:x*y)("8")!r}', "'88888'")
        self.assertEqual(f'{(lambda y:x*y)("8")!r:10}', "'88888'   ")
        self.assertEqual(f'{(lambda y:x*y)("8"):10}', "88888     ")

        # lambda doesn't work without parens, because the colon
        #  makes the parser think it's a format_spec
        self.assertAllRaise(SyntaxError, 'f-string: invalid syntax',
                            ["f'{lambda x:x}'",
                             ])

    def test_yield(self):
        # Not terribly useful, but make sure the yield turns
        #  a function into a generator
        def fn(y):
            f'y:{yield y*2}'
            f'{yield}'

        g = fn(4)
        self.assertEqual(next(g), 8)
        self.assertEqual(next(g), None)

    def test_yield_send(self):
        def fn(x):
            yield f'x:{yield (lambda i: x * i)}'

        g = fn(10)
        the_lambda = next(g)
        self.assertEqual(the_lambda(4), 40)
        self.assertEqual(g.send('string'), 'x:string')

    def test_expressions_with_triple_quoted_strings(self):
        self.assertEqual(f"{'''x'''}", 'x')
        self.assertEqual(f"{'''eric's'''}", "eric's")

        # Test concatenation within an expression
        self.assertEqual(f'{"x" """eric"s""" "y"}', 'xeric"sy')
        self.assertEqual(f'{"x" """eric"s"""}', 'xeric"s')
        self.assertEqual(f'{"""eric"s""" "y"}', 'eric"sy')
        self.assertEqual(f'{"""x""" """eric"s""" "y"}', 'xeric"sy')
        self.assertEqual(f'{"""x""" """eric"s""" """y"""}', 'xeric"sy')
        self.assertEqual(f'{r"""x""" """eric"s""" """y"""}', 'xeric"sy')

    def test_multiple_vars(self):
        x = 98
        y = 'abc'
        self.assertEqual(f'{x}{y}', '98abc')

        self.assertEqual(f'X{x}{y}', 'X98abc')
        self.assertEqual(f'{x}X{y}', '98Xabc')
        self.assertEqual(f'{x}{y}X', '98abcX')

        self.assertEqual(f'X{x}Y{y}', 'X98Yabc')
        self.assertEqual(f'X{x}{y}Y', 'X98abcY')
        self.assertEqual(f'{x}X{y}Y', '98XabcY')

        self.assertEqual(f'X{x}Y{y}Z', 'X98YabcZ')

    def test_closure(self):
        def outer(x):
            def inner():
                return f'x:{x}'
            return inner

        self.assertEqual(outer('987')(), 'x:987')
        self.assertEqual(outer(7)(), 'x:7')

    def test_arguments(self):
        y = 2
        def f(x, width):
            return f'x={x*y:{width}}'

        self.assertEqual(f('foo', 10), 'x=foofoo    ')
        x = 'bar'
        self.assertEqual(f(10, 10), 'x=        20')

    def test_locals(self):
        value = 123
        self.assertEqual(f'v:{value}', 'v:123')

    def test_missing_variable(self):
        with self.assertRaises(NameError):
            f'v:{value}'

    def test_missing_format_spec(self):
        class O:
            def __format__(self, spec):
                if not spec:
                    return '*'
                return spec

        self.assertEqual(f'{O():x}', 'x')
        self.assertEqual(f'{O()}', '*')
        self.assertEqual(f'{O():}', '*')

        self.assertEqual(f'{3:}', '3')
        self.assertEqual(f'{3!s:}', '3')

    def test_global(self):
        self.assertEqual(f'g:{a_global}', 'g:global variable')
        self.assertEqual(f'g:{a_global!r}', "g:'global variable'")

        a_local = 'local variable'
        self.assertEqual(f'g:{a_global} l:{a_local}',
                         'g:global variable l:local variable')
        self.assertEqual(f'g:{a_global!r}',
                         "g:'global variable'")
        self.assertEqual(f'g:{a_global} l:{a_local!r}',
                         "g:global variable l:'local variable'")

        self.assertIn("module 'unittest' from", f'{unittest}')

    def test_shadowed_global(self):
        a_global = 'really a local'
        self.assertEqual(f'g:{a_global}', 'g:really a local')
        self.assertEqual(f'g:{a_global!r}', "g:'really a local'")

        a_local = 'local variable'
        self.assertEqual(f'g:{a_global} l:{a_local}',
                         'g:really a local l:local variable')
        self.assertEqual(f'g:{a_global!r}',
                         "g:'really a local'")
        self.assertEqual(f'g:{a_global} l:{a_local!r}',
                         "g:really a local l:'local variable'")

    def test_call(self):
        def foo(x):
            return 'x=' + str(x)

        self.assertEqual(f'{foo(10)}', 'x=10')

    def test_nested_fstrings(self):
        y = 5
        self.assertEqual(f'{f"{0}"*3}', '000')
        self.assertEqual(f'{f"{y}"*3}', '555')

    def test_invalid_string_prefixes(self):
        single_quote_cases = ["fu''",
                             "uf''",
                             "Fu''",
                             "fU''",
                             "Uf''",
                             "uF''",
                             "ufr''",
                             "urf''",
                             "fur''",
                             "fru''",
                             "rfu''",
                             "ruf''",
                             "FUR''",
                             "Fur''",
                             "fb''",
                             "fB''",
                             "Fb''",
                             "FB''",
                             "bf''",
                             "bF''",
                             "Bf''",
                             "BF''",]
        double_quote_cases = [case.replace("'", '"') for case in single_quote_cases]
        self.assertAllRaise(SyntaxError, 'invalid syntax',
                            single_quote_cases + double_quote_cases)

    def test_leading_trailing_spaces(self):
        self.assertEqual(f'{ 3}', '3')
        self.assertEqual(f'{  3}', '3')
        self.assertEqual(f'{3 }', '3')
        self.assertEqual(f'{3  }', '3')

        self.assertEqual(f'expr={ {x: y for x, y in [(1, 2), ]}}',
                         'expr={1: 2}')
        self.assertEqual(f'expr={ {x: y for x, y in [(1, 2), ]} }',
                         'expr={1: 2}')

    def test_not_equal(self):
        # There's a special test for this because there's a special
        #  case in the f-string parser to look for != as not ending an
        #  expression. Normally it would, while looking for !s or !r.

        self.assertEqual(f'{3!=4}', 'True')
        self.assertEqual(f'{3!=4:}', 'True')
        self.assertEqual(f'{3!=4!s}', 'True')
        self.assertEqual(f'{3!=4!s:.3}', 'Tru')

    def test_equal_equal(self):
        # Because an expression ending in = has special meaning,
        # there's a special test for ==. Make sure it works.

        self.assertEqual(f'{0==1}', 'False')

    def test_conversions(self):
        self.assertEqual(f'{3.14:10.10}', '      3.14')
        self.assertEqual(f'{3.14!s:10.10}', '3.14      ')
        self.assertEqual(f'{3.14!r:10.10}', '3.14      ')
        self.assertEqual(f'{3.14!a:10.10}', '3.14      ')

        self.assertEqual(f'{"a"}', 'a')
        self.assertEqual(f'{"a"!r}', "'a'")
        self.assertEqual(f'{"a"!a}', "'a'")

        # Not a conversion.
        self.assertEqual(f'{"a!r"}', "a!r")

        # Not a conversion, but show that ! is allowed in a format spec.
        self.assertEqual(f'{3.14:!<10.10}', '3.14!!!!!!')

        self.assertAllRaise(SyntaxError, 'f-string: invalid conversion character',
                            ["f'{3!g}'",
                             "f'{3!A}'",
                             "f'{3!3}'",
                             "f'{3!G}'",
                             "f'{3!!}'",
                             "f'{3!:}'",
                             "f'{3! s}'",  # no space before conversion char
                             ])

        self.assertAllRaise(SyntaxError, "f-string: expecting '}'",
                            ["f'{x!s{y}}'",
                             "f'{3!ss}'",
                             "f'{3!ss:}'",
                             "f'{3!ss:s}'",
                             ])

    def test_assignment(self):
        self.assertAllRaise(SyntaxError, r'invalid syntax',
                            ["f'' = 3",
                             "f'{0}' = x",
                             "f'{x}' = x",
                             ])

    def test_del(self):
        self.assertAllRaise(SyntaxError, 'invalid syntax',
                            ["del f''",
                             "del '' f''",
                             ])

    def test_mismatched_braces(self):
        self.assertAllRaise(SyntaxError, "f-string: single '}' is not allowed",
                            ["f'{{}'",
                             "f'{{}}}'",
                             "f'}'",
                             "f'x}'",
                             "f'x}x'",
                             r"f'\u007b}'",

                             # Can't have { or } in a format spec.
                             "f'{3:}>10}'",
                             "f'{3:}}>10}'",
                             ])

        self.assertAllRaise(SyntaxError, "f-string: expecting '}'",
                            ["f'{3:{{>10}'",
                             "f'{3'",
                             "f'{3!'",
                             "f'{3:'",
                             "f'{3!s'",
                             "f'{3!s:'",
                             "f'{3!s:3'",
                             "f'x{'",
                             "f'x{x'",
                             "f'{x'",
                             "f'{3:s'",
                             "f'{{{'",
                             "f'{{}}{'",
                             "f'{'",
                             "f'x{<'",  # See bpo-46762.
                             "f'x{>'",
                             ])

        # But these are just normal strings.
        self.assertEqual(f'{"{"}', '{')
        self.assertEqual(f'{"}"}', '}')
        self.assertEqual(f'{3:{"}"}>10}', '}}}}}}}}}3')
        self.assertEqual(f'{2:{"{"}>10}', '{{{{{{{{{2')

    def test_if_conditional(self):
        # There's special logic in compile.c to test if the
        #  conditional for an if (and while) are constants. Exercise
        #  that code.

        def test_fstring(x, expected):
            flag = 0
            if f'{x}':
                flag = 1
            else:
                flag = 2
            self.assertEqual(flag, expected)

        def test_concat_empty(x, expected):
            flag = 0
            if '' f'{x}':
                flag = 1
            else:
                flag = 2
            self.assertEqual(flag, expected)

        def test_concat_non_empty(x, expected):
            flag = 0
            if ' ' f'{x}':
                flag = 1
            else:
                flag = 2
            self.assertEqual(flag, expected)

        test_fstring('', 2)
        test_fstring(' ', 1)

        test_concat_empty('', 2)
        test_concat_empty(' ', 1)

        test_concat_non_empty('', 1)
        test_concat_non_empty(' ', 1)

    def test_empty_format_specifier(self):
        x = 'test'
        self.assertEqual(f'{x}', 'test')
        self.assertEqual(f'{x:}', 'test')
        self.assertEqual(f'{x!s:}', 'test')
        self.assertEqual(f'{x!r:}', "'test'")

    def test_str_format_differences(self):
        d = {'a': 'string',
             0: 'integer',
             }
        a = 0
        self.assertEqual(f'{d[0]}', 'integer')
        self.assertEqual(f'{d["a"]}', 'string')
        self.assertEqual(f'{d[a]}', 'integer')
        self.assertEqual('{d[a]}'.format(d=d), 'string')
        self.assertEqual('{d[0]}'.format(d=d), 'integer')

    def test_errors(self):
        # see issue 26287
        self.assertAllRaise(TypeError, 'unsupported',
                            [r"f'{(lambda: 0):x}'",
                             r"f'{(0,):x}'",
                             ])
        self.assertAllRaise(ValueError, 'Unknown format code',
                            [r"f'{1000:j}'",
                             r"f'{1000:j}'",
                            ])

    def test_filename_in_syntaxerror(self):
        # see issue 38964
        with temp_cwd() as cwd:
            file_path = os.path.join(cwd, 't.py')
            with open(file_path, 'w', encoding="utf-8") as f:
                f.write('f"{a b}"') # This generates a SyntaxError
            _, _, stderr = assert_python_failure(file_path,
                                                 PYTHONIOENCODING='ascii')
        self.assertIn(file_path.encode('ascii', 'backslashreplace'), stderr)

    def test_loop(self):
        for i in range(1000):
            self.assertEqual(f'i:{i}', 'i:' + str(i))

    def test_dict(self):
        d = {'"': 'dquote',
             "'": 'squote',
             'foo': 'bar',
             }
        self.assertEqual(f'''{d["'"]}''', 'squote')
        self.assertEqual(f"""{d['"']}""", 'dquote')

        self.assertEqual(f'{d["foo"]}', 'bar')
        self.assertEqual(f"{d['foo']}", 'bar')

    def test_backslash_char(self):
        # Check eval of a backslash followed by a control char.
        # See bpo-30682: this used to raise an assert in pydebug mode.
        self.assertEqual(eval('f"\\\n"'), '')
        self.assertEqual(eval('f"\\\r"'), '')

    def test_debug_conversion(self):
        x = 'A string'
        self.assertEqual(f'{x=}', 'x=' + repr(x))
        self.assertEqual(f'{x =}', 'x =' + repr(x))
        self.assertEqual(f'{x=!s}', 'x=' + str(x))
        self.assertEqual(f'{x=!r}', 'x=' + repr(x))
        self.assertEqual(f'{x=!a}', 'x=' + ascii(x))

        x = 2.71828
        self.assertEqual(f'{x=:.2f}', 'x=' + format(x, '.2f'))
        self.assertEqual(f'{x=:}', 'x=' + format(x, ''))
        self.assertEqual(f'{x=!r:^20}', 'x=' + format(repr(x), '^20'))
        self.assertEqual(f'{x=!s:^20}', 'x=' + format(str(x), '^20'))
        self.assertEqual(f'{x=!a:^20}', 'x=' + format(ascii(x), '^20'))

        x = 9
        self.assertEqual(f'{3*x+15=}', '3*x+15=42')

        # There is code in ast.c that deals with non-ascii expression values.  So,
        # use a unicode identifier to trigger that.
        tenπ = 31.4
        self.assertEqual(f'{tenπ=:.2f}', 'tenπ=31.40')

        # Also test with Unicode in non-identifiers.
        self.assertEqual(f'{"Σ"=}', '"Σ"=\'Σ\'')

        # Make sure nested fstrings still work.
        self.assertEqual(f'{f"{3.1415=:.1f}":*^20}', '*****3.1415=3.1*****')

        # Make sure text before and after an expression with = works
        # correctly.
        pi = 'π'
        self.assertEqual(f'alpha α {pi=} ω omega', "alpha α pi='π' ω omega")

        # Check multi-line expressions.
        self.assertEqual(f'''{
3
=}''', '\n3\n=3')

        # Since = is handled specially, make sure all existing uses of
        # it still work.

        self.assertEqual(f'{0==1}', 'False')
        self.assertEqual(f'{0!=1}', 'True')
        self.assertEqual(f'{0<=1}', 'True')
        self.assertEqual(f'{0>=1}', 'False')
        self.assertEqual(f'{(x:="5")}', '5')
        self.assertEqual(x, '5')
        self.assertEqual(f'{(x:=5)}', '5')
        self.assertEqual(x, 5)
        self.assertEqual(f'{"="}', '=')

        x = 20
        # This isn't an assignment expression, it's 'x', with a format
        # spec of '=10'.  See test_walrus: you need to use parens.
        self.assertEqual(f'{x:=10}', '        20')

        # Test named function parameters, to make sure '=' parsing works
        # there.
        def f(a):
            nonlocal x
            oldx = x
            x = a
            return oldx
        x = 0
        self.assertEqual(f'{f(a="3=")}', '0')
        self.assertEqual(x, '3=')
        self.assertEqual(f'{f(a=4)}', '3=')
        self.assertEqual(x, 4)

        # Make sure __format__ is being called.
        class C:
            def __format__(self, s):
                return f'FORMAT-{s}'
            def __repr__(self):
                return 'REPR'

        self.assertEqual(f'{C()=}', 'C()=REPR')
        self.assertEqual(f'{C()=!r}', 'C()=REPR')
        self.assertEqual(f'{C()=:}', 'C()=FORMAT-')
        self.assertEqual(f'{C()=: }', 'C()=FORMAT- ')
        self.assertEqual(f'{C()=:x}', 'C()=FORMAT-x')
        self.assertEqual(f'{C()=!r:*^20}', 'C()=********REPR********')

        self.assertRaises(SyntaxError, eval, "f'{C=]'")

        # Make sure leading and following text works.
        x = 'foo'
        self.assertEqual(f'X{x=}Y', 'Xx='+repr(x)+'Y')

        # Make sure whitespace around the = works.
        self.assertEqual(f'X{x  =}Y', 'Xx  ='+repr(x)+'Y')
        self.assertEqual(f'X{x=  }Y', 'Xx=  '+repr(x)+'Y')
        self.assertEqual(f'X{x  =  }Y', 'Xx  =  '+repr(x)+'Y')

        # These next lines contains tabs.  Backslash escapes don't
        # work in f-strings.
        # patchcheck doesn't like these tabs.  So the only way to test
        # this will be to dynamically created and exec the f-strings.  But
        # that's such a hassle I'll save it for another day.  For now, convert
        # the tabs to spaces just to shut up patchcheck.
        #self.assertEqual(f'X{x =}Y', 'Xx\t='+repr(x)+'Y')
        #self.assertEqual(f'X{x =       }Y', 'Xx\t=\t'+repr(x)+'Y')

    def test_walrus(self):
        x = 20
        # This isn't an assignment expression, it's 'x', with a format
        # spec of '=10'.
        self.assertEqual(f'{x:=10}', '        20')

        # This is an assignment expression, which requires parens.
        self.assertEqual(f'{(x:=10)}', '10')
        self.assertEqual(x, 10)

    def test_invalid_syntax_error_message(self):
        with self.assertRaisesRegex(SyntaxError, "f-string: invalid syntax"):
            compile("f'{a $ b}'", "?", "exec")

    def test_with_two_commas_in_format_specifier(self):
        error_msg = re.escape("Cannot specify ',' with ','.")
        with self.assertRaisesRegex(ValueError, error_msg):
            f'{1:,,}'

    def test_with_two_underscore_in_format_specifier(self):
        error_msg = re.escape("Cannot specify '_' with '_'.")
        with self.assertRaisesRegex(ValueError, error_msg):
            f'{1:__}'

    def test_with_a_commas_and_an_underscore_in_format_specifier(self):
        error_msg = re.escape("Cannot specify both ',' and '_'.")
        with self.assertRaisesRegex(ValueError, error_msg):
            f'{1:,_}'

    def test_with_an_underscore_and_a_comma_in_format_specifier(self):
        error_msg = re.escape("Cannot specify both ',' and '_'.")
        with self.assertRaisesRegex(ValueError, error_msg):
            f'{1:_,}'

    def test_syntax_error_for_starred_expressions(self):
        error_msg = re.escape("cannot use starred expression here")
        with self.assertRaisesRegex(SyntaxError, error_msg):
            compile("f'{*a}'", "?", "exec")

        error_msg = re.escape("cannot use double starred expression here")
        with self.assertRaisesRegex(SyntaxError, error_msg):
            compile("f'{**a}'", "?", "exec")

if __name__ == '__main__':
    unittest.main()
