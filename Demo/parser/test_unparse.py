import unittest
from test import test_support

import cStringIO
import ast
import _ast
import unparse

forelse = """\
def f():
    for x in range(10):
        break
    else:
        y = 2
    z = 3
"""

whileelse = """\
def g():
    while True:
        break
    else:
        y = 2
    z = 3
"""

class UnparseTestCase(unittest.TestCase):
    # Tests for specific bugs found in earlier versions of unparse

    def check_roundtrip(self, code1, filename="internal"):
        ast1 = compile(code1, filename, "exec", _ast.PyCF_ONLY_AST)
        unparse_buffer = cStringIO.StringIO()
        unparse.Unparser(ast1, unparse_buffer)
        code2 = unparse_buffer.getvalue()
        ast2 = compile(code2, filename, "exec", _ast.PyCF_ONLY_AST)
        self.assertEqual(ast.dump(ast1), ast.dump(ast2))

    def test_del_statement(self):
        self.check_roundtrip("del x, y, z")

    def test_shifts(self):
        self.check_roundtrip("45 << 2")
        self.check_roundtrip("13 >> 7")

    def test_for_else(self):
        self.check_roundtrip(forelse)

    def test_while_else(self):
        self.check_roundtrip(forelse)

    def test_unary_parens(self):
        self.check_roundtrip("(-1)**7")
        self.check_roundtrip("not True or False")
        self.check_roundtrip("True or not False")


def test_main():
    test_support.run_unittest(UnparseTestCase)

if __name__ == '__main__':
    test_main()
