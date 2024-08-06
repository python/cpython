"""
Test the API of the symtable module.
"""

import textwrap
import symtable
import unittest



TEST_CODE = """
import sys

glob = 42
some_var = 12
some_non_assigned_global_var: int
some_assigned_global_var = 11

class Mine:
    instance_var = 24
    def a_method(p1, p2):
        pass

def spam(a, b, *var, **kw):
    global bar
    global some_assigned_global_var
    some_assigned_global_var = 12
    bar = 47
    some_var = 10
    x = 23
    glob
    def internal():
        return x
    def other_internal():
        nonlocal some_var
        some_var = 3
        return some_var
    return internal

def foo():
    pass

def namespace_test(): pass
def namespace_test(): pass

type Alias = int
type GenericAlias[T] = list[T]

def generic_spam[T](a):
    pass

class GenericMine[T: int]:
    pass
"""

TEST_COMPLEX_CLASS_CODE = """
# The following symbols are defined in ComplexClass
# without being introduced by a 'global' statement.
glob_unassigned_meth: Any
glob_unassigned_meth_pep_695: Any

glob_unassigned_async_meth: Any
glob_unassigned_async_meth_pep_695: Any

def glob_assigned_meth(): pass
def glob_assigned_meth_pep_695[T](): pass

async def glob_assigned_async_meth(): pass
async def glob_assigned_async_meth_pep_695[T](): pass

# The following symbols are defined in ComplexClass after
# being introduced by a 'global' statement (and therefore
# are not considered as local symbols of ComplexClass).
glob_unassigned_meth_ignore: Any
glob_unassigned_meth_pep_695_ignore: Any

glob_unassigned_async_meth_ignore: Any
glob_unassigned_async_meth_pep_695_ignore: Any

def glob_assigned_meth_ignore(): pass
def glob_assigned_meth_pep_695_ignore[T](): pass

async def glob_assigned_async_meth_ignore(): pass
async def glob_assigned_async_meth_pep_695_ignore[T](): pass

class ComplexClass:
    a_var = 1234
    a_genexpr = (x for x in [])
    a_lambda = lambda x: x

    type a_type_alias = int
    type a_type_alias_pep_695[T] = list[T]

    class a_class: pass
    class a_class_pep_695[T]: pass

    def a_method(self): pass
    def a_method_pep_695[T](self): pass

    async def an_async_method(self): pass
    async def an_async_method_pep_695[T](self): pass

    @classmethod
    def a_classmethod(cls): pass
    @classmethod
    def a_classmethod_pep_695[T](self): pass

    @classmethod
    async def an_async_classmethod(cls): pass
    @classmethod
    async def an_async_classmethod_pep_695[T](self): pass

    @staticmethod
    def a_staticmethod(): pass
    @staticmethod
    def a_staticmethod_pep_695[T](self): pass

    @staticmethod
    async def an_async_staticmethod(): pass
    @staticmethod
    async def an_async_staticmethod_pep_695[T](self): pass

    # These ones will be considered as methods because of the 'def' although
    # they are *not* valid methods at runtime since they are not decorated
    # with @staticmethod.
    def a_fakemethod(): pass
    def a_fakemethod_pep_695[T](): pass

    async def an_async_fakemethod(): pass
    async def an_async_fakemethod_pep_695[T](): pass

    # Check that those are still considered as methods
    # since they are not using the 'global' keyword.
    def glob_unassigned_meth(): pass
    def glob_unassigned_meth_pep_695[T](): pass

    async def glob_unassigned_async_meth(): pass
    async def glob_unassigned_async_meth_pep_695[T](): pass

    def glob_assigned_meth(): pass
    def glob_assigned_meth_pep_695[T](): pass

    async def glob_assigned_async_meth(): pass
    async def glob_assigned_async_meth_pep_695[T](): pass

    # The following are not picked as local symbols because they are not
    # visible by the class at runtime (this is equivalent to having the
    # definitions outside of the class).
    global glob_unassigned_meth_ignore
    def glob_unassigned_meth_ignore(): pass
    global glob_unassigned_meth_pep_695_ignore
    def glob_unassigned_meth_pep_695_ignore[T](): pass

    global glob_unassigned_async_meth_ignore
    async def glob_unassigned_async_meth_ignore(): pass
    global glob_unassigned_async_meth_pep_695_ignore
    async def glob_unassigned_async_meth_pep_695_ignore[T](): pass

    global glob_assigned_meth_ignore
    def glob_assigned_meth_ignore(): pass
    global glob_assigned_meth_pep_695_ignore
    def glob_assigned_meth_pep_695_ignore[T](): pass

    global glob_assigned_async_meth_ignore
    async def glob_assigned_async_meth_ignore(): pass
    global glob_assigned_async_meth_pep_695_ignore
    async def glob_assigned_async_meth_pep_695_ignore[T](): pass
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
    other_internal = find_block(spam, "other_internal")
    foo = find_block(top, "foo")
    Alias = find_block(top, "Alias")
    GenericAlias = find_block(top, "GenericAlias")
    GenericAlias_inner = find_block(GenericAlias, "GenericAlias")
    generic_spam = find_block(top, "generic_spam")
    generic_spam_inner = find_block(generic_spam, "generic_spam")
    GenericMine = find_block(top, "GenericMine")
    GenericMine_inner = find_block(GenericMine, "GenericMine")
    T = find_block(GenericMine, "T")

    def test_type(self):
        self.assertEqual(self.top.get_type(), "module")
        self.assertEqual(self.Mine.get_type(), "class")
        self.assertEqual(self.a_method.get_type(), "function")
        self.assertEqual(self.spam.get_type(), "function")
        self.assertEqual(self.internal.get_type(), "function")
        self.assertEqual(self.foo.get_type(), "function")
        self.assertEqual(self.Alias.get_type(), "type alias")
        self.assertEqual(self.GenericAlias.get_type(), "type parameter")
        self.assertEqual(self.GenericAlias_inner.get_type(), "type alias")
        self.assertEqual(self.generic_spam.get_type(), "type parameter")
        self.assertEqual(self.generic_spam_inner.get_type(), "function")
        self.assertEqual(self.GenericMine.get_type(), "type parameter")
        self.assertEqual(self.GenericMine_inner.get_type(), "class")
        self.assertEqual(self.T.get_type(), "TypeVar bound")

    def test_id(self):
        self.assertGreater(self.top.get_id(), 0)
        self.assertGreater(self.Mine.get_id(), 0)
        self.assertGreater(self.a_method.get_id(), 0)
        self.assertGreater(self.spam.get_id(), 0)
        self.assertGreater(self.internal.get_id(), 0)
        self.assertGreater(self.foo.get_id(), 0)
        self.assertGreater(self.Alias.get_id(), 0)
        self.assertGreater(self.GenericAlias.get_id(), 0)
        self.assertGreater(self.generic_spam.get_id(), 0)
        self.assertGreater(self.GenericMine.get_id(), 0)

    def test_optimized(self):
        self.assertFalse(self.top.is_optimized())

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
        self.assertEqual(self.spam.get_lineno(), 14)

    def test_function_info(self):
        func = self.spam
        self.assertEqual(sorted(func.get_parameters()), ["a", "b", "kw", "var"])
        expected = ['a', 'b', 'internal', 'kw', 'other_internal', 'some_var', 'var', 'x']
        self.assertEqual(sorted(func.get_locals()), expected)
        self.assertEqual(sorted(func.get_globals()), ["bar", "glob", "some_assigned_global_var"])
        self.assertEqual(self.internal.get_frees(), ("x",))

    def test_globals(self):
        self.assertTrue(self.spam.lookup("glob").is_global())
        self.assertFalse(self.spam.lookup("glob").is_declared_global())
        self.assertTrue(self.spam.lookup("bar").is_global())
        self.assertTrue(self.spam.lookup("bar").is_declared_global())
        self.assertFalse(self.internal.lookup("x").is_global())
        self.assertFalse(self.Mine.lookup("instance_var").is_global())
        self.assertTrue(self.spam.lookup("bar").is_global())
        # Module-scope globals are both global and local
        self.assertTrue(self.top.lookup("some_non_assigned_global_var").is_global())
        self.assertTrue(self.top.lookup("some_assigned_global_var").is_global())

    def test_nonlocal(self):
        self.assertFalse(self.spam.lookup("some_var").is_nonlocal())
        self.assertTrue(self.other_internal.lookup("some_var").is_nonlocal())
        expected = ("some_var",)
        self.assertEqual(self.other_internal.get_nonlocals(), expected)

    def test_local(self):
        self.assertTrue(self.spam.lookup("x").is_local())
        self.assertFalse(self.spam.lookup("bar").is_local())
        # Module-scope globals are both global and local
        self.assertTrue(self.top.lookup("some_non_assigned_global_var").is_local())
        self.assertTrue(self.top.lookup("some_assigned_global_var").is_local())

    def test_free(self):
        self.assertTrue(self.internal.lookup("x").is_free())

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

        ns_test_2 = self.top.lookup("glob")
        self.assertEqual(len(ns_test_2.get_namespaces()), 0)
        self.assertRaises(ValueError, ns_test_2.get_namespace)

    def test_assigned(self):
        self.assertTrue(self.spam.lookup("x").is_assigned())
        self.assertTrue(self.spam.lookup("bar").is_assigned())
        self.assertTrue(self.top.lookup("spam").is_assigned())
        self.assertTrue(self.Mine.lookup("a_method").is_assigned())
        self.assertFalse(self.internal.lookup("x").is_assigned())

    def test_annotated(self):
        st1 = symtable.symtable('def f():\n    x: int\n', 'test', 'exec')
        st2 = st1.get_children()[0]
        self.assertTrue(st2.lookup('x').is_local())
        self.assertTrue(st2.lookup('x').is_annotated())
        self.assertFalse(st2.lookup('x').is_global())
        st3 = symtable.symtable('def f():\n    x = 1\n', 'test', 'exec')
        st4 = st3.get_children()[0]
        self.assertTrue(st4.lookup('x').is_local())
        self.assertFalse(st4.lookup('x').is_annotated())

        # Test that annotations in the global scope are valid after the
        # variable is declared as nonlocal.
        st5 = symtable.symtable('global x\nx: int', 'test', 'exec')
        self.assertTrue(st5.lookup("x").is_global())

        # Test that annotations for nonlocals are valid after the
        # variable is declared as nonlocal.
        st6 = symtable.symtable('def g():\n'
                                '    x = 2\n'
                                '    def f():\n'
                                '        nonlocal x\n'
                                '    x: int',
                                'test', 'exec')

    def test_imported(self):
        self.assertTrue(self.top.lookup("sys").is_imported())

    def test_name(self):
        self.assertEqual(self.top.get_name(), "top")
        self.assertEqual(self.spam.get_name(), "spam")
        self.assertEqual(self.spam.lookup("x").get_name(), "x")
        self.assertEqual(self.Mine.get_name(), "Mine")

    def test_class_get_methods(self):
        self.assertEqual(self.Mine.get_methods(), ('a_method',))

        top = symtable.symtable(TEST_COMPLEX_CLASS_CODE, "?", "exec")
        this = find_block(top, "ComplexClass")

        self.assertEqual(this.get_methods(), (
            'a_method', 'a_method_pep_695',
            'an_async_method', 'an_async_method_pep_695',
            'a_classmethod', 'a_classmethod_pep_695',
            'an_async_classmethod', 'an_async_classmethod_pep_695',
            'a_staticmethod', 'a_staticmethod_pep_695',
            'an_async_staticmethod', 'an_async_staticmethod_pep_695',
            'a_fakemethod', 'a_fakemethod_pep_695',
            'an_async_fakemethod', 'an_async_fakemethod_pep_695',
            'glob_unassigned_meth', 'glob_unassigned_meth_pep_695',
            'glob_unassigned_async_meth', 'glob_unassigned_async_meth_pep_695',
            'glob_assigned_meth', 'glob_assigned_meth_pep_695',
            'glob_assigned_async_meth', 'glob_assigned_async_meth_pep_695',
        ))

        # Test generator expressions that are of type TYPE_FUNCTION
        # but will not be reported by get_methods() since they are
        # not functions per se.
        #
        # Other kind of comprehensions such as list, set or dict
        # expressions do not have the TYPE_FUNCTION type.

        def check_body(body, expected_methods):
            indented = textwrap.indent(body, ' ' * 4)
            top = symtable.symtable(f"class A:\n{indented}", "?", "exec")
            this = find_block(top, "A")
            self.assertEqual(this.get_methods(), expected_methods)

        # statements with 'genexpr' inside it
        GENEXPRS = (
            'x = (x for x in [])',
            'x = (x async for x in [])',
            'genexpr = (x for x in [])',
            'genexpr = (x async for x in [])',
        )

        for gen in GENEXPRS:
            # test generator expression
            with self.subTest(gen=gen):
                check_body(gen, ())

            # test generator expression + variable named 'genexpr'
            with self.subTest(gen=gen, isvar=True):
                check_body('\n'.join((gen, 'genexpr = 1')), ())
                check_body('\n'.join(('genexpr = 1', gen)), ())

        for paramlist in ('()', '(x)', '(x, y)', '(z: T)'):
            for func in (
                f'def genexpr{paramlist}:pass',
                f'async def genexpr{paramlist}:pass',
                f'def genexpr[T]{paramlist}:pass',
                f'async def genexpr[T]{paramlist}:pass',
            ):
                with self.subTest(func=func):
                    # test function named 'genexpr'
                    check_body(func, ('genexpr',))

                for gen in GENEXPRS:
                    with self.subTest(gen=gen, func=func):
                        # test generator expression + function named 'genexpr'
                        check_body('\n'.join((gen, func)), ('genexpr',))
                        check_body('\n'.join((func, gen)), ('genexpr',))

    def test_filename_correct(self):
        ### Bug tickler: SyntaxError file name correct whether error raised
        ### while parsing or building symbol table.
        def checkfilename(brokencode, offset):
            try:
                symtable.symtable(brokencode, "spam", "exec")
            except SyntaxError as e:
                self.assertEqual(e.filename, "spam")
                self.assertEqual(e.lineno, 1)
                self.assertEqual(e.offset, offset)
            else:
                self.fail("no SyntaxError for %r" % (brokencode,))
        checkfilename("def f(x): foo)(", 14)  # parse-time
        checkfilename("def f(x): global x", 11)  # symtable-build-time
        symtable.symtable("pass", b"spam", "exec")
        with self.assertRaises(TypeError):
            symtable.symtable("pass", bytearray(b"spam"), "exec")
        with self.assertRaises(TypeError):
            symtable.symtable("pass", memoryview(b"spam"), "exec")
        with self.assertRaises(TypeError):
            symtable.symtable("pass", list(b"spam"), "exec")

    def test_eval(self):
        symbols = symtable.symtable("42", "?", "eval")

    def test_single(self):
        symbols = symtable.symtable("42", "?", "single")

    def test_exec(self):
        symbols = symtable.symtable("def f(x): return x", "?", "exec")

    def test_bytes(self):
        top = symtable.symtable(TEST_CODE.encode('utf8'), "?", "exec")
        self.assertIsNotNone(find_block(top, "Mine"))

        code = b'# -*- coding: iso8859-15 -*-\nclass \xb4: pass\n'

        top = symtable.symtable(code, "?", "exec")
        self.assertIsNotNone(find_block(top, "\u017d"))

    def test_symtable_repr(self):
        self.assertEqual(str(self.top), "<SymbolTable for module ?>")
        self.assertEqual(str(self.spam), "<Function SymbolTable for spam in ?>")

    def test_symtable_entry_repr(self):
        expected = f"<symtable entry top({self.top.get_id()}), line {self.top.get_lineno()}>"
        self.assertEqual(repr(self.top._table), expected)


if __name__ == '__main__':
    unittest.main()
