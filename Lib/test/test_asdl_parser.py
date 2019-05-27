"""Tests for the asdl parser in Parser/asdl.py"""

import importlib.machinery
import os
from os.path import dirname
import sys
import sysconfig
import unittest


# This test is only relevant for from-source builds of Python.
if not sysconfig.is_python_build():
    raise unittest.SkipTest('test irrelevant for an installed Python')

src_base = dirname(dirname(dirname(__file__)))
parser_dir = os.path.join(src_base, 'Parser')


class TestAsdlParser(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Loads the asdl module dynamically, since it's not in a real importable
        # package.
        # Parses Python.asdl into an ast.Module and run the check on it.
        # There's no need to do this for each test method, hence setUpClass.
        sys.path.insert(0, parser_dir)
        loader = importlib.machinery.SourceFileLoader(
                'asdl', os.path.join(parser_dir, 'asdl.py'))
        cls.asdl = loader.load_module()
        cls.mod = cls.asdl.parse(os.path.join(parser_dir, 'Python.asdl'))
        cls.assertTrue(cls.asdl.check(cls.mod), 'Module validation failed')

    @classmethod
    def tearDownClass(cls):
        del sys.path[0]

    def setUp(self):
        # alias stuff from the class, for convenience
        self.asdl = TestAsdlParser.asdl
        self.mod = TestAsdlParser.mod
        self.types = self.mod.types

    def test_module(self):
        self.assertEqual(self.mod.name, 'Python')
        self.assertIn('stmt', self.types)
        self.assertIn('expr', self.types)
        self.assertIn('mod', self.types)

    def test_definitions(self):
        defs = self.mod.dfns
        self.assertIsInstance(defs[0], self.asdl.Type)
        self.assertIsInstance(defs[0].value, self.asdl.Sum)

        self.assertIsInstance(self.types['withitem'], self.asdl.Product)
        self.assertIsInstance(self.types['alias'], self.asdl.Product)

    def test_product(self):
        alias = self.types['alias']
        self.assertEqual(
            str(alias),
            'Product([Field(identifier, name), Field(identifier, asname, opt=True)])')

    def test_attributes(self):
        stmt = self.types['stmt']
        self.assertEqual(len(stmt.attributes), 4)
        self.assertEqual(str(stmt.attributes[0]), 'Field(int, lineno)')
        self.assertEqual(str(stmt.attributes[1]), 'Field(int, col_offset)')
        self.assertEqual(str(stmt.attributes[2]), 'Field(int, end_lineno, opt=True)')
        self.assertEqual(str(stmt.attributes[3]), 'Field(int, end_col_offset, opt=True)')

    def test_constructor_fields(self):
        ehandler = self.types['excepthandler']
        self.assertEqual(len(ehandler.types), 1)
        self.assertEqual(len(ehandler.attributes), 4)

        cons = ehandler.types[0]
        self.assertIsInstance(cons, self.asdl.Constructor)
        self.assertEqual(len(cons.fields), 3)

        f0 = cons.fields[0]
        self.assertEqual(f0.type, 'expr')
        self.assertEqual(f0.name, 'type')
        self.assertTrue(f0.opt)

        f1 = cons.fields[1]
        self.assertEqual(f1.type, 'identifier')
        self.assertEqual(f1.name, 'name')
        self.assertTrue(f1.opt)

        f2 = cons.fields[2]
        self.assertEqual(f2.type, 'stmt')
        self.assertEqual(f2.name, 'body')
        self.assertFalse(f2.opt)
        self.assertTrue(f2.seq)

    def test_visitor(self):
        class CustomVisitor(self.asdl.VisitorBase):
            def __init__(self):
                super().__init__()
                self.names_with_seq = []

            def visitModule(self, mod):
                for dfn in mod.dfns:
                    self.visit(dfn)

            def visitType(self, type):
                self.visit(type.value)

            def visitSum(self, sum):
                for t in sum.types:
                    self.visit(t)

            def visitConstructor(self, cons):
                for f in cons.fields:
                    if f.seq:
                        self.names_with_seq.append(cons.name)

        v = CustomVisitor()
        v.visit(self.types['mod'])
        self.assertEqual(v.names_with_seq,
                         ['Module', 'Module', 'Interactive', 'FunctionType', 'Suite'])


if __name__ == '__main__':
    unittest.main()
