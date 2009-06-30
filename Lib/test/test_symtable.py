"""
Test the API of the symtable module.
"""
import symtable
import unittest

from test import support


TEST_CODE = """
import sys

glob = 42

class Mine:
    instance_var = 24
    def a_method(p1, p2):
        pass

def spam(a, b, *var, **kw):
    global bar
    bar = 47
    x = 23
    glob
    def internal():
        return x
    return internal

def foo():
    pass

def namespace_test(): pass
def namespace_test(): pass
"""


def find_block(block, name):
    for ch in block.get_children():
        if ch.get_name() == name:
            return ch


class SymtableTest(unittest.TestCase):

    top = symtable.symtable(TEST_CODE, "?", "exec")
    # These correspond to scopes in TEST_CODE
    Mine = find_block(top, "Mine")
    a_method = find_block(Mine, "a_method")
    spam = find_block(top, "spam")
    internal = find_block(spam, "internal")
    foo = find_block(top, "foo")

    def test_type(self):
        self.assertEqual(self.top.get_type(), "module")
        self.assertEqual(self.Mine.get_type(), "class")
        self.assertEqual(self.a_method.get_type(), "function")
        self.assertEqual(self.spam.get_type(), "function")
        self.assertEqual(self.internal.get_type(), "function")

    def test_optimized(self):
        self.assertFalse(self.top.is_optimized())
        self.assertFalse(self.top.has_exec())

        self.assertTrue(self.spam.is_optimized())

    def test_nested(self):
        self.assertFalse(self.top.is_nested())
        self.assertFalse(self.Mine.is_nested())
        self.assertFalse(self.spam.is_nested())
        self.assertTrue(self.internal.is_nested())

    def test_children(self):
        self.assertTrue(self.top.has_children())
        self.assertTrue(self.Mine.has_children())
        self.assertFalse(self.foo.has_children())

    def test_lineno(self):
        self.assertEqual(self.top.get_lineno(), 0)
        self.assertEqual(self.spam.get_lineno(), 11)

    def test_function_info(self):
        func = self.spam
        self.assertEqual(func.get_parameters(), ("a", "b", "kw", "var"))
        self.assertEqual(func.get_locals(),
                         ("a", "b", "internal", "kw", "var", "x"))
        self.assertEqual(func.get_globals(), ("bar", "glob"))
        self.assertEqual(self.internal.get_frees(), ("x",))

    def test_globals(self):
        self.assertTrue(self.spam.lookup("glob").is_global())
        self.assertFalse(self.spam.lookup("glob").is_declared_global())
        self.assertTrue(self.spam.lookup("bar").is_global())
        self.assertTrue(self.spam.lookup("bar").is_declared_global())
        self.assertFalse(self.internal.lookup("x").is_global())
        self.assertFalse(self.Mine.lookup("instance_var").is_global())

    def test_local(self):
        self.assertTrue(self.spam.lookup("x").is_local())
        self.assertFalse(self.internal.lookup("x").is_local())

    def test_referenced(self):
        self.assertTrue(self.internal.lookup("x").is_referenced())
        self.assertTrue(self.spam.lookup("internal").is_referenced())
        self.assertFalse(self.spam.lookup("x").is_referenced())

    def test_parameters(self):
        for sym in ("a", "var", "kw"):
            self.assertTrue(self.spam.lookup(sym).is_parameter())
        self.assertFalse(self.spam.lookup("x").is_parameter())

    def test_symbol_lookup(self):
        self.assertEqual(len(self.top.get_identifiers()),
                         len(self.top.get_symbols()))

        self.assertRaises(KeyError, self.top.lookup, "not_here")

    def test_namespaces(self):
        self.assertTrue(self.top.lookup("Mine").is_namespace())
        self.assertTrue(self.Mine.lookup("a_method").is_namespace())
        self.assertTrue(self.top.lookup("spam").is_namespace())
        self.assertTrue(self.spam.lookup("internal").is_namespace())
        self.assertTrue(self.top.lookup("namespace_test").is_namespace())
        self.assertFalse(self.spam.lookup("x").is_namespace())

        self.assertTrue(self.top.lookup("spam").get_namespace() is self.spam)
        ns_test = self.top.lookup("namespace_test")
        self.assertEqual(len(ns_test.get_namespaces()), 2)
        self.assertRaises(ValueError, ns_test.get_namespace)

    def test_assigned(self):
        self.assertTrue(self.spam.lookup("x").is_assigned())
        self.assertTrue(self.spam.lookup("bar").is_assigned())
        self.assertTrue(self.top.lookup("spam").is_assigned())
        self.assertTrue(self.Mine.lookup("a_method").is_assigned())
        self.assertFalse(self.internal.lookup("x").is_assigned())

    def test_imported(self):
        self.assertTrue(self.top.lookup("sys").is_imported())

    def test_name(self):
        self.assertEqual(self.top.get_name(), "top")
        self.assertEqual(self.spam.get_name(), "spam")
        self.assertEqual(self.spam.lookup("x").get_name(), "x")
        self.assertEqual(self.Mine.get_name(), "Mine")

    def test_class_info(self):
        self.assertEqual(self.Mine.get_methods(), ('a_method',))

    def test_filename_correct(self):
        ### Bug tickler: SyntaxError file name correct whether error raised
        ### while parsing or building symbol table.
        def checkfilename(brokencode):
            try:
                symtable.symtable(brokencode, "spam", "exec")
            except SyntaxError as e:
                self.assertEqual(e.filename, "spam")
            else:
                self.fail("no SyntaxError for %r" % (brokencode,))
        checkfilename("def f(x): foo)(")  # parse-time
        checkfilename("def f(x): global x")  # symtable-build-time

    def test_eval(self):
        symbols = symtable.symtable("42", "?", "eval")

    def test_single(self):
        symbols = symtable.symtable("42", "?", "single")

    def test_exec(self):
        symbols = symtable.symtable("def f(x): return x", "?", "exec")


def test_main():
    support.run_unittest(SymtableTest)

if __name__ == '__main__':
    test_main()
