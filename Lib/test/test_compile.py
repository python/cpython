import unittest
import warnings
import sys
from test import test_support

class TestSpecifics(unittest.TestCase):

    def test_debug_assignment(self):
        # catch assignments to __debug__
        self.assertRaises(SyntaxError, compile, '__debug__ = 1', '?', 'single')
        import __builtin__
        prev = __builtin__.__debug__
        setattr(__builtin__, '__debug__', 'sure')
        setattr(__builtin__, '__debug__', prev)

    def test_argument_handling(self):
        # detect duplicate positional and keyword arguments
        self.assertRaises(SyntaxError, eval, 'lambda a,a:0')
        self.assertRaises(SyntaxError, eval, 'lambda a,a=1:0')
        self.assertRaises(SyntaxError, eval, 'lambda a=1,a=1:0')
        try:
            exec 'def f(a, a): pass'
            self.fail("duplicate arguments")
        except SyntaxError:
            pass
        try:
            exec 'def f(a = 0, a = 1): pass'
            self.fail("duplicate keyword arguments")
        except SyntaxError:
            pass
        try:
            exec 'def f(a): global a; a = 1'
            self.fail("variable is global and local")
        except SyntaxError:
            pass

    def test_syntax_error(self):
        self.assertRaises(SyntaxError, compile, "1+*3", "filename", "exec")

    def test_none_keyword_arg(self):
        self.assertRaises(SyntaxError, compile, "f(None=1)", "<string>", "exec")

    def test_duplicate_global_local(self):
        try:
            exec 'def f(a): global a; a = 1'
            self.fail("variable is global and local")
        except SyntaxError:
            pass

    def test_exec_with_general_mapping_for_locals(self):

        class M:
            "Test mapping interface versus possible calls from eval()."
            def __getitem__(self, key):
                if key == 'a':
                    return 12
                raise KeyError
            def __setitem__(self, key, value):
                self.results = (key, value)
            def keys(self):
                return list('xyz')

        m = M()
        g = globals()
        exec 'z = a' in g, m
        self.assertEqual(m.results, ('z', 12))
        try:
            exec 'z = b' in g, m
        except NameError:
            pass
        else:
            self.fail('Did not detect a KeyError')
        exec 'z = dir()' in g, m
        self.assertEqual(m.results, ('z', list('xyz')))
        exec 'z = globals()' in g, m
        self.assertEqual(m.results, ('z', g))
        exec 'z = locals()' in g, m
        self.assertEqual(m.results, ('z', m))
        try:
            exec 'z = b' in m
        except TypeError:
            pass
        else:
            self.fail('Did not validate globals as a real dict')

        class A:
            "Non-mapping"
            pass
        m = A()
        try:
            exec 'z = a' in g, m
        except TypeError:
            pass
        else:
            self.fail('Did not validate locals as a mapping')

        # Verify that dict subclasses work as well
        class D(dict):
            def __getitem__(self, key):
                if key == 'a':
                    return 12
                return dict.__getitem__(self, key)
        d = D()
        exec 'z = a' in g, d
        self.assertEqual(d['z'], 12)

    def test_extended_arg(self):
        longexpr = 'x = x or ' + '-x' * 2500
        code = '''
def f(x):
    %s
    %s
    %s
    %s
    %s
    %s
    %s
    %s
    %s
    %s
    # the expressions above have no effect, x == argument
    while x:
        x -= 1
        # EXTENDED_ARG/JUMP_ABSOLUTE here
    return x
''' % ((longexpr,)*10)
        exec code
        self.assertEqual(f(5), 0)

    def test_complex_args(self):

        def comp_args((a, b)):
            return a,b
        self.assertEqual(comp_args((1, 2)), (1, 2))

        def comp_args((a, b)=(3, 4)):
            return a, b
        self.assertEqual(comp_args((1, 2)), (1, 2))
        self.assertEqual(comp_args(), (3, 4))

        def comp_args(a, (b, c)):
            return a, b, c
        self.assertEqual(comp_args(1, (2, 3)), (1, 2, 3))

        def comp_args(a=2, (b, c)=(3, 4)):
            return a, b, c
        self.assertEqual(comp_args(1, (2, 3)), (1, 2, 3))
        self.assertEqual(comp_args(), (2, 3, 4))

    def test_argument_order(self):
        try:
            exec 'def f(a=1, (b, c)): pass'
            self.fail("non-default args after default")
        except SyntaxError:
            pass

    def test_float_literals(self):
        # testing bad float literals
        self.assertRaises(SyntaxError, eval, "2e")
        self.assertRaises(SyntaxError, eval, "2.0e+")
        self.assertRaises(SyntaxError, eval, "1e-")
        self.assertRaises(SyntaxError, eval, "3-4e/21")

    def test_indentation(self):
        # testing compile() of indented block w/o trailing newline"
        s = """
if 1:
    if 2:
        pass"""
        compile(s, "<string>", "exec")

    # This test is probably specific to CPython and may not generalize
    # to other implementations.  We are trying to ensure that when
    # the first line of code starts after 256, correct line numbers
    # in tracebacks are still produced.
    def test_leading_newlines(self):
        s256 = "".join(["\n"] * 256 + ["spam"])
        co = compile(s256, 'fn', 'exec')
        self.assertEqual(co.co_firstlineno, 257)
        self.assertEqual(co.co_lnotab, '')

    def test_literals_with_leading_zeroes(self):
        for arg in ["077787", "0xj", "0x.", "0e",  "090000000000000",
                    "080000000000000", "000000000000009", "000000000000008"]:
            self.assertRaises(SyntaxError, eval, arg)

        self.assertEqual(eval("0777"), 511)
        self.assertEqual(eval("0777L"), 511)
        self.assertEqual(eval("000777"), 511)
        self.assertEqual(eval("0xff"), 255)
        self.assertEqual(eval("0xffL"), 255)
        self.assertEqual(eval("0XfF"), 255)
        self.assertEqual(eval("0777."), 777)
        self.assertEqual(eval("0777.0"), 777)
        self.assertEqual(eval("000000000000000000000000000000000000000000000000000777e0"), 777)
        self.assertEqual(eval("0777e1"), 7770)
        self.assertEqual(eval("0e0"), 0)
        self.assertEqual(eval("0000E-012"), 0)
        self.assertEqual(eval("09.5"), 9.5)
        self.assertEqual(eval("0777j"), 777j)
        self.assertEqual(eval("00j"), 0j)
        self.assertEqual(eval("00.0"), 0)
        self.assertEqual(eval("0e3"), 0)
        self.assertEqual(eval("090000000000000."), 90000000000000.)
        self.assertEqual(eval("090000000000000.0000000000000000000000"), 90000000000000.)
        self.assertEqual(eval("090000000000000e0"), 90000000000000.)
        self.assertEqual(eval("090000000000000e-0"), 90000000000000.)
        self.assertEqual(eval("090000000000000j"), 90000000000000j)
        self.assertEqual(eval("000000000000007"), 7)
        self.assertEqual(eval("000000000000008."), 8.)
        self.assertEqual(eval("000000000000009."), 9.)

    def test_unary_minus(self):
        # Verify treatment of unary minus on negative numbers SF bug #660455
        if sys.maxint == 2147483647:
            # 32-bit machine
            all_one_bits = '0xffffffff'
            self.assertEqual(eval(all_one_bits), 4294967295L)
            self.assertEqual(eval("-" + all_one_bits), -4294967295L)
        elif sys.maxint == 9223372036854775807:
            # 64-bit machine
            all_one_bits = '0xffffffffffffffff'
            self.assertEqual(eval(all_one_bits), 18446744073709551615L)
            self.assertEqual(eval("-" + all_one_bits), -18446744073709551615L)
        else:
            self.fail("How many bits *does* this machine have???")
        # Verify treatment of contant folding on -(sys.maxint+1)
        # i.e. -2147483648 on 32 bit platforms.  Should return int, not long.
        self.assertTrue(isinstance(eval("%s" % (-sys.maxint - 1)), int))
        self.assertTrue(isinstance(eval("%s" % (-sys.maxint - 2)), long))

    if sys.maxint == 9223372036854775807:
        def test_32_63_bit_values(self):
            a = +4294967296  # 1 << 32
            b = -4294967296  # 1 << 32
            c = +281474976710656  # 1 << 48
            d = -281474976710656  # 1 << 48
            e = +4611686018427387904  # 1 << 62
            f = -4611686018427387904  # 1 << 62
            g = +9223372036854775807  # 1 << 63 - 1
            h = -9223372036854775807  # 1 << 63 - 1

            for variable in self.test_32_63_bit_values.func_code.co_consts:
                if variable is not None:
                    self.assertTrue(isinstance(variable, int))

    def test_sequence_unpacking_error(self):
        # Verify sequence packing/unpacking with "or".  SF bug #757818
        i,j = (1, -1) or (-1, 1)
        self.assertEqual(i, 1)
        self.assertEqual(j, -1)

    def test_none_assignment(self):
        stmts = [
            'None = 0',
            'None += 0',
            '__builtins__.None = 0',
            'def None(): pass',
            'class None: pass',
            '(a, None) = 0, 0',
            'for None in range(10): pass',
            'def f(None): pass',
        ]
        for stmt in stmts:
            stmt += "\n"
            self.assertRaises(SyntaxError, compile, stmt, 'tmp', 'single')
            self.assertRaises(SyntaxError, compile, stmt, 'tmp', 'exec')

    def test_import(self):
        succeed = [
            'import sys',
            'import os, sys',
            'import os as bar',
            'import os.path as bar',
            'from __future__ import nested_scopes, generators',
            'from __future__ import (nested_scopes,\ngenerators)',
            'from __future__ import (nested_scopes,\ngenerators,)',
            'from sys import stdin, stderr, stdout',
            'from sys import (stdin, stderr,\nstdout)',
            'from sys import (stdin, stderr,\nstdout,)',
            'from sys import (stdin\n, stderr, stdout)',
            'from sys import (stdin\n, stderr, stdout,)',
            'from sys import stdin as si, stdout as so, stderr as se',
            'from sys import (stdin as si, stdout as so, stderr as se)',
            'from sys import (stdin as si, stdout as so, stderr as se,)',
            ]
        fail = [
            'import (os, sys)',
            'import (os), (sys)',
            'import ((os), (sys))',
            'import (sys',
            'import sys)',
            'import (os,)',
            'import os As bar',
            'import os.path a bar',
            'from sys import stdin As stdout',
            'from sys import stdin a stdout',
            'from (sys) import stdin',
            'from __future__ import (nested_scopes',
            'from __future__ import nested_scopes)',
            'from __future__ import nested_scopes,\ngenerators',
            'from sys import (stdin',
            'from sys import stdin)',
            'from sys import stdin, stdout,\nstderr',
            'from sys import stdin si',
            'from sys import stdin,'
            'from sys import (*)',
            'from sys import (stdin,, stdout, stderr)',
            'from sys import (stdin, stdout),',
            ]
        for stmt in succeed:
            compile(stmt, 'tmp', 'exec')
        for stmt in fail:
            self.assertRaises(SyntaxError, compile, stmt, 'tmp', 'exec')

    def test_for_distinct_code_objects(self):
        # SF bug 1048870
        def f():
            f1 = lambda x=1: x
            f2 = lambda x=2: x
            return f1, f2
        f1, f2 = f()
        self.assertNotEqual(id(f1.func_code), id(f2.func_code))

    def test_unicode_encoding(self):
        code = u"# -*- coding: utf-8 -*-\npass\n"
        self.assertRaises(SyntaxError, compile, code, "tmp", "exec")

    def test_subscripts(self):
        # SF bug 1448804
        # Class to make testing subscript results easy
        class str_map(object):
            def __init__(self):
                self.data = {}
            def __getitem__(self, key):
                return self.data[str(key)]
            def __setitem__(self, key, value):
                self.data[str(key)] = value
            def __delitem__(self, key):
                del self.data[str(key)]
            def __contains__(self, key):
                return str(key) in self.data
        d = str_map()
        # Index
        d[1] = 1
        self.assertEqual(d[1], 1)
        d[1] += 1
        self.assertEqual(d[1], 2)
        del d[1]
        self.assertEqual(1 in d, False)
        # Tuple of indices
        d[1, 1] = 1
        self.assertEqual(d[1, 1], 1)
        d[1, 1] += 1
        self.assertEqual(d[1, 1], 2)
        del d[1, 1]
        self.assertEqual((1, 1) in d, False)
        # Simple slice
        d[1:2] = 1
        self.assertEqual(d[1:2], 1)
        d[1:2] += 1
        self.assertEqual(d[1:2], 2)
        del d[1:2]
        self.assertEqual(slice(1, 2) in d, False)
        # Tuple of simple slices
        d[1:2, 1:2] = 1
        self.assertEqual(d[1:2, 1:2], 1)
        d[1:2, 1:2] += 1
        self.assertEqual(d[1:2, 1:2], 2)
        del d[1:2, 1:2]
        self.assertEqual((slice(1, 2), slice(1, 2)) in d, False)
        # Extended slice
        d[1:2:3] = 1
        self.assertEqual(d[1:2:3], 1)
        d[1:2:3] += 1
        self.assertEqual(d[1:2:3], 2)
        del d[1:2:3]
        self.assertEqual(slice(1, 2, 3) in d, False)
        # Tuple of extended slices
        d[1:2:3, 1:2:3] = 1
        self.assertEqual(d[1:2:3, 1:2:3], 1)
        d[1:2:3, 1:2:3] += 1
        self.assertEqual(d[1:2:3, 1:2:3], 2)
        del d[1:2:3, 1:2:3]
        self.assertEqual((slice(1, 2, 3), slice(1, 2, 3)) in d, False)
        # Ellipsis
        d[...] = 1
        self.assertEqual(d[...], 1)
        d[...] += 1
        self.assertEqual(d[...], 2)
        del d[...]
        self.assertEqual(Ellipsis in d, False)
        # Tuple of Ellipses
        d[..., ...] = 1
        self.assertEqual(d[..., ...], 1)
        d[..., ...] += 1
        self.assertEqual(d[..., ...], 2)
        del d[..., ...]
        self.assertEqual((Ellipsis, Ellipsis) in d, False)

def test_main():
    test_support.run_unittest(TestSpecifics)

if __name__ == "__main__":
    test_main()
