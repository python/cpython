import copy
import parser
import pickle
import unittest
import operator
import struct
from test import support
from test.support.script_helper import assert_python_failure

#
#  First, we test that we can generate trees from valid source fragments,
#  and that these valid trees are indeed allowed by the tree-loading side
#  of the parser module.
#

class RoundtripLegalSyntaxTestCase(unittest.TestCase):

    def roundtrip(self, f, s):
        st1 = f(s)
        t = st1.totuple()
        try:
            st2 = parser.sequence2st(t)
        except parser.ParserError as why:
            self.fail("could not roundtrip %r: %s" % (s, why))

        self.assertEqual(t, st2.totuple(),
                         "could not re-generate syntax tree")

    def check_expr(self, s):
        self.roundtrip(parser.expr, s)

    def test_flags_passed(self):
        # The unicode literals flags has to be passed from the parser to AST
        # generation.
        suite = parser.suite("from __future__ import unicode_literals; x = ''")
        code = suite.compile()
        scope = {}
        exec(code, {}, scope)
        self.assertIsInstance(scope["x"], str)

    def check_suite(self, s):
        self.roundtrip(parser.suite, s)

    def test_yield_statement(self):
        self.check_suite("def f(): yield 1")
        self.check_suite("def f(): yield")
        self.check_suite("def f(): x += yield")
        self.check_suite("def f(): x = yield 1")
        self.check_suite("def f(): x = y = yield 1")
        self.check_suite("def f(): x = yield")
        self.check_suite("def f(): x = y = yield")
        self.check_suite("def f(): 1 + (yield)*2")
        self.check_suite("def f(): (yield 1)*2")
        self.check_suite("def f(): return; yield 1")
        self.check_suite("def f(): yield 1; return")
        self.check_suite("def f(): yield from 1")
        self.check_suite("def f(): x = yield from 1")
        self.check_suite("def f(): f((yield from 1))")
        self.check_suite("def f(): yield 1; return 1")
        self.check_suite("def f():\n"
                         "    for x in range(30):\n"
                         "        yield x\n")
        self.check_suite("def f():\n"
                         "    if (yield):\n"
                         "        yield x\n")

    def test_await_statement(self):
        self.check_suite("async def f():\n await smth()")
        self.check_suite("async def f():\n foo = await smth()")
        self.check_suite("async def f():\n foo, bar = await smth()")
        self.check_suite("async def f():\n (await smth())")
        self.check_suite("async def f():\n foo((await smth()))")
        self.check_suite("async def f():\n await foo(); return 42")

    def test_async_with_statement(self):
        self.check_suite("async def f():\n async with 1: pass")
        self.check_suite("async def f():\n async with a as b, c as d: pass")

    def test_async_for_statement(self):
        self.check_suite("async def f():\n async for i in (): pass")
        self.check_suite("async def f():\n async for i, b in (): pass")

    def test_nonlocal_statement(self):
        self.check_suite("def f():\n"
                         "    x = 0\n"
                         "    def g():\n"
                         "        nonlocal x\n")
        self.check_suite("def f():\n"
                         "    x = y = 0\n"
                         "    def g():\n"
                         "        nonlocal x, y\n")

    def test_expressions(self):
        self.check_expr("foo(1)")
        self.check_expr("[1, 2, 3]")
        self.check_expr("[x**3 for x in range(20)]")
        self.check_expr("[x**3 for x in range(20) if x % 3]")
        self.check_expr("[x**3 for x in range(20) if x % 2 if x % 3]")
        self.check_expr("list(x**3 for x in range(20))")
        self.check_expr("list(x**3 for x in range(20) if x % 3)")
        self.check_expr("list(x**3 for x in range(20) if x % 2 if x % 3)")
        self.check_expr("foo(*args)")
        self.check_expr("foo(*args, **kw)")
        self.check_expr("foo(**kw)")
        self.check_expr("foo(key=value)")
        self.check_expr("foo(key=value, *args)")
        self.check_expr("foo(key=value, *args, **kw)")
        self.check_expr("foo(key=value, **kw)")
        self.check_expr("foo(a, b, c, *args)")
        self.check_expr("foo(a, b, c, *args, **kw)")
        self.check_expr("foo(a, b, c, **kw)")
        self.check_expr("foo(a, *args, keyword=23)")
        self.check_expr("foo + bar")
        self.check_expr("foo - bar")
        self.check_expr("foo * bar")
        self.check_expr("foo / bar")
        self.check_expr("foo // bar")
        self.check_expr("(foo := 1)")
        self.check_expr("lambda: 0")
        self.check_expr("lambda x: 0")
        self.check_expr("lambda *y: 0")
        self.check_expr("lambda *y, **z: 0")
        self.check_expr("lambda **z: 0")
        self.check_expr("lambda x, y: 0")
        self.check_expr("lambda foo=bar: 0")
        self.check_expr("lambda foo=bar, spaz=nifty+spit: 0")
        self.check_expr("lambda foo=bar, **z: 0")
        self.check_expr("lambda foo=bar, blaz=blat+2, **z: 0")
        self.check_expr("lambda foo=bar, blaz=blat+2, *y, **z: 0")
        self.check_expr("lambda x, *y, **z: 0")
        self.check_expr("(x for x in range(10))")
        self.check_expr("foo(x for x in range(10))")
        self.check_expr("...")
        self.check_expr("a[...]")

    def test_simple_expression(self):
        # expr_stmt
        self.check_suite("a")

    def test_simple_assignments(self):
        self.check_suite("a = b")
        self.check_suite("a = b = c = d = e")

    def test_var_annot(self):
        self.check_suite("x: int = 5")
        self.check_suite("y: List[T] = []; z: [list] = fun()")
        self.check_suite("x: tuple = (1, 2)")
        self.check_suite("d[f()]: int = 42")
        self.check_suite("f(d[x]): str = 'abc'")
        self.check_suite("x.y.z.w: complex = 42j")
        self.check_suite("x: int")
        self.check_suite("def f():\n"
                         "    x: str\n"
                         "    y: int = 5\n")
        self.check_suite("class C:\n"
                         "    x: str\n"
                         "    y: int = 5\n")
        self.check_suite("class C:\n"
                         "    def __init__(self, x: int) -> None:\n"
                         "        self.x: int = x\n")
        # double check for nonsense
        with self.assertRaises(SyntaxError):
            exec("2+2: int", {}, {})
        with self.assertRaises(SyntaxError):
            exec("[]: int = 5", {}, {})
        with self.assertRaises(SyntaxError):
            exec("x, *y, z: int = range(5)", {}, {})
        with self.assertRaises(SyntaxError):
            exec("x: int = 1, y = 2", {}, {})
        with self.assertRaises(SyntaxError):
            exec("u = v: int", {}, {})
        with self.assertRaises(SyntaxError):
            exec("False: int", {}, {})
        with self.assertRaises(SyntaxError):
            exec("x.False: int", {}, {})
        with self.assertRaises(SyntaxError):
            exec("x.y,: int", {}, {})
        with self.assertRaises(SyntaxError):
            exec("[0]: int", {}, {})
        with self.assertRaises(SyntaxError):
            exec("f(): int", {}, {})

    def test_simple_augmented_assignments(self):
        self.check_suite("a += b")
        self.check_suite("a -= b")
        self.check_suite("a *= b")
        self.check_suite("a /= b")
        self.check_suite("a //= b")
        self.check_suite("a %= b")
        self.check_suite("a &= b")
        self.check_suite("a |= b")
        self.check_suite("a ^= b")
        self.check_suite("a <<= b")
        self.check_suite("a >>= b")
        self.check_suite("a **= b")

    def test_function_defs(self):
        self.check_suite("def f(): pass")
        self.check_suite("def f(*args): pass")
        self.check_suite("def f(*args, **kw): pass")
        self.check_suite("def f(**kw): pass")
        self.check_suite("def f(foo=bar): pass")
        self.check_suite("def f(foo=bar, *args): pass")
        self.check_suite("def f(foo=bar, *args, **kw): pass")
        self.check_suite("def f(foo=bar, **kw): pass")

        self.check_suite("def f(a, b): pass")
        self.check_suite("def f(a, b, *args): pass")
        self.check_suite("def f(a, b, *args, **kw): pass")
        self.check_suite("def f(a, b, **kw): pass")
        self.check_suite("def f(a, b, foo=bar): pass")
        self.check_suite("def f(a, b, foo=bar, *args): pass")
        self.check_suite("def f(a, b, foo=bar, *args, **kw): pass")
        self.check_suite("def f(a, b, foo=bar, **kw): pass")

        self.check_suite("@staticmethod\n"
                         "def f(): pass")
        self.check_suite("@staticmethod\n"
                         "@funcattrs(x, y)\n"
                         "def f(): pass")
        self.check_suite("@funcattrs()\n"
                         "def f(): pass")

        # keyword-only arguments
        self.check_suite("def f(*, a): pass")
        self.check_suite("def f(*, a = 5): pass")
        self.check_suite("def f(*, a = 5, b): pass")
        self.check_suite("def f(*, a, b = 5): pass")
        self.check_suite("def f(*, a, b = 5, **kwds): pass")
        self.check_suite("def f(*args, a): pass")
        self.check_suite("def f(*args, a = 5): pass")
        self.check_suite("def f(*args, a = 5, b): pass")
        self.check_suite("def f(*args, a, b = 5): pass")
        self.check_suite("def f(*args, a, b = 5, **kwds): pass")

        # positional-only arguments
        self.check_suite("def f(a, /): pass")
        self.check_suite("def f(a, /,): pass")
        self.check_suite("def f(a, b, /): pass")
        self.check_suite("def f(a, b, /, c): pass")
        self.check_suite("def f(a, b, /, c = 6): pass")
        self.check_suite("def f(a, b, /, c, *, d): pass")
        self.check_suite("def f(a, b, /, c = 1, *, d): pass")
        self.check_suite("def f(a, b, /, c, *, d = 1): pass")
        self.check_suite("def f(a, b=1, /, c=2, *, d = 3): pass")
        self.check_suite("def f(a=0, b=1, /, c=2, *, d = 3): pass")

        # function annotations
        self.check_suite("def f(a: int): pass")
        self.check_suite("def f(a: int = 5): pass")
        self.check_suite("def f(*args: list): pass")
        self.check_suite("def f(**kwds: dict): pass")
        self.check_suite("def f(*, a: int): pass")
        self.check_suite("def f(*, a: int = 5): pass")
        self.check_suite("def f() -> int: pass")

    def test_class_defs(self):
        self.check_suite("class foo():pass")
        self.check_suite("class foo(object):pass")
        self.check_suite("@class_decorator\n"
                         "class foo():pass")
        self.check_suite("@class_decorator(arg)\n"
                         "class foo():pass")
        self.check_suite("@decorator1\n"
                         "@decorator2\n"
                         "class foo():pass")

    def test_import_from_statement(self):
        self.check_suite("from sys.path import *")
        self.check_suite("from sys.path import dirname")
        self.check_suite("from sys.path import (dirname)")
        self.check_suite("from sys.path import (dirname,)")
        self.check_suite("from sys.path import dirname as my_dirname")
        self.check_suite("from sys.path import (dirname as my_dirname)")
        self.check_suite("from sys.path import (dirname as my_dirname,)")
        self.check_suite("from sys.path import dirname, basename")
        self.check_suite("from sys.path import (dirname, basename)")
        self.check_suite("from sys.path import (dirname, basename,)")
        self.check_suite(
            "from sys.path import dirname as my_dirname, basename")
        self.check_suite(
            "from sys.path import (dirname as my_dirname, basename)")
        self.check_suite(
            "from sys.path import (dirname as my_dirname, basename,)")
        self.check_suite(
            "from sys.path import dirname, basename as my_basename")
        self.check_suite(
            "from sys.path import (dirname, basename as my_basename)")
        self.check_suite(
            "from sys.path import (dirname, basename as my_basename,)")
        self.check_suite("from .bogus import x")

    def test_basic_import_statement(self):
        self.check_suite("import sys")
        self.check_suite("import sys as system")
        self.check_suite("import sys, math")
        self.check_suite("import sys as system, math")
        self.check_suite("import sys, math as my_math")

    def test_relative_imports(self):
        self.check_suite("from . import name")
        self.check_suite("from .. import name")
        # check all the way up to '....', since '...' is tokenized
        # differently from '.' (it's an ellipsis token).
        self.check_suite("from ... import name")
        self.check_suite("from .... import name")
        self.check_suite("from .pkg import name")
        self.check_suite("from ..pkg import name")
        self.check_suite("from ...pkg import name")
        self.check_suite("from ....pkg import name")

    def test_pep263(self):
        self.check_suite("# -*- coding: iso-8859-1 -*-\n"
                         "pass\n")

    def test_assert(self):
        self.check_suite("assert alo < ahi and blo < bhi\n")

    def test_with(self):
        self.check_suite("with open('x'): pass\n")
        self.check_suite("with open('x') as f: pass\n")
        self.check_suite("with open('x') as f, open('y') as g: pass\n")

    def test_try_stmt(self):
        self.check_suite("try: pass\nexcept: pass\n")
        self.check_suite("try: pass\nfinally: pass\n")
        self.check_suite("try: pass\nexcept A: pass\nfinally: pass\n")
        self.check_suite("try: pass\nexcept A: pass\nexcept: pass\n"
                         "finally: pass\n")
        self.check_suite("try: pass\nexcept: pass\nelse: pass\n")
        self.check_suite("try: pass\nexcept: pass\nelse: pass\n"
                         "finally: pass\n")

    def test_if_stmt(self):
        self.check_suite("if True:\n  pass\nelse:\n  pass\n")
        self.check_suite("if True:\n  pass\nelif True:\n  pass\nelse:\n  pass\n")

    def test_position(self):
        # An absolutely minimal test of position information.  Better
        # tests would be a big project.
        code = "def f(x):\n    return x + 1"
        st = parser.suite(code)

        def walk(tree):
            node_type = tree[0]
            next = tree[1]
            if isinstance(next, (tuple, list)):
                for elt in tree[1:]:
                    for x in walk(elt):
                        yield x
            else:
                yield tree

        expected = [
            (1, 'def', 1, 0),
            (1, 'f', 1, 4),
            (7, '(', 1, 5),
            (1, 'x', 1, 6),
            (8, ')', 1, 7),
            (11, ':', 1, 8),
            (4, '', 1, 9),
            (5, '', 2, -1),
            (1, 'return', 2, 4),
            (1, 'x', 2, 11),
            (14, '+', 2, 13),
            (2, '1', 2, 15),
            (4, '', 2, 16),
            (6, '', 2, -1),
            (4, '', 2, -1),
            (0, '', 2, -1),
        ]

        self.assertEqual(list(walk(st.totuple(line_info=True, col_info=True))),
                         expected)
        self.assertEqual(list(walk(st.totuple())),
                         [(t, n) for t, n, l, c in expected])
        self.assertEqual(list(walk(st.totuple(line_info=True))),
                         [(t, n, l) for t, n, l, c in expected])
        self.assertEqual(list(walk(st.totuple(col_info=True))),
                         [(t, n, c) for t, n, l, c in expected])
        self.assertEqual(list(walk(st.tolist(line_info=True, col_info=True))),
                         [list(x) for x in expected])
        self.assertEqual(list(walk(parser.st2tuple(st, line_info=True,
                                                   col_info=True))),
                         expected)
        self.assertEqual(list(walk(parser.st2list(st, line_info=True,
                                                  col_info=True))),
                         [list(x) for x in expected])

    def test_extended_unpacking(self):
        self.check_suite("*a = y")
        self.check_suite("x, *b, = m")
        self.check_suite("[*a, *b] = y")
        self.check_suite("for [*x, b] in x: pass")

    def test_raise_statement(self):
        self.check_suite("raise\n")
        self.check_suite("raise e\n")
        self.check_suite("try:\n"
                         "    suite\n"
                         "except Exception as e:\n"
                         "    raise ValueError from e\n")

    def test_list_displays(self):
        self.check_expr('[]')
        self.check_expr('[*{2}, 3, *[4]]')

    def test_set_displays(self):
        self.check_expr('{*{2}, 3, *[4]}')
        self.check_expr('{2}')
        self.check_expr('{2,}')
        self.check_expr('{2, 3}')
        self.check_expr('{2, 3,}')

    def test_dict_displays(self):
        self.check_expr('{}')
        self.check_expr('{a:b}')
        self.check_expr('{a:b,}')
        self.check_expr('{a:b, c:d}')
        self.check_expr('{a:b, c:d,}')
        self.check_expr('{**{}}')
        self.check_expr('{**{}, 3:4, **{5:6, 7:8}}')

    def test_argument_unpacking(self):
        self.check_expr("f(*a, **b)")
        self.check_expr('f(a, *b, *c, *d)')
        self.check_expr('f(**a, **b)')
        self.check_expr('f(2, *a, *b, **b, **c, **d)')
        self.check_expr("f(*b, *() or () and (), **{} and {}, **() or {})")

    def test_set_comprehensions(self):
        self.check_expr('{x for x in seq}')
        self.check_expr('{f(x) for x in seq}')
        self.check_expr('{f(x) for x in seq if condition(x)}')

    def test_dict_comprehensions(self):
        self.check_expr('{x:x for x in seq}')
        self.check_expr('{x**2:x[3] for x in seq if condition(x)}')
        self.check_expr('{x:x for x in seq1 for y in seq2 if condition(x, y)}')

    def test_named_expressions(self):
        self.check_suite("(a := 1)")
        self.check_suite("(a := a)")
        self.check_suite("if (match := pattern.search(data)) is None: pass")
        self.check_suite("while match := pattern.search(f.read()): pass")
        self.check_suite("[y := f(x), y**2, y**3]")
        self.check_suite("filtered_data = [y for x in data if (y := f(x)) is None]")
        self.check_suite("(y := f(x))")
        self.check_suite("y0 = (y1 := f(x))")
        self.check_suite("foo(x=(y := f(x)))")
        self.check_suite("def foo(answer=(p := 42)): pass")
        self.check_suite("def foo(answer: (p := 42) = 5): pass")
        self.check_suite("lambda: (x := 1)")
        self.check_suite("(x := lambda: 1)")
        self.check_suite("(x := lambda: (y := 1))")  # not in PEP
        self.check_suite("lambda line: (m := re.match(pattern, line)) and m.group(1)")
        self.check_suite("x = (y := 0)")
        self.check_suite("(z:=(y:=(x:=0)))")
        self.check_suite("(info := (name, phone, *rest))")
        self.check_suite("(x:=1,2)")
        self.check_suite("(total := total + tax)")
        self.check_suite("len(lines := f.readlines())")
        self.check_suite("foo(x := 3, cat='vector')")
        self.check_suite("foo(cat=(category := 'vector'))")
        self.check_suite("if any(len(longline := l) >= 100 for l in lines): print(longline)")
        self.check_suite(
            "if env_base := os.environ.get('PYTHONUSERBASE', None): return env_base"
        )
        self.check_suite(
            "if self._is_special and (ans := self._check_nans(context=context)): return ans"
        )
        self.check_suite("foo(b := 2, a=1)")
        self.check_suite("foo(b := 2, a=1)")
        self.check_suite("foo((b := 2), a=1)")
        self.check_suite("foo(c=(b := 2), a=1)")
        self.check_suite("{(x := C(i)).q: x for i in y}")


#
#  Second, we take *invalid* trees and make sure we get ParserError
#  rejections for them.
#

class IllegalSyntaxTestCase(unittest.TestCase):

    def check_bad_tree(self, tree, label):
        try:
            parser.sequence2st(tree)
        except parser.ParserError:
            pass
        else:
            self.fail("did not detect invalid tree for %r" % label)

    def test_junk(self):
        # not even remotely valid:
        self.check_bad_tree((1, 2, 3), "<junk>")

    def test_illegal_terminal(self):
        tree = \
            (257,
             (269,
              (270,
               (271,
                (277,
                 (1,))),
               (4, ''))),
             (4, ''),
             (0, ''))
        self.check_bad_tree(tree, "too small items in terminal node")
        tree = \
            (257,
             (269,
              (270,
               (271,
                (277,
                 (1, b'pass'))),
               (4, ''))),
             (4, ''),
             (0, ''))
        self.check_bad_tree(tree, "non-string second item in terminal node")
        tree = \
            (257,
             (269,
              (270,
               (271,
                (277,
                 (1, 'pass', '0', 0))),
               (4, ''))),
             (4, ''),
             (0, ''))
        self.check_bad_tree(tree, "non-integer third item in terminal node")
        tree = \
            (257,
             (269,
              (270,
               (271,
                (277,
                 (1, 'pass', 0, 0))),
               (4, ''))),
             (4, ''),
             (0, ''))
        self.check_bad_tree(tree, "too many items in terminal node")

    def test_illegal_yield_1(self):
        # Illegal yield statement: def f(): return 1; yield 1
        tree = \
        (257,
         (264,
          (285,
           (259,
            (1, 'def'),
            (1, 'f'),
            (260, (7, '('), (8, ')')),
            (11, ':'),
            (291,
             (4, ''),
             (5, ''),
             (264,
              (265,
               (266,
                (272,
                 (275,
                  (1, 'return'),
                  (313,
                   (292,
                    (293,
                     (294,
                      (295,
                       (297,
                        (298,
                         (299,
                          (300,
                           (301,
                            (302, (303, (304, (305, (2, '1')))))))))))))))))),
               (264,
                (265,
                 (266,
                  (272,
                   (276,
                    (1, 'yield'),
                    (313,
                     (292,
                      (293,
                       (294,
                        (295,
                         (297,
                          (298,
                           (299,
                            (300,
                             (301,
                              (302,
                               (303, (304, (305, (2, '1')))))))))))))))))),
                 (4, ''))),
               (6, ''))))),
           (4, ''),
           (0, ''))))
        self.check_bad_tree(tree, "def f():\n  return 1\n  yield 1")

    def test_illegal_yield_2(self):
        # Illegal return in generator: def f(): return 1; yield 1
        tree = \
        (257,
         (264,
          (265,
           (266,
            (278,
             (1, 'from'),
             (281, (1, '__future__')),
             (1, 'import'),
             (279, (1, 'generators')))),
           (4, ''))),
         (264,
          (285,
           (259,
            (1, 'def'),
            (1, 'f'),
            (260, (7, '('), (8, ')')),
            (11, ':'),
            (291,
             (4, ''),
             (5, ''),
             (264,
              (265,
               (266,
                (272,
                 (275,
                  (1, 'return'),
                  (313,
                   (292,
                    (293,
                     (294,
                      (295,
                       (297,
                        (298,
                         (299,
                          (300,
                           (301,
                            (302, (303, (304, (305, (2, '1')))))))))))))))))),
               (264,
                (265,
                 (266,
                  (272,
                   (276,
                    (1, 'yield'),
                    (313,
                     (292,
                      (293,
                       (294,
                        (295,
                         (297,
                          (298,
                           (299,
                            (300,
                             (301,
                              (302,
                               (303, (304, (305, (2, '1')))))))))))))))))),
                 (4, ''))),
               (6, ''))))),
           (4, ''),
           (0, ''))))
        self.check_bad_tree(tree, "def f():\n  return 1\n  yield 1")

    def test_a_comma_comma_c(self):
        # Illegal input: a,,c
        tree = \
        (258,
         (311,
          (290,
           (291,
            (292,
             (293,
              (295,
               (296,
                (297,
                 (298, (299, (300, (301, (302, (303, (1, 'a')))))))))))))),
          (12, ','),
          (12, ','),
          (290,
           (291,
            (292,
             (293,
              (295,
               (296,
                (297,
                 (298, (299, (300, (301, (302, (303, (1, 'c'))))))))))))))),
         (4, ''),
         (0, ''))
        self.check_bad_tree(tree, "a,,c")

    def test_illegal_operator(self):
        # Illegal input: a $= b
        tree = \
        (257,
         (264,
          (265,
           (266,
            (267,
             (312,
              (291,
               (292,
                (293,
                 (294,
                  (296,
                   (297,
                    (298,
                     (299,
                      (300, (301, (302, (303, (304, (1, 'a'))))))))))))))),
             (268, (37, '$=')),
             (312,
              (291,
               (292,
                (293,
                 (294,
                  (296,
                   (297,
                    (298,
                     (299,
                      (300, (301, (302, (303, (304, (1, 'b'))))))))))))))))),
           (4, ''))),
         (0, ''))
        self.check_bad_tree(tree, "a $= b")

    def test_malformed_global(self):
        #doesn't have global keyword in ast
        tree = (257,
                (264,
                 (265,
                  (266,
                   (282, (1, 'foo'))), (4, ''))),
                (4, ''),
                (0, ''))
        self.check_bad_tree(tree, "malformed global ast")

    def test_missing_import_source(self):
        # from import fred
        tree = \
            (257,
             (268,
              (269,
               (270,
                (282,
                 (284, (1, 'from'), (1, 'import'),
                  (287, (285, (1, 'fred')))))),
               (4, ''))),
             (4, ''), (0, ''))
        self.check_bad_tree(tree, "from import fred")

    def test_illegal_encoding(self):
        # Illegal encoding declaration
        tree = \
            (341,
             (257, (0, '')))
        self.check_bad_tree(tree, "missed encoding")
        tree = \
            (341,
             (257, (0, '')),
              b'iso-8859-1')
        self.check_bad_tree(tree, "non-string encoding")
        tree = \
            (341,
             (257, (0, '')),
              '\udcff')
        with self.assertRaises(UnicodeEncodeError):
            parser.sequence2st(tree)

    def test_invalid_node_id(self):
        tree = (257, (269, (-7, '')))
        self.check_bad_tree(tree, "negative node id")
        tree = (257, (269, (99, '')))
        self.check_bad_tree(tree, "invalid token id")
        tree = (257, (269, (9999, (0, ''))))
        self.check_bad_tree(tree, "invalid symbol id")

    def test_ParserError_message(self):
        try:
            parser.sequence2st((257,(269,(257,(0,'')))))
        except parser.ParserError as why:
            self.assertIn("compound_stmt", str(why))  # Expected
            self.assertIn("file_input", str(why))     # Got



class CompileTestCase(unittest.TestCase):

    # These tests are very minimal. :-(

    def test_compile_expr(self):
        st = parser.expr('2 + 3')
        code = parser.compilest(st)
        self.assertEqual(eval(code), 5)

    def test_compile_suite(self):
        st = parser.suite('x = 2; y = x + 3')
        code = parser.compilest(st)
        globs = {}
        exec(code, globs)
        self.assertEqual(globs['y'], 5)

    def test_compile_error(self):
        st = parser.suite('1 = 3 + 4')
        self.assertRaises(SyntaxError, parser.compilest, st)

    def test_compile_badunicode(self):
        st = parser.suite('a = "\\U12345678"')
        self.assertRaises(SyntaxError, parser.compilest, st)
        st = parser.suite('a = "\\u1"')
        self.assertRaises(SyntaxError, parser.compilest, st)

    def test_issue_9011(self):
        # Issue 9011: compilation of an unary minus expression changed
        # the meaning of the ST, so that a second compilation produced
        # incorrect results.
        st = parser.expr('-3')
        code1 = parser.compilest(st)
        self.assertEqual(eval(code1), -3)
        code2 = parser.compilest(st)
        self.assertEqual(eval(code2), -3)

    def test_compile_filename(self):
        st = parser.expr('a + 5')
        code = parser.compilest(st)
        self.assertEqual(code.co_filename, '<syntax-tree>')
        code = st.compile()
        self.assertEqual(code.co_filename, '<syntax-tree>')
        for filename in 'file.py', b'file.py':
            code = parser.compilest(st, filename)
            self.assertEqual(code.co_filename, 'file.py')
            code = st.compile(filename)
            self.assertEqual(code.co_filename, 'file.py')
        for filename in bytearray(b'file.py'), memoryview(b'file.py'):
            with self.assertWarns(DeprecationWarning):
                code = parser.compilest(st, filename)
            self.assertEqual(code.co_filename, 'file.py')
            with self.assertWarns(DeprecationWarning):
                code = st.compile(filename)
            self.assertEqual(code.co_filename, 'file.py')
        self.assertRaises(TypeError, parser.compilest, st, list(b'file.py'))
        self.assertRaises(TypeError, st.compile, list(b'file.py'))


class ParserStackLimitTestCase(unittest.TestCase):
    """try to push the parser to/over its limits.
    see http://bugs.python.org/issue1881 for a discussion
    """
    def _nested_expression(self, level):
        return "["*level+"]"*level

    def test_deeply_nested_list(self):
        # This has fluctuated between 99 levels in 2.x, down to 93 levels in
        # 3.7.X and back up to 99 in 3.8.X. Related to MAXSTACK size in Parser.h
        e = self._nested_expression(99)
        st = parser.expr(e)
        st.compile()

    def test_trigger_memory_error(self):
        e = self._nested_expression(100)
        rc, out, err = assert_python_failure('-c', e)
        # parsing the expression will result in an error message
        # followed by a MemoryError (see #11963)
        self.assertIn(b's_push: parser stack overflow', err)
        self.assertIn(b'MemoryError', err)

class STObjectTestCase(unittest.TestCase):
    """Test operations on ST objects themselves"""

    def test_comparisons(self):
        # ST objects should support order and equality comparisons
        st1 = parser.expr('2 + 3')
        st2 = parser.suite('x = 2; y = x + 3')
        st3 = parser.expr('list(x**3 for x in range(20))')
        st1_copy = parser.expr('2 + 3')
        st2_copy = parser.suite('x = 2; y = x + 3')
        st3_copy = parser.expr('list(x**3 for x in range(20))')

        # exercise fast path for object identity
        self.assertEqual(st1 == st1, True)
        self.assertEqual(st2 == st2, True)
        self.assertEqual(st3 == st3, True)
        # slow path equality
        self.assertEqual(st1, st1_copy)
        self.assertEqual(st2, st2_copy)
        self.assertEqual(st3, st3_copy)
        self.assertEqual(st1 == st2, False)
        self.assertEqual(st1 == st3, False)
        self.assertEqual(st2 == st3, False)
        self.assertEqual(st1 != st1, False)
        self.assertEqual(st2 != st2, False)
        self.assertEqual(st3 != st3, False)
        self.assertEqual(st1 != st1_copy, False)
        self.assertEqual(st2 != st2_copy, False)
        self.assertEqual(st3 != st3_copy, False)
        self.assertEqual(st2 != st1, True)
        self.assertEqual(st1 != st3, True)
        self.assertEqual(st3 != st2, True)
        # we don't particularly care what the ordering is;  just that
        # it's usable and self-consistent
        self.assertEqual(st1 < st2, not (st2 <= st1))
        self.assertEqual(st1 < st3, not (st3 <= st1))
        self.assertEqual(st2 < st3, not (st3 <= st2))
        self.assertEqual(st1 < st2, st2 > st1)
        self.assertEqual(st1 < st3, st3 > st1)
        self.assertEqual(st2 < st3, st3 > st2)
        self.assertEqual(st1 <= st2, st2 >= st1)
        self.assertEqual(st3 <= st1, st1 >= st3)
        self.assertEqual(st2 <= st3, st3 >= st2)
        # transitivity
        bottom = min(st1, st2, st3)
        top = max(st1, st2, st3)
        mid = sorted([st1, st2, st3])[1]
        self.assertTrue(bottom < mid)
        self.assertTrue(bottom < top)
        self.assertTrue(mid < top)
        self.assertTrue(bottom <= mid)
        self.assertTrue(bottom <= top)
        self.assertTrue(mid <= top)
        self.assertTrue(bottom <= bottom)
        self.assertTrue(mid <= mid)
        self.assertTrue(top <= top)
        # interaction with other types
        self.assertEqual(st1 == 1588.602459, False)
        self.assertEqual('spanish armada' != st2, True)
        self.assertRaises(TypeError, operator.ge, st3, None)
        self.assertRaises(TypeError, operator.le, False, st1)
        self.assertRaises(TypeError, operator.lt, st1, 1815)
        self.assertRaises(TypeError, operator.gt, b'waterloo', st2)

    def test_copy_pickle(self):
        sts = [
            parser.expr('2 + 3'),
            parser.suite('x = 2; y = x + 3'),
            parser.expr('list(x**3 for x in range(20))')
        ]
        for st in sts:
            st_copy = copy.copy(st)
            self.assertEqual(st_copy.totuple(), st.totuple())
            st_copy = copy.deepcopy(st)
            self.assertEqual(st_copy.totuple(), st.totuple())
            for proto in range(pickle.HIGHEST_PROTOCOL+1):
                st_copy = pickle.loads(pickle.dumps(st, proto))
                self.assertEqual(st_copy.totuple(), st.totuple())

    check_sizeof = support.check_sizeof

    @support.cpython_only
    def test_sizeof(self):
        def XXXROUNDUP(n):
            if n <= 1:
                return n
            if n <= 128:
                return (n + 3) & ~3
            return 1 << (n - 1).bit_length()

        basesize = support.calcobjsize('Piii')
        nodesize = struct.calcsize('hP3iP0h2i')
        def sizeofchildren(node):
            if node is None:
                return 0
            res = 0
            hasstr = len(node) > 1 and isinstance(node[-1], str)
            if hasstr:
                res += len(node[-1]) + 1
            children = node[1:-1] if hasstr else node[1:]
            if children:
                res += XXXROUNDUP(len(children)) * nodesize
                for child in children:
                    res += sizeofchildren(child)
            return res

        def check_st_sizeof(st):
            self.check_sizeof(st, basesize + nodesize +
                                  sizeofchildren(st.totuple()))

        check_st_sizeof(parser.expr('2 + 3'))
        check_st_sizeof(parser.expr('2 + 3 + 4'))
        check_st_sizeof(parser.suite('x = 2 + 3'))
        check_st_sizeof(parser.suite(''))
        check_st_sizeof(parser.suite('# -*- coding: utf-8 -*-'))
        check_st_sizeof(parser.expr('[' + '2,' * 1000 + ']'))


    # XXX tests for pickling and unpickling of ST objects should go here

class OtherParserCase(unittest.TestCase):

    def test_two_args_to_expr(self):
        # See bug #12264
        with self.assertRaises(TypeError):
            parser.expr("a", "b")

if __name__ == "__main__":
    unittest.main()
