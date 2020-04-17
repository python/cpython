"""Tests for the unparse.py script in the Tools/parser directory."""

import unittest
import test.support
import pathlib
import random
import tokenize
import ast


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

docstring_prefixes = [
    "",
    "class foo():\n    ",
    "def foo():\n    ",
    "async def foo():\n    ",
]

class ASTTestCase(unittest.TestCase):
    def assertASTEqual(self, ast1, ast2):
        self.assertEqual(ast.dump(ast1), ast.dump(ast2))

    def check_ast_roundtrip(self, code1, **kwargs):
        ast1 = ast.parse(code1, **kwargs)
        code2 = ast.unparse(ast1)
        ast2 = ast.parse(code2, **kwargs)
        self.assertASTEqual(ast1, ast2)

    def check_invalid(self, node, raises=ValueError):
        self.assertRaises(raises, ast.unparse, node)

    def get_source(self, code1, code2=None, strip=True):
        code2 = code2 or code1
        code1 = ast.unparse(ast.parse(code1))
        if strip:
            code1 = code1.strip()
        return code1, code2

    def check_src_roundtrip(self, code1, code2=None, strip=True):
        code1, code2 = self.get_source(code1, code2, strip)
        self.assertEqual(code2, code1)

    def check_src_dont_roundtrip(self, code1, code2=None, strip=True):
        code1, code2 = self.get_source(code1, code2, strip)
        self.assertNotEqual(code2, code1)

class UnparseTestCase(ASTTestCase):
    # Tests for specific bugs found in earlier versions of unparse

    def test_fstrings(self):
        # See issue 25180
        self.check_ast_roundtrip(r"""f'{f"{0}"*3}'""")
        self.check_ast_roundtrip(r"""f'{f"{y}"*3}'""")

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

    def test_ext_slices(self):
        self.check_ast_roundtrip("a[i]")
        self.check_ast_roundtrip("a[i,]")
        self.check_ast_roundtrip("a[i, j]")
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
        self.check_invalid(ast.Raise(exc=None, cause=ast.Name(id="X")))

    def test_invalid_fstring_constant(self):
        self.check_invalid(ast.JoinedStr(values=[ast.Constant(value=100)]))

    def test_invalid_fstring_conversion(self):
        self.check_invalid(
            ast.FormattedValue(
                value=ast.Constant(value="a", kind=None),
                conversion=ord("Y"),  # random character
                format_spec=None,
            )
        )

    def test_invalid_set(self):
        self.check_invalid(ast.Set(elts=[]))

    def test_invalid_yield_from(self):
        self.check_invalid(ast.YieldFrom(value=None))

    def test_docstrings(self):
        docstrings = (
            'this ends with double quote"',
            'this includes a """triple quote"""'
        )
        for docstring in docstrings:
            # check as Module docstrings for easy testing
            self.check_ast_roundtrip(f"'{docstring}'")

    def test_constant_tuples(self):
        self.check_src_roundtrip(ast.Constant(value=(1,), kind=None), "(1,)")
        self.check_src_roundtrip(
            ast.Constant(value=(1, 2, 3), kind=None), "(1, 2, 3)"
        )

    def test_function_type(self):
        for function_type in (
            "() -> int",
            "(int, int) -> int",
            "(Callable[complex], More[Complex(call.to_typevar())]) -> None"
        ):
            self.check_ast_roundtrip(function_type, mode="func_type")


class CosmeticTestCase(ASTTestCase):
    """Test if there are cosmetic issues caused by unnecesary additions"""

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
        self.check_src_roundtrip("~ x")
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

    def test_docstrings(self):
        docstrings = (
            '"""simple doc string"""',
            '''"""A more complex one
            with some newlines"""''',
            '''"""Foo bar baz

            empty newline"""''',
            '"""With some \t"""',
            '"""Foo "bar" baz """',
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

class DirectoryTestCase(ASTTestCase):
    """Test roundtrip behaviour on all files in Lib and Lib/test."""

    lib_dir = pathlib.Path(__file__).parent / ".."
    test_directories = (lib_dir, lib_dir / "test")
    skip_files = {"test_fstring.py"}
    run_always_files = {"test_grammar.py", "test_syntax.py", "test_compile.py",
                        "test_ast.py", "test_asdl_parser.py"}

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
        for item in self.files_to_test():
            if test.support.verbose:
                print(f"Testing {item.absolute()}")

            # Some f-strings are not correctly round-tripped by
            # Tools/parser/unparse.py.  See issue 28002 for details.
            # We need to skip files that contain such f-strings.
            if item.name in self.skip_files:
                if test.support.verbose:
                    print(f"Skipping {item.absolute()}: see issue 28002")
                continue

            with self.subTest(filename=item):
                source = read_pyfile(item)
                self.check_ast_roundtrip(source)


if __name__ == "__main__":
    unittest.main()
