import math
import unittest
import sys
import _ast
from test import test_support
from test import script_helper
import os
import tempfile
import textwrap

class TestSpecifics(unittest.TestCase):

    def test_no_ending_newline(self):
        compile("hi", "<test>", "exec")
        compile("hi\r", "<test>", "exec")

    def test_empty(self):
        compile("", "<test>", "exec")

    def test_other_newlines(self):
        compile("\r\n", "<test>", "exec")
        compile("\r", "<test>", "exec")
        compile("hi\r\nstuff\r\ndef f():\n    pass\r", "<test>", "exec")
        compile("this_is\rreally_old_mac\rdef f():\n    pass", "<test>", "exec")

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

    def test_exec_functional_style(self):
        # Exec'ing a tuple of length 2 works.
        g = {'b': 2}
        exec("a = b + 1", g)
        self.assertEqual(g['a'], 3)

        # As does exec'ing a tuple of length 3.
        l = {'b': 3}
        g = {'b': 5, 'c': 7}
        exec("a = b + c", g, l)
        self.assertNotIn('a', g)
        self.assertEqual(l['a'], 10)

        # Tuples not of length 2 or 3 are invalid.
        with self.assertRaises(TypeError):
            exec("a = b + 1",)

        with self.assertRaises(TypeError):
            exec("a = b + 1", {}, {}, {})

        # Can't mix and match the two calling forms.
        g = {'a': 3, 'b': 4}
        l = {}
        with self.assertRaises(TypeError):
            exec("a = b + 1", g) in g
        with self.assertRaises(TypeError):
            exec("a = b + 1", g, l) in g, l

    def test_nested_qualified_exec(self):
        # Can use qualified exec in nested functions.
        code = ["""
def g():
    def f():
        if True:
            exec "" in {}, {}
        """, """
def g():
    def f():
        if True:
            exec("", {}, {})
        """, """
def g():
    def f():
        if True:
            exec("", {})
        """]
        for c in code:
            compile(c, "<code>", "exec")

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

        with test_support.check_py3k_warnings(
                ("tuple parameter unpacking has been removed", SyntaxWarning)):
            exec textwrap.dedent('''
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
        ''')

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
                    "080000000000000", "000000000000009", "000000000000008",
                    "0b42", "0BADCAFE", "0o123456789", "0b1.1", "0o4.2",
                    "0b101j2", "0o153j2", "0b100e1", "0o777e1", "0o8", "0o78"]:
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
        self.assertEqual(eval("0b101010"), 42)
        self.assertEqual(eval("-0b000000000010"), -2)
        self.assertEqual(eval("0o777"), 511)
        self.assertEqual(eval("-0o0000010"), -8)
        self.assertEqual(eval("020000000000.0"), 20000000000.0)
        self.assertEqual(eval("037777777777e0"), 37777777777.0)
        self.assertEqual(eval("01000000000000000000000.0"),
                         1000000000000000000000.0)

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
        # Verify treatment of constant folding on -(sys.maxint+1)
        # i.e. -2147483648 on 32 bit platforms.  Should return int, not long.
        self.assertIsInstance(eval("%s" % (-sys.maxint - 1)), int)
        self.assertIsInstance(eval("%s" % (-sys.maxint - 2)), long)

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
                    self.assertIsInstance(variable, int)

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
            'import None',
            'import x as None',
            'from x import None',
            'from x import y as None'
        ]
        for stmt in stmts:
            stmt += "\n"
            self.assertRaises(SyntaxError, compile, stmt, 'tmp', 'single')
            self.assertRaises(SyntaxError, compile, stmt, 'tmp', 'exec')
        # This is ok.
        compile("from None import x", "tmp", "exec")
        compile("from x import None as y", "tmp", "exec")
        compile("import None as x", "tmp", "exec")

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

    def test_lambda_doc(self):
        l = lambda: "foo"
        self.assertIsNone(l.__doc__)

    @test_support.requires_unicode
    def test_encoding(self):
        code = b'# -*- coding: badencoding -*-\npass\n'
        self.assertRaises(SyntaxError, compile, code, 'tmp', 'exec')
        code = u"# -*- coding: utf-8 -*-\npass\n"
        self.assertRaises(SyntaxError, compile, code, "tmp", "exec")
        code = 'u"\xc2\xa4"\n'
        self.assertEqual(eval(code), u'\xc2\xa4')
        code = u'u"\xc2\xa4"\n'
        self.assertEqual(eval(code), u'\xc2\xa4')
        code = '# -*- coding: latin1 -*-\nu"\xc2\xa4"\n'
        self.assertEqual(eval(code), u'\xc2\xa4')
        code = '# -*- coding: utf-8 -*-\nu"\xc2\xa4"\n'
        self.assertEqual(eval(code), u'\xa4')
        code = '# -*- coding: iso8859-15 -*-\nu"\xc2\xa4"\n'
        self.assertEqual(eval(code), test_support.u(r'\xc2\u20ac'))
        code = 'u"""\\\n# -*- coding: utf-8 -*-\n\xc2\xa4"""\n'
        self.assertEqual(eval(code), u'# -*- coding: utf-8 -*-\n\xc2\xa4')

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
        self.assertNotIn(1, d)
        # Tuple of indices
        d[1, 1] = 1
        self.assertEqual(d[1, 1], 1)
        d[1, 1] += 1
        self.assertEqual(d[1, 1], 2)
        del d[1, 1]
        self.assertNotIn((1, 1), d)
        # Simple slice
        d[1:2] = 1
        self.assertEqual(d[1:2], 1)
        d[1:2] += 1
        self.assertEqual(d[1:2], 2)
        del d[1:2]
        self.assertNotIn(slice(1, 2), d)
        # Tuple of simple slices
        d[1:2, 1:2] = 1
        self.assertEqual(d[1:2, 1:2], 1)
        d[1:2, 1:2] += 1
        self.assertEqual(d[1:2, 1:2], 2)
        del d[1:2, 1:2]
        self.assertNotIn((slice(1, 2), slice(1, 2)), d)
        # Extended slice
        d[1:2:3] = 1
        self.assertEqual(d[1:2:3], 1)
        d[1:2:3] += 1
        self.assertEqual(d[1:2:3], 2)
        del d[1:2:3]
        self.assertNotIn(slice(1, 2, 3), d)
        # Tuple of extended slices
        d[1:2:3, 1:2:3] = 1
        self.assertEqual(d[1:2:3, 1:2:3], 1)
        d[1:2:3, 1:2:3] += 1
        self.assertEqual(d[1:2:3, 1:2:3], 2)
        del d[1:2:3, 1:2:3]
        self.assertNotIn((slice(1, 2, 3), slice(1, 2, 3)), d)
        # Ellipsis
        d[...] = 1
        self.assertEqual(d[...], 1)
        d[...] += 1
        self.assertEqual(d[...], 2)
        del d[...]
        self.assertNotIn(Ellipsis, d)
        # Tuple of Ellipses
        d[..., ...] = 1
        self.assertEqual(d[..., ...], 1)
        d[..., ...] += 1
        self.assertEqual(d[..., ...], 2)
        del d[..., ...]
        self.assertNotIn((Ellipsis, Ellipsis), d)

    def test_mangling(self):
        class A:
            def f():
                __mangled = 1
                __not_mangled__ = 2
                import __mangled_mod
                import __package__.module

        self.assertIn("_A__mangled", A.f.func_code.co_varnames)
        self.assertIn("__not_mangled__", A.f.func_code.co_varnames)
        self.assertIn("_A__mangled_mod", A.f.func_code.co_varnames)
        self.assertIn("__package__", A.f.func_code.co_varnames)

    def test_compile_ast(self):
        fname = __file__
        if fname.lower().endswith(('pyc', 'pyo')):
            fname = fname[:-1]
        with open(fname, 'r') as f:
            fcontents = f.read()
        sample_code = [
            ['<assign>', 'x = 5'],
            ['<print1>', 'print 1'],
            ['<printv>', 'print v'],
            ['<printTrue>', 'print True'],
            ['<printList>', 'print []'],
            ['<ifblock>', """if True:\n    pass\n"""],
            ['<forblock>', """for n in [1, 2, 3]:\n    print n\n"""],
            ['<deffunc>', """def foo():\n    pass\nfoo()\n"""],
            [fname, fcontents],
        ]

        for fname, code in sample_code:
            co1 = compile(code, '%s1' % fname, 'exec')
            ast = compile(code, '%s2' % fname, 'exec', _ast.PyCF_ONLY_AST)
            self.assertTrue(type(ast) == _ast.Module)
            co2 = compile(ast, '%s3' % fname, 'exec')
            self.assertEqual(co1, co2)
            # the code object's filename comes from the second compilation step
            self.assertEqual(co2.co_filename, '%s3' % fname)

        # raise exception when node type doesn't match with compile mode
        co1 = compile('print 1', '<string>', 'exec', _ast.PyCF_ONLY_AST)
        self.assertRaises(TypeError, compile, co1, '<ast>', 'eval')

        # raise exception when node type is no start node
        self.assertRaises(TypeError, compile, _ast.If(), '<ast>', 'exec')

        # raise exception when node has invalid children
        ast = _ast.Module()
        ast.body = [_ast.BoolOp()]
        self.assertRaises(TypeError, compile, ast, '<ast>', 'exec')

    def test_yet_more_evil_still_undecodable(self):
        # Issue #25388
        src = b"#\x00\n#\xfd\n"
        tmpd = tempfile.mkdtemp()
        try:
            fn = os.path.join(tmpd, "bad.py")
            with open(fn, "wb") as fp:
                fp.write(src)
            rc, out, err = script_helper.assert_python_failure(fn)
        finally:
            test_support.rmtree(tmpd)
        self.assertIn(b"Non-ASCII", err)

    def test_null_terminated(self):
        # The source code is null-terminated internally, but bytes-like
        # objects are accepted, which could be not terminated.
        with self.assertRaisesRegexp(TypeError, "without null bytes"):
            compile(u"123\x00", "<dummy>", "eval")
        with test_support.check_py3k_warnings():
            with self.assertRaisesRegexp(TypeError, "without null bytes"):
                compile(buffer("123\x00"), "<dummy>", "eval")
            code = compile(buffer("123\x00", 1, 2), "<dummy>", "eval")
            self.assertEqual(eval(code), 23)
            code = compile(buffer("1234", 1, 2), "<dummy>", "eval")
            self.assertEqual(eval(code), 23)
            code = compile(buffer("$23$", 1, 2), "<dummy>", "eval")
            self.assertEqual(eval(code), 23)

class TestStackSize(unittest.TestCase):
    # These tests check that the computed stack size for a code object
    # stays within reasonable bounds (see issue #21523 for an example
    # dysfunction).
    N = 100

    def check_stack_size(self, code):
        # To assert that the alleged stack size is not O(N), we
        # check that it is smaller than log(N).
        if isinstance(code, str):
            code = compile(code, "<foo>", "single")
        max_size = math.ceil(math.log(len(code.co_code)))
        self.assertLessEqual(code.co_stacksize, max_size)

    def test_and(self):
        self.check_stack_size("x and " * self.N + "x")

    def test_or(self):
        self.check_stack_size("x or " * self.N + "x")

    def test_and_or(self):
        self.check_stack_size("x and x or " * self.N + "x")

    def test_chained_comparison(self):
        self.check_stack_size("x < " * self.N + "x")

    def test_if_else(self):
        self.check_stack_size("x if x else " * self.N + "x")

    def test_binop(self):
        self.check_stack_size("x + " * self.N + "x")

    def test_func_and(self):
        code = "def f(x):\n"
        code += "   x and x\n" * self.N
        self.check_stack_size(code)

    def check_constant(self, func, expected):
        for const in func.__code__.co_consts:
            if repr(const) == repr(expected):
                break
        else:
            self.fail("unable to find constant %r in %r"
                      % (expected, func.__code__.co_consts))

    # Merging equal constants is not a strict requirement for the Python
    # semantics, it's a more an implementation detail.
    @test_support.cpython_only
    def test_merge_constants(self):
        # Issue #25843: compile() must merge constants which are equal
        # and have the same type.

        def check_same_constant(const):
            ns = {}
            code = "f1, f2 = lambda: %r, lambda: %r" % (const, const)
            exec(code, ns)
            f1 = ns['f1']
            f2 = ns['f2']
            self.assertIs(f1.__code__, f2.__code__)
            self.check_constant(f1, const)
            self.assertEqual(repr(f1()), repr(const))

        check_same_constant(None)
        check_same_constant(0)
        check_same_constant(0.0)
        check_same_constant(b'abc')
        check_same_constant('abc')

    def test_dont_merge_constants(self):
        # Issue #25843: compile() must not merge constants which are equal
        # but have a different type.

        def check_different_constants(const1, const2):
            ns = {}
            exec("f1, f2 = lambda: %r, lambda: %r" % (const1, const2), ns)
            f1 = ns['f1']
            f2 = ns['f2']
            self.assertIsNot(f1.__code__, f2.__code__)
            self.check_constant(f1, const1)
            self.check_constant(f2, const2)
            self.assertEqual(repr(f1()), repr(const1))
            self.assertEqual(repr(f2()), repr(const2))

        check_different_constants(0, 0.0)
        check_different_constants(+0.0, -0.0)
        check_different_constants((0,), (0.0,))

        # check_different_constants() cannot be used because repr(-0j) is
        # '(-0-0j)', but when '(-0-0j)' is evaluated to 0j: we loose the sign.
        f1, f2 = lambda: +0.0j, lambda: -0.0j
        self.assertIsNot(f1.__code__, f2.__code__)
        self.check_constant(f1, +0.0j)
        self.check_constant(f2, -0.0j)
        self.assertEqual(repr(f1()), repr(+0.0j))
        self.assertEqual(repr(f2()), repr(-0.0j))


def test_main():
    test_support.run_unittest(__name__)

if __name__ == "__main__":
    unittest.main()
