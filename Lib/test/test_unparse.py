"""Tests for ast.unparse."""

import unittest
import test.support
import pathlib
import random
import tokenize
import warnings
import ast
from test.support.ast_helper import ASTTestMixin


def read_pyfile(filename):
    """Read and return the contents of a Python source file (as a
    string), taking into account the file encoding."""
    with tokenize.open(filename) as stream:
        return stream.read()


for_else = """\
def f():
    for x in range(10):
        break
    else:
        y = 2
    z = 3
"""

while_else = """\
def g():
    while True:
        break
    else:
        y = 2
    z = 3
"""

relative_import = """\
from . import fred
from .. import barney
from .australia import shrimp as prawns
"""

nonlocal_ex = """\
def f():
    x = 1
    def g():
        nonlocal x
        x = 2
        y = 7
        def h():
            nonlocal x, y
"""

# also acts as test for 'except ... as ...'
raise_from = """\
try:
    1 / 0
except ZeroDivisionError as e:
    raise ArithmeticError from e
"""

class_decorator = """\
@f1(arg)
@f2
class Foo: pass
"""

elif1 = """\
if cond1:
    suite1
elif cond2:
    suite2
else:
    suite3
"""

elif2 = """\
if cond1:
    suite1
elif cond2:
    suite2
"""

try_except_finally = """\
try:
    suite1
except ex1:
    suite2
except ex2:
    suite3
else:
    suite4
finally:
    suite5
"""

try_except_star_finally = """\
try:
    suite1
except* ex1:
    suite2
except* ex2:
    suite3
else:
    suite4
finally:
    suite5
"""

with_simple = """\
with f():
    suite1
"""

with_as = """\
with f() as x:
    suite1
"""

with_two_items = """\
with f() as x, g() as y:
    suite1
"""

docstring_prefixes = (
    "",
    "class foo:\n    ",
    "def foo():\n    ",
    "async def foo():\n    ",
)

class ASTTestCase(ASTTestMixin, unittest.TestCase):
    def check_ast_roundtrip(self, code1, **kwargs):
        with self.subTest(code1=code1, ast_parse_kwargs=kwargs):
            ast1 = ast.parse(code1, **kwargs)
            code2 = ast.unparse(ast1)
            ast2 = ast.parse(code2, **kwargs)
            self.assertASTEqual(ast1, ast2)

    def check_invalid(self, node, raises=ValueError):
        with self.subTest(node=node):
            self.assertRaises(raises, ast.unparse, node)

    def get_source(self, code1, code2=None, **kwargs):
        code2 = code2 or code1
        code1 = ast.unparse(ast.parse(code1, **kwargs))
        return code1, code2

    def check_src_roundtrip(self, code1, code2=None, **kwargs):
        code1, code2 = self.get_source(code1, code2, **kwargs)
        with self.subTest(code1=code1, code2=code2):
            self.assertEqual(code2, code1)

    def check_src_dont_roundtrip(self, code1, code2=None):
        code1, code2 = self.get_source(code1, code2)
        with self.subTest(code1=code1, code2=code2):
            self.assertNotEqual(code2, code1)

class UnparseTestCase(ASTTestCase):
    # Tests for specific bugs found in earlier versions of unparse

    def test_fstrings(self):
        self.check_ast_roundtrip("f'a'")
        self.check_ast_roundtrip("f'{{}}'")
        self.check_ast_roundtrip("f'{{5}}'")
        self.check_ast_roundtrip("f'{{5}}5'")
        self.check_ast_roundtrip("f'X{{}}X'")
        self.check_ast_roundtrip("f'{a}'")
        self.check_ast_roundtrip("f'{ {1:2}}'")
        self.check_ast_roundtrip("f'a{a}a'")
        self.check_ast_roundtrip("f'a{a}{a}a'")
        self.check_ast_roundtrip("f'a{a}a{a}a'")
        self.check_ast_roundtrip("f'{a!r}x{a!s}12{{}}{a!a}'")
        self.check_ast_roundtrip("f'{a:10}'")
        self.check_ast_roundtrip("f'{a:100_000{10}}'")
        self.check_ast_roundtrip("f'{a!r:10}'")
        self.check_ast_roundtrip("f'{a:a{b}10}'")
        self.check_ast_roundtrip(
                "f'a{b}{c!s}{d!r}{e!a}{f:a}{g:a{b}}{h!s:a}"
                "{j!s:{a}b}{k!s:a{b}c}{l!a:{b}c{d}}{x+y=}'"
        )

    def test_fstrings_special_chars(self):
        # See issue 25180
        self.check_ast_roundtrip(r"""f'{f"{0}"*3}'""")
        self.check_ast_roundtrip(r"""f'{f"{y}"*3}'""")
        self.check_ast_roundtrip("""f''""")
        self.check_ast_roundtrip('''f"""'end' "quote\\""""''')

    def test_fstrings_complicated(self):
        # See issue 28002
        self.check_ast_roundtrip("""f'''{"'"}'''""")
        self.check_ast_roundtrip('''f\'\'\'-{f"""*{f"+{f'.{x}.'}+"}*"""}-\'\'\'''')
        self.check_ast_roundtrip('''f\'\'\'-{f"""*{f"+{f'.{x}.'}+"}*"""}-'single quote\\'\'\'\'''')
        self.check_ast_roundtrip('f"""{\'\'\'\n\'\'\'}"""')
        self.check_ast_roundtrip('f"""{g(\'\'\'\n\'\'\')}"""')
        self.check_ast_roundtrip('''f"a\\r\\nb"''')
        self.check_ast_roundtrip('''f"\\u2028{'x'}"''')

    def test_fstrings_pep701(self):
        self.check_ast_roundtrip('f" something { my_dict["key"] } something else "')
        self.check_ast_roundtrip('f"{f"{f"{f"{f"{f"{1+1}"}"}"}"}"}"')

    def test_tstrings(self):
        self.check_ast_roundtrip("t'foo'")
        self.check_ast_roundtrip("t'foo {bar}'")
        self.check_ast_roundtrip("t'foo {bar!s:.2f}'")

    def test_strings(self):
        self.check_ast_roundtrip("u'foo'")
        self.check_ast_roundtrip("r'foo'")
        self.check_ast_roundtrip("b'foo'")

    def test_del_statement(self):
        self.check_ast_roundtrip("del x, y, z")

    def test_shifts(self):
        self.check_ast_roundtrip("45 << 2")
        self.check_ast_roundtrip("13 >> 7")

    def test_for_else(self):
        self.check_ast_roundtrip(for_else)

    def test_while_else(self):
        self.check_ast_roundtrip(while_else)

    def test_unary_parens(self):
        self.check_ast_roundtrip("(-1)**7")
        self.check_ast_roundtrip("(-1.)**8")
        self.check_ast_roundtrip("(-1j)**6")
        self.check_ast_roundtrip("not True or False")
        self.check_ast_roundtrip("True or not False")

    def test_integer_parens(self):
        self.check_ast_roundtrip("3 .__abs__()")

    def test_huge_float(self):
        self.check_ast_roundtrip("1e1000")
        self.check_ast_roundtrip("-1e1000")
        self.check_ast_roundtrip("1e1000j")
        self.check_ast_roundtrip("-1e1000j")

    def test_nan(self):
        self.assertASTEqual(
            ast.parse(ast.unparse(ast.Constant(value=float('nan')))),
            ast.parse('1e1000 - 1e1000')
        )

    def test_min_int(self):
        self.check_ast_roundtrip(str(-(2 ** 31)))
        self.check_ast_roundtrip(str(-(2 ** 63)))

    def test_imaginary_literals(self):
        self.check_ast_roundtrip("7j")
        self.check_ast_roundtrip("-7j")
        self.check_ast_roundtrip("0j")
        self.check_ast_roundtrip("-0j")

    def test_lambda_parentheses(self):
        self.check_ast_roundtrip("(lambda: int)()")

    def test_chained_comparisons(self):
        self.check_ast_roundtrip("1 < 4 <= 5")
        self.check_ast_roundtrip("a is b is c is not d")

    def test_function_arguments(self):
        self.check_ast_roundtrip("def f(): pass")
        self.check_ast_roundtrip("def f(a): pass")
        self.check_ast_roundtrip("def f(b = 2): pass")
        self.check_ast_roundtrip("def f(a, b): pass")
        self.check_ast_roundtrip("def f(a, b = 2): pass")
        self.check_ast_roundtrip("def f(a = 5, b = 2): pass")
        self.check_ast_roundtrip("def f(*, a = 1, b = 2): pass")
        self.check_ast_roundtrip("def f(*, a = 1, b): pass")
        self.check_ast_roundtrip("def f(*, a, b = 2): pass")
        self.check_ast_roundtrip("def f(a, b = None, *, c, **kwds): pass")
        self.check_ast_roundtrip("def f(a=2, *args, c=5, d, **kwds): pass")
        self.check_ast_roundtrip("def f(*args, **kwargs): pass")

    def test_relative_import(self):
        self.check_ast_roundtrip(relative_import)

    def test_nonlocal(self):
        self.check_ast_roundtrip(nonlocal_ex)

    def test_raise_from(self):
        self.check_ast_roundtrip(raise_from)

    def test_bytes(self):
        self.check_ast_roundtrip("b'123'")

    def test_annotations(self):
        self.check_ast_roundtrip("def f(a : int): pass")
        self.check_ast_roundtrip("def f(a: int = 5): pass")
        self.check_ast_roundtrip("def f(*args: [int]): pass")
        self.check_ast_roundtrip("def f(**kwargs: dict): pass")
        self.check_ast_roundtrip("def f() -> None: pass")

    def test_set_literal(self):
        self.check_ast_roundtrip("{'a', 'b', 'c'}")

    def test_empty_set(self):
        self.assertASTEqual(
            ast.parse(ast.unparse(ast.Set(elts=[]))),
            ast.parse('{*()}')
        )

    def test_set_comprehension(self):
        self.check_ast_roundtrip("{x for x in range(5)}")

    def test_dict_comprehension(self):
        self.check_ast_roundtrip("{x: x*x for x in range(10)}")

    def test_class_decorators(self):
        self.check_ast_roundtrip(class_decorator)

    def test_class_definition(self):
        self.check_ast_roundtrip("class A(metaclass=type, *[], **{}): pass")

    def test_elifs(self):
        self.check_ast_roundtrip(elif1)
        self.check_ast_roundtrip(elif2)

    def test_try_except_finally(self):
        self.check_ast_roundtrip(try_except_finally)

    def test_try_except_star_finally(self):
        self.check_ast_roundtrip(try_except_star_finally)

    def test_starred_assignment(self):
        self.check_ast_roundtrip("a, *b, c = seq")
        self.check_ast_roundtrip("a, (*b, c) = seq")
        self.check_ast_roundtrip("a, *b[0], c = seq")
        self.check_ast_roundtrip("a, *(b, c) = seq")

    def test_with_simple(self):
        self.check_ast_roundtrip(with_simple)

    def test_with_as(self):
        self.check_ast_roundtrip(with_as)

    def test_with_two_items(self):
        self.check_ast_roundtrip(with_two_items)

    def test_dict_unpacking_in_dict(self):
        # See issue 26489
        self.check_ast_roundtrip(r"""{**{'y': 2}, 'x': 1}""")
        self.check_ast_roundtrip(r"""{**{'y': 2}, **{'x': 1}}""")

    def test_slices(self):
        self.check_ast_roundtrip("a[i]")
        self.check_ast_roundtrip("a[i,]")
        self.check_ast_roundtrip("a[i, j]")
        # The AST for these next two both look like `a[(*a,)]`
        self.check_ast_roundtrip("a[(*a,)]")
        self.check_ast_roundtrip("a[*a]")
        self.check_ast_roundtrip("a[b, *a]")
        self.check_ast_roundtrip("a[*a, c]")
        self.check_ast_roundtrip("a[b, *a, c]")
        self.check_ast_roundtrip("a[*a, *a]")
        self.check_ast_roundtrip("a[b, *a, *a]")
        self.check_ast_roundtrip("a[*a, b, *a]")
        self.check_ast_roundtrip("a[*a, *a, b]")
        self.check_ast_roundtrip("a[b, *a, *a, c]")
        self.check_ast_roundtrip("a[(a:=b)]")
        self.check_ast_roundtrip("a[(a:=b,c)]")
        self.check_ast_roundtrip("a[()]")
        self.check_ast_roundtrip("a[i:j]")
        self.check_ast_roundtrip("a[:j]")
        self.check_ast_roundtrip("a[i:]")
        self.check_ast_roundtrip("a[i:j:k]")
        self.check_ast_roundtrip("a[:j:k]")
        self.check_ast_roundtrip("a[i::k]")
        self.check_ast_roundtrip("a[i:j,]")
        self.check_ast_roundtrip("a[i:j, k]")

    def test_invalid_raise(self):
        self.check_invalid(ast.Raise(exc=None, cause=ast.Name(id="X", ctx=ast.Load())))

    def test_invalid_fstring_value(self):
        self.check_invalid(
            ast.JoinedStr(
                values=[
                    ast.Name(id="test", ctx=ast.Load()),
                    ast.Constant(value="test")
                ]
            )
        )

    def test_fstring_backslash(self):
        # valid since Python 3.12
        self.assertEqual(ast.unparse(
                            ast.FormattedValue(
                                value=ast.Constant(value="\\\\"),
                                conversion=-1,
                                format_spec=None,
                            )
                        ), "{'\\\\\\\\'}")

    def test_invalid_yield_from(self):
        self.check_invalid(ast.YieldFrom(value=None))

    def test_import_from_level_none(self):
        tree = ast.ImportFrom(module='mod', names=[ast.alias(name='x')])
        self.assertEqual(ast.unparse(tree), "from mod import x")
        tree = ast.ImportFrom(module='mod', names=[ast.alias(name='x')], level=None)
        self.assertEqual(ast.unparse(tree), "from mod import x")

    def test_docstrings(self):
        docstrings = (
            'this ends with double quote"',
            'this includes a """triple quote"""',
            '\r',
            '\\r',
            '\t',
            '\\t',
            '\n',
            '\\n',
            '\r\\r\t\\t\n\\n',
            '""">>> content = \"\"\"blabla\"\"\" <<<"""',
            r'foo\n\x00',
            "' \\'\\'\\'\"\"\" \"\"\\'\\' \\'",
            '🐍⛎𩸽üéş^\\\\X\\\\BB\N{LONG RIGHTWARDS SQUIGGLE ARROW}'
        )
        for docstring in docstrings:
            # check as Module docstrings for easy testing
            self.check_ast_roundtrip(f"'''{docstring}'''")

    def test_constant_tuples(self):
        locs = ast.fix_missing_locations
        self.check_src_roundtrip(
            locs(ast.Module([ast.Expr(ast.Constant(value=(1,)))])), "(1,)")
        self.check_src_roundtrip(
            locs(ast.Module([ast.Expr(ast.Constant(value=(1, 2, 3)))])), "(1, 2, 3)"
        )

    def test_function_type(self):
        for function_type in (
            "() -> int",
            "(int, int) -> int",
            "(Callable[complex], More[Complex(call.to_typevar())]) -> None"
        ):
            self.check_ast_roundtrip(function_type, mode="func_type")

    def test_type_comments(self):
        for statement in (
            "a = 5 # type:",
            "a = 5 # type: int",
            "a = 5 # type: int and more",
            "def x(): # type: () -> None\n\tpass",
            "def x(y): # type: (int) -> None and more\n\tpass",
            "async def x(): # type: () -> None\n\tpass",
            "async def x(y): # type: (int) -> None and more\n\tpass",
            "for x in y: # type: int\n\tpass",
            "async for x in y: # type: int\n\tpass",
            "with x(): # type: int\n\tpass",
            "async with x(): # type: int\n\tpass"
        ):
            self.check_ast_roundtrip(statement, type_comments=True)

    def test_type_ignore(self):
        for statement in (
            "a = 5 # type: ignore",
            "a = 5 # type: ignore and more",
            "def x(): # type: ignore\n\tpass",
            "def x(y): # type: ignore and more\n\tpass",
            "async def x(): # type: ignore\n\tpass",
            "async def x(y): # type: ignore and more\n\tpass",
            "for x in y: # type: ignore\n\tpass",
            "async for x in y: # type: ignore\n\tpass",
            "with x(): # type: ignore\n\tpass",
            "async with x(): # type: ignore\n\tpass"
        ):
            self.check_ast_roundtrip(statement, type_comments=True)

    def test_unparse_interactive_semicolons(self):
        # gh-129598: Fix ast.unparse() when ast.Interactive contains multiple statements
        self.check_src_roundtrip("i = 1; 'expr'; raise Exception", mode='single')
        self.check_src_roundtrip("i: int = 1; j: float = 0; k += l", mode='single')
        combinable = (
            "'expr'",
            "(i := 1)",
            "import foo",
            "from foo import bar",
            "i = 1",
            "i += 1",
            "i: int = 1",
            "return i",
            "pass",
            "break",
            "continue",
            "del i",
            "assert i",
            "global i",
            "nonlocal j",
            "await i",
            "yield i",
            "yield from i",
            "raise i",
            "type t[T] = ...",
            "i",
        )
        for a in combinable:
            for b in combinable:
                self.check_src_roundtrip(f"{a}; {b}", mode='single')

    def test_unparse_interactive_integrity_1(self):
        # rest of unparse_interactive_integrity tests just make sure mode='single' parse and unparse didn't break
        self.check_src_roundtrip(
            "if i:\n 'expr'\nelse:\n raise Exception",
            "if i:\n    'expr'\nelse:\n    raise Exception",
            mode='single'
        )
        self.check_src_roundtrip(
            "@decorator1\n@decorator2\ndef func():\n 'docstring'\n i = 1; 'expr'; raise Exception",
            '''@decorator1\n@decorator2\ndef func():\n    """docstring"""\n    i = 1\n    'expr'\n    raise Exception''',
            mode='single'
        )
        self.check_src_roundtrip(
            "@decorator1\n@decorator2\nclass cls:\n 'docstring'\n i = 1; 'expr'; raise Exception",
            '''@decorator1\n@decorator2\nclass cls:\n    """docstring"""\n    i = 1\n    'expr'\n    raise Exception''',
            mode='single'
        )

    def test_unparse_interactive_integrity_2(self):
        for statement in (
            "def x():\n    pass",
            "def x(y):\n    pass",
            "async def x():\n    pass",
            "async def x(y):\n    pass",
            "for x in y:\n    pass",
            "async for x in y:\n    pass",
            "with x():\n    pass",
            "async with x():\n    pass",
            "def f():\n    pass",
            "def f(a):\n    pass",
            "def f(b=2):\n    pass",
            "def f(a, b):\n    pass",
            "def f(a, b=2):\n    pass",
            "def f(a=5, b=2):\n    pass",
            "def f(*, a=1, b=2):\n    pass",
            "def f(*, a=1, b):\n    pass",
            "def f(*, a, b=2):\n    pass",
            "def f(a, b=None, *, c, **kwds):\n    pass",
            "def f(a=2, *args, c=5, d, **kwds):\n    pass",
            "def f(*args, **kwargs):\n    pass",
            "class cls:\n\n    def f(self):\n        pass",
            "class cls:\n\n    def f(self, a):\n        pass",
            "class cls:\n\n    def f(self, b=2):\n        pass",
            "class cls:\n\n    def f(self, a, b):\n        pass",
            "class cls:\n\n    def f(self, a, b=2):\n        pass",
            "class cls:\n\n    def f(self, a=5, b=2):\n        pass",
            "class cls:\n\n    def f(self, *, a=1, b=2):\n        pass",
            "class cls:\n\n    def f(self, *, a=1, b):\n        pass",
            "class cls:\n\n    def f(self, *, a, b=2):\n        pass",
            "class cls:\n\n    def f(self, a, b=None, *, c, **kwds):\n        pass",
            "class cls:\n\n    def f(self, a=2, *args, c=5, d, **kwds):\n        pass",
            "class cls:\n\n    def f(self, *args, **kwargs):\n        pass",
        ):
            self.check_src_roundtrip(statement, mode='single')

    def test_unparse_interactive_integrity_3(self):
        for statement in (
            "def x():",
            "def x(y):",
            "async def x():",
            "async def x(y):",
            "for x in y:",
            "async for x in y:",
            "with x():",
            "async with x():",
            "def f():",
            "def f(a):",
            "def f(b=2):",
            "def f(a, b):",
            "def f(a, b=2):",
            "def f(a=5, b=2):",
            "def f(*, a=1, b=2):",
            "def f(*, a=1, b):",
            "def f(*, a, b=2):",
            "def f(a, b=None, *, c, **kwds):",
            "def f(a=2, *args, c=5, d, **kwds):",
            "def f(*args, **kwargs):",
        ):
            src = statement + '\n i=1;j=2'
            out = statement + '\n    i = 1\n    j = 2'

            self.check_src_roundtrip(src, out, mode='single')


class CosmeticTestCase(ASTTestCase):
    """Test if there are cosmetic issues caused by unnecessary additions"""

    def test_simple_expressions_parens(self):
        self.check_src_roundtrip("(a := b)")
        self.check_src_roundtrip("await x")
        self.check_src_roundtrip("x if x else y")
        self.check_src_roundtrip("lambda x: x")
        self.check_src_roundtrip("1 + 1")
        self.check_src_roundtrip("1 + 2 / 3")
        self.check_src_roundtrip("(1 + 2) / 3")
        self.check_src_roundtrip("(1 + 2) * 3 + 4 * (5 + 2)")
        self.check_src_roundtrip("(1 + 2) * 3 + 4 * (5 + 2) ** 2")
        self.check_src_roundtrip("~x")
        self.check_src_roundtrip("x and y")
        self.check_src_roundtrip("x and y and z")
        self.check_src_roundtrip("x and (y and x)")
        self.check_src_roundtrip("(x and y) and z")
        self.check_src_roundtrip("(x ** y) ** z ** q")
        self.check_src_roundtrip("x >> y")
        self.check_src_roundtrip("x << y")
        self.check_src_roundtrip("x >> y and x >> z")
        self.check_src_roundtrip("x + y - z * q ^ t ** k")
        self.check_src_roundtrip("P * V if P and V else n * R * T")
        self.check_src_roundtrip("lambda P, V, n: P * V == n * R * T")
        self.check_src_roundtrip("flag & (other | foo)")
        self.check_src_roundtrip("not x == y")
        self.check_src_roundtrip("x == (not y)")
        self.check_src_roundtrip("yield x")
        self.check_src_roundtrip("yield from x")
        self.check_src_roundtrip("call((yield x))")
        self.check_src_roundtrip("return x + (yield x)")

    def test_class_bases_and_keywords(self):
        self.check_src_roundtrip("class X:\n    pass")
        self.check_src_roundtrip("class X(A):\n    pass")
        self.check_src_roundtrip("class X(A, B, C, D):\n    pass")
        self.check_src_roundtrip("class X(x=y):\n    pass")
        self.check_src_roundtrip("class X(metaclass=z):\n    pass")
        self.check_src_roundtrip("class X(x=y, z=d):\n    pass")
        self.check_src_roundtrip("class X(A, x=y):\n    pass")
        self.check_src_roundtrip("class X(A, **kw):\n    pass")
        self.check_src_roundtrip("class X(*args):\n    pass")
        self.check_src_roundtrip("class X(*args, **kwargs):\n    pass")

    def test_fstrings(self):
        self.check_src_roundtrip('''f\'\'\'-{f"""*{f"+{f'.{x}.'}+"}*"""}-\'\'\'''')
        self.check_src_roundtrip('''f\'-{f\'\'\'*{f"""+{f".{f'{x}'}."}+"""}*\'\'\'}-\'''')
        self.check_src_roundtrip('''f\'-{f\'*{f\'\'\'+{f""".{f"{f'{x}'}"}."""}+\'\'\'}*\'}-\'''')
        self.check_src_roundtrip('''f"\\u2028{'x'}"''')
        self.check_src_roundtrip(r"f'{x}\n'")
        self.check_src_roundtrip('''f"{'\\n'}\\n"''')
        self.check_src_roundtrip('''f"{f'{x}\\n'}\\n"''')

    def test_docstrings(self):
        docstrings = (
            '"""simple doc string"""',
            '''"""A more complex one
            with some newlines"""''',
            '''"""Foo bar baz

            empty newline"""''',
            '"""With some \t"""',
            '"""Foo "bar" baz """',
            '"""\\r"""',
            '""""""',
            '"""\'\'\'"""',
            '"""\'\'\'\'\'\'"""',
            '"""🐍⛎𩸽üéş^\\\\X\\\\BB⟿"""',
            '"""end in single \'quote\'"""',
            "'''end in double \"quote\"'''",
            '"""almost end in double "quote"."""',
        )

        for prefix in docstring_prefixes:
            for docstring in docstrings:
                self.check_src_roundtrip(f"{prefix}{docstring}")

    def test_docstrings_negative_cases(self):
        # Test some cases that involve strings in the children of the
        # first node but aren't docstrings to make sure we don't have
        # False positives.
        docstrings_negative = (
            'a = """false"""',
            '"""false""" + """unless its optimized"""',
            '1 + 1\n"""false"""',
            'f"""no, top level but f-fstring"""'
        )
        for prefix in docstring_prefixes:
            for negative in docstrings_negative:
                # this cases should be result with single quote
                # rather then triple quoted docstring
                src = f"{prefix}{negative}"
                self.check_ast_roundtrip(src)
                self.check_src_dont_roundtrip(src)

    def test_unary_op_factor(self):
        for prefix in ("+", "-", "~"):
            self.check_src_roundtrip(f"{prefix}1")
        for prefix in ("not",):
            self.check_src_roundtrip(f"{prefix} 1")

    def test_slices(self):
        self.check_src_roundtrip("a[()]")
        self.check_src_roundtrip("a[1]")
        self.check_src_roundtrip("a[1, 2]")
        # Note that `a[*a]`, `a[*a,]`, and `a[(*a,)]` all evaluate to the same
        # thing at runtime and have the same AST, but only `a[*a,]` passes
        # this test, because that's what `ast.unparse` produces.
        self.check_src_roundtrip("a[*a,]")
        self.check_src_roundtrip("a[1, *a]")
        self.check_src_roundtrip("a[*a, 2]")
        self.check_src_roundtrip("a[1, *a, 2]")
        self.check_src_roundtrip("a[*a, *a]")
        self.check_src_roundtrip("a[1, *a, *a]")
        self.check_src_roundtrip("a[*a, 1, *a]")
        self.check_src_roundtrip("a[*a, *a, 1]")
        self.check_src_roundtrip("a[1, *a, *a, 2]")
        self.check_src_roundtrip("a[1:2, *a]")
        self.check_src_roundtrip("a[*a, 1:2]")

    def test_lambda_parameters(self):
        self.check_src_roundtrip("lambda: something")
        self.check_src_roundtrip("four = lambda: 2 + 2")
        self.check_src_roundtrip("lambda x: x * 2")
        self.check_src_roundtrip("square = lambda n: n ** 2")
        self.check_src_roundtrip("lambda x, y: x + y")
        self.check_src_roundtrip("add = lambda x, y: x + y")
        self.check_src_roundtrip("lambda x, y, /, z, q, *, u: None")
        self.check_src_roundtrip("lambda x, *y, **z: None")

    def test_star_expr_assign_target(self):
        for source_type, source in [
            ("single assignment", "{target} = foo"),
            ("multiple assignment", "{target} = {target} = bar"),
            ("for loop", "for {target} in foo:\n    pass"),
            ("async for loop", "async for {target} in foo:\n    pass")
        ]:
            for target in [
                "a",
                "a,",
                "a, b",
                "a, *b, c",
                "a, (b, c), d",
                "a, (b, c, d), *e",
                "a, (b, *c, d), e",
                "a, (b, *c, (d, e), f), g",
                "[a]",
                "[a, b]",
                "[a, *b, c]",
                "[a, [b, c], d]",
                "[a, [b, c, d], *e]",
                "[a, [b, *c, d], e]",
                "[a, [b, *c, [d, e], f], g]",
                "a, [b, c], d",
                "[a, b, (c, d), (e, f)]",
                "a, b, [*c], d, e"
            ]:
                with self.subTest(source_type=source_type, target=target):
                    self.check_src_roundtrip(source.format(target=target))

    def test_star_expr_assign_target_multiple(self):
        self.check_src_roundtrip("() = []")
        self.check_src_roundtrip("[] = ()")
        self.check_src_roundtrip("() = [a] = c, = [d] = e, f = () = g = h")
        self.check_src_roundtrip("a = b = c = d")
        self.check_src_roundtrip("a, b = c, d = e, f = g")
        self.check_src_roundtrip("[a, b] = [c, d] = [e, f] = g")
        self.check_src_roundtrip("a, b = [c, d] = e, f = g")

    def test_multiquote_joined_string(self):
        self.check_ast_roundtrip("f\"'''{1}\\\"\\\"\\\"\" ")
        self.check_ast_roundtrip("""f"'''{1}""\\"" """)
        self.check_ast_roundtrip("""f'""\"{1}''' """)
        self.check_ast_roundtrip("""f'""\"{1}""\\"' """)

        self.check_ast_roundtrip("""f"'''{"\\n"}""\\"" """)
        self.check_ast_roundtrip("""f'""\"{"\\n"}''' """)
        self.check_ast_roundtrip("""f'""\"{"\\n"}""\\"' """)

        self.check_ast_roundtrip("""f'''""\"''\\'{"\\n"}''' """)
        self.check_ast_roundtrip("""f'''""\"''\\'{"\\n\\"'"}''' """)
        self.check_ast_roundtrip("""f'''""\"''\\'{""\"\\n\\"'''""\" '''\\n'''}''' """)

    def test_backslash_in_format_spec(self):
        import re
        msg = re.escape('"\\ " is an invalid escape sequence. '
                        'Such sequences will not work in the future. '
                        'Did you mean "\\\\ "? A raw string is also an option.')
        with self.assertWarnsRegex(SyntaxWarning, msg):
            self.check_ast_roundtrip("""f"{x:\\ }" """)
        self.check_ast_roundtrip("""f"{x:\\n}" """)

        self.check_ast_roundtrip("""f"{x:\\\\ }" """)

        with self.assertWarnsRegex(SyntaxWarning, msg):
            self.check_ast_roundtrip("""f"{x:\\\\\\ }" """)
        self.check_ast_roundtrip("""f"{x:\\\\\\n}" """)

        self.check_ast_roundtrip("""f"{x:\\\\\\\\ }" """)

    def test_quote_in_format_spec(self):
        self.check_ast_roundtrip("""f"{x:'}" """)
        self.check_ast_roundtrip("""f"{x:\\'}" """)
        self.check_ast_roundtrip("""f"{x:\\\\'}" """)

        self.check_ast_roundtrip("""f'\\'{x:"}' """)
        self.check_ast_roundtrip("""f'\\'{x:\\"}' """)
        self.check_ast_roundtrip("""f'\\'{x:\\\\"}' """)

    def test_type_params(self):
        self.check_ast_roundtrip("type A = int")
        self.check_ast_roundtrip("type A[T] = int")
        self.check_ast_roundtrip("type A[T: int] = int")
        self.check_ast_roundtrip("type A[T = int] = int")
        self.check_ast_roundtrip("type A[T: int = int] = int")
        self.check_ast_roundtrip("type A[**P] = int")
        self.check_ast_roundtrip("type A[**P = int] = int")
        self.check_ast_roundtrip("type A[*Ts] = int")
        self.check_ast_roundtrip("type A[*Ts = int] = int")
        self.check_ast_roundtrip("type A[*Ts = *int] = int")
        self.check_ast_roundtrip("def f[T: int = int, **P = int, *Ts = *int]():\n    pass")
        self.check_ast_roundtrip("class C[T: int = int, **P = int, *Ts = *int]():\n    pass")

    def test_tstr(self):
        self.check_ast_roundtrip("t'{a +    b}'")
        self.check_ast_roundtrip("t'{a +    b:x}'")
        self.check_ast_roundtrip("t'{a +    b!s}'")
        self.check_ast_roundtrip("t'{ {a}}'")
        self.check_ast_roundtrip("t'{ {a}=}'")
        self.check_ast_roundtrip("t'{{a}}'")
        self.check_ast_roundtrip("t''")


class ManualASTCreationTestCase(unittest.TestCase):
    """Test that AST nodes created without a type_params field unparse correctly."""

    def test_class(self):
        node = ast.ClassDef(name="X", bases=[], keywords=[], body=[ast.Pass()], decorator_list=[])
        ast.fix_missing_locations(node)
        self.assertEqual(ast.unparse(node), "class X:\n    pass")

    def test_class_with_type_params(self):
        node = ast.ClassDef(name="X", bases=[], keywords=[], body=[ast.Pass()], decorator_list=[],
                             type_params=[ast.TypeVar("T")])
        ast.fix_missing_locations(node)
        self.assertEqual(ast.unparse(node), "class X[T]:\n    pass")

    def test_function(self):
        node = ast.FunctionDef(
            name="f",
            args=ast.arguments(posonlyargs=[], args=[], vararg=None, kwonlyargs=[], kw_defaults=[], kwarg=None, defaults=[]),
            body=[ast.Pass()],
            decorator_list=[],
            returns=None,
        )
        ast.fix_missing_locations(node)
        self.assertEqual(ast.unparse(node), "def f():\n    pass")

    def test_function_with_type_params(self):
        node = ast.FunctionDef(
            name="f",
            args=ast.arguments(posonlyargs=[], args=[], vararg=None, kwonlyargs=[], kw_defaults=[], kwarg=None, defaults=[]),
            body=[ast.Pass()],
            decorator_list=[],
            returns=None,
            type_params=[ast.TypeVar("T")],
        )
        ast.fix_missing_locations(node)
        self.assertEqual(ast.unparse(node), "def f[T]():\n    pass")

    def test_function_with_type_params_and_bound(self):
        node = ast.FunctionDef(
            name="f",
            args=ast.arguments(posonlyargs=[], args=[], vararg=None, kwonlyargs=[], kw_defaults=[], kwarg=None, defaults=[]),
            body=[ast.Pass()],
            decorator_list=[],
            returns=None,
            type_params=[ast.TypeVar("T", bound=ast.Name("int", ctx=ast.Load()))],
        )
        ast.fix_missing_locations(node)
        self.assertEqual(ast.unparse(node), "def f[T: int]():\n    pass")

    def test_function_with_type_params_and_default(self):
        node = ast.FunctionDef(
            name="f",
            args=ast.arguments(),
            body=[ast.Pass()],
            type_params=[
                ast.TypeVar("T", default_value=ast.Constant(value=1)),
                ast.TypeVarTuple("Ts", default_value=ast.Starred(value=ast.Constant(value=1), ctx=ast.Load())),
                ast.ParamSpec("P", default_value=ast.Constant(value=1)),
            ],
        )
        ast.fix_missing_locations(node)
        self.assertEqual(ast.unparse(node), "def f[T = 1, *Ts = *1, **P = 1]():\n    pass")

    def test_async_function(self):
        node = ast.AsyncFunctionDef(
            name="f",
            args=ast.arguments(posonlyargs=[], args=[], vararg=None, kwonlyargs=[], kw_defaults=[], kwarg=None, defaults=[]),
            body=[ast.Pass()],
            decorator_list=[],
            returns=None,
        )
        ast.fix_missing_locations(node)
        self.assertEqual(ast.unparse(node), "async def f():\n    pass")

    def test_async_function_with_type_params(self):
        node = ast.AsyncFunctionDef(
            name="f",
            args=ast.arguments(posonlyargs=[], args=[], vararg=None, kwonlyargs=[], kw_defaults=[], kwarg=None, defaults=[]),
            body=[ast.Pass()],
            decorator_list=[],
            returns=None,
            type_params=[ast.TypeVar("T")],
        )
        ast.fix_missing_locations(node)
        self.assertEqual(ast.unparse(node), "async def f[T]():\n    pass")

    def test_async_function_with_type_params_and_default(self):
        node = ast.AsyncFunctionDef(
            name="f",
            args=ast.arguments(),
            body=[ast.Pass()],
            type_params=[
                ast.TypeVar("T", default_value=ast.Constant(value=1)),
                ast.TypeVarTuple("Ts", default_value=ast.Starred(value=ast.Constant(value=1), ctx=ast.Load())),
                ast.ParamSpec("P", default_value=ast.Constant(value=1)),
            ],
        )
        ast.fix_missing_locations(node)
        self.assertEqual(ast.unparse(node), "async def f[T = 1, *Ts = *1, **P = 1]():\n    pass")


class DirectoryTestCase(ASTTestCase):
    """Test roundtrip behaviour on all files in Lib and Lib/test."""

    lib_dir = pathlib.Path(__file__).parent / ".."
    test_directories = (lib_dir, lib_dir / "test")
    run_always_files = {"test_grammar.py", "test_syntax.py", "test_compile.py",
                        "test_ast.py", "test_asdl_parser.py", "test_fstring.py",
                        "test_patma.py", "test_type_alias.py", "test_type_params.py",
                        "test_tokenize.py", "test_tstring.py"}

    _files_to_test = None

    @classmethod
    def files_to_test(cls):

        if cls._files_to_test is not None:
            return cls._files_to_test

        items = [
            item.resolve()
            for directory in cls.test_directories
            for item in directory.glob("*.py")
            if not item.name.startswith("bad")
        ]

        # Test limited subset of files unless the 'cpu' resource is specified.
        if not test.support.is_resource_enabled("cpu"):

            tests_to_run_always = {item for item in items if
                                   item.name in cls.run_always_files}

            items = set(random.sample(items, 10))

            # Make sure that at least tests that heavily use grammar features are
            # always considered in order to reduce the chance of missing something.
            items = list(items | tests_to_run_always)

        # bpo-31174: Store the names sample to always test the same files.
        # It prevents false alarms when hunting reference leaks.
        cls._files_to_test = items

        return items

    def test_files(self):
        with warnings.catch_warnings():
            warnings.simplefilter('ignore', SyntaxWarning)

            for item in self.files_to_test():
                if test.support.verbose:
                    print(f"Testing {item.absolute()}")

                with self.subTest(filename=item):
                    source = read_pyfile(item)
                    self.check_ast_roundtrip(source)


if __name__ == "__main__":
    unittest.main()
