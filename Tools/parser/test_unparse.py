import unittest
import test.support
import io
import os
import tokenize
import ast
import unparse

def read_pyfile(filename):
    """Read and return the contents of a Python source file (as a
    string), taking into account the file encoding."""
    with open(filename, "rb") as pyfile:
        encoding = tokenize.detect_encoding(pyfile.readline)[0]
    with open(filename, "r", encoding=encoding) as pyfile:
        source = pyfile.read()
    return source

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

class ASTTestCase(unittest.TestCase):
    def assertASTEqual(self, ast1, ast2):
        self.assertEqual(ast.dump(ast1), ast.dump(ast2))

    def check_roundtrip(self, code1, filename="internal"):
        ast1 = compile(code1, filename, "exec", ast.PyCF_ONLY_AST)
        unparse_buffer = io.StringIO()
        unparse.Unparser(ast1, unparse_buffer)
        code2 = unparse_buffer.getvalue()
        ast2 = compile(code2, filename, "exec", ast.PyCF_ONLY_AST)
        self.assertASTEqual(ast1, ast2)

class UnparseTestCase(ASTTestCase):
    # Tests for specific bugs found in earlier versions of unparse

    def test_del_statement(self):
        self.check_roundtrip("del x, y, z")

    def test_shifts(self):
        self.check_roundtrip("45 << 2")
        self.check_roundtrip("13 >> 7")

    def test_for_else(self):
        self.check_roundtrip(for_else)

    def test_while_else(self):
        self.check_roundtrip(while_else)

    def test_unary_parens(self):
        self.check_roundtrip("(-1)**7")
        self.check_roundtrip("(-1.)**8")
        self.check_roundtrip("(-1j)**6")
        self.check_roundtrip("not True or False")
        self.check_roundtrip("True or not False")

    def test_integer_parens(self):
        self.check_roundtrip("3 .__abs__()")

    def test_huge_float(self):
        self.check_roundtrip("1e1000")
        self.check_roundtrip("-1e1000")
        self.check_roundtrip("1e1000j")
        self.check_roundtrip("-1e1000j")

    def test_min_int(self):
        self.check_roundtrip(str(-2**31))
        self.check_roundtrip(str(-2**63))

    def test_imaginary_literals(self):
        self.check_roundtrip("7j")
        self.check_roundtrip("-7j")
        self.check_roundtrip("0j")
        self.check_roundtrip("-0j")

    def test_lambda_parentheses(self):
        self.check_roundtrip("(lambda: int)()")

    def test_chained_comparisons(self):
        self.check_roundtrip("1 < 4 <= 5")
        self.check_roundtrip("a is b is c is not d")

    def test_function_arguments(self):
        self.check_roundtrip("def f(): pass")
        self.check_roundtrip("def f(a): pass")
        self.check_roundtrip("def f(b = 2): pass")
        self.check_roundtrip("def f(a, b): pass")
        self.check_roundtrip("def f(a, b = 2): pass")
        self.check_roundtrip("def f(a = 5, b = 2): pass")
        self.check_roundtrip("def f(*, a = 1, b = 2): pass")
        self.check_roundtrip("def f(*, a = 1, b): pass")
        self.check_roundtrip("def f(*, a, b = 2): pass")
        self.check_roundtrip("def f(a, b = None, *, c, **kwds): pass")
        self.check_roundtrip("def f(a=2, *args, c=5, d, **kwds): pass")
        self.check_roundtrip("def f(*args, **kwargs): pass")

    def test_relative_import(self):
        self.check_roundtrip(relative_import)

    def test_nonlocal(self):
        self.check_roundtrip(nonlocal_ex)

    def test_raise_from(self):
        self.check_roundtrip(raise_from)

    def test_bytes(self):
        self.check_roundtrip("b'123'")

    def test_annotations(self):
        self.check_roundtrip("def f(a : int): pass")
        self.check_roundtrip("def f(a: int = 5): pass")
        self.check_roundtrip("def f(*args: [int]): pass")
        self.check_roundtrip("def f(**kwargs: dict): pass")
        self.check_roundtrip("def f() -> None: pass")

    def test_set_literal(self):
        self.check_roundtrip("{'a', 'b', 'c'}")

    def test_set_comprehension(self):
        self.check_roundtrip("{x for x in range(5)}")

    def test_dict_comprehension(self):
        self.check_roundtrip("{x: x*x for x in range(10)}")

    def test_class_decorators(self):
        self.check_roundtrip(class_decorator)

    def test_class_definition(self):
        self.check_roundtrip("class A(metaclass=type, *[], **{}): pass")

    def test_elifs(self):
        self.check_roundtrip(elif1)
        self.check_roundtrip(elif2)

    def test_try_except_finally(self):
        self.check_roundtrip(try_except_finally)

class DirectoryTestCase(ASTTestCase):
    """Test roundtrip behaviour on all files in Lib and Lib/test."""

    # test directories, relative to the root of the distribution
    test_directories = 'Lib', os.path.join('Lib', 'test')

    def test_files(self):
        # get names of files to test
        dist_dir = os.path.join(os.path.dirname(__file__), os.pardir, os.pardir)

        names = []
        for d in self.test_directories:
            test_dir = os.path.join(dist_dir, d)
            for n in os.listdir(test_dir):
                if n.endswith('.py') and not n.startswith('bad'):
                    names.append(os.path.join(test_dir, n))

        for filename in names:
            if test.support.verbose:
                print('Testing %s' % filename)
            source = read_pyfile(filename)
            self.check_roundtrip(source)


def test_main():
    test.support.run_unittest(UnparseTestCase, DirectoryTestCase)

if __name__ == '__main__':
    test_main()
