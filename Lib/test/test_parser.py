import parser
import test_support
import unittest

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
            st2 = parser.sequence2ast(t)
        except parser.ParserError:
            self.fail("could not roundtrip %r" % s)

        self.assertEquals(t, st2.totuple(),
                          "could not re-generate syntax tree")

    def check_expr(self, s):
        self.roundtrip(parser.expr, s)

    def check_suite(self, s):
        self.roundtrip(parser.suite, s)

    def test_expressions(self):
        self.check_expr("foo(1)")
        self.check_expr("[1, 2, 3]")
        self.check_expr("[x**3 for x in range(20)]")
        self.check_expr("[x**3 for x in range(20) if x % 3]")
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
        self.check_expr("foo + bar")
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

    def test_print(self):
        self.check_suite("print")
        self.check_suite("print 1")
        self.check_suite("print 1,")
        self.check_suite("print >>fp")
        self.check_suite("print >>fp, 1")
        self.check_suite("print >>fp, 1,")

    def test_simple_expression(self):
        # expr_stmt
        self.check_suite("a")

    def test_simple_assignments(self):
        self.check_suite("a = b")
        self.check_suite("a = b = c = d = e")

    def test_simple_augmented_assignments(self):
        self.check_suite("a += b")
        self.check_suite("a -= b")
        self.check_suite("a *= b")
        self.check_suite("a /= b")
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

    def test_import_from_statement(self):
        self.check_suite("from sys.path import *")
        self.check_suite("from sys.path import dirname")
        self.check_suite("from sys.path import dirname as my_dirname")
        self.check_suite("from sys.path import dirname, basename")
        self.check_suite(
            "from sys.path import dirname as my_dirname, basename")
        self.check_suite(
            "from sys.path import dirname, basename as my_basename")

    def test_basic_import_statement(self):
        self.check_suite("import sys")
        self.check_suite("import sys as system")
        self.check_suite("import sys, math")
        self.check_suite("import sys as system, math")
        self.check_suite("import sys, math as my_math")

#
#  Second, we take *invalid* trees and make sure we get ParserError
#  rejections for them.
#

class IllegalSyntaxTestCase(unittest.TestCase):
    def check_bad_tree(self, tree, label):
        try:
            parser.sequence2ast(tree)
        except parser.ParserError:
            pass
        else:
            self.fail("did not detect invalid tree for %r" % label)

    def test_junk(self):
        # not even remotely valid:
        self.check_bad_tree((1, 2, 3), "<junk>")

    def test_print_chevron_comma(self):
        "Illegal input: print >>fp,"""
        tree = \
        (257,
         (264,
          (265,
           (266,
            (268,
             (1, 'print'),
             (35, '>>'),
             (290,
              (291,
               (292,
                (293,
                 (295,
                  (296,
                   (297,
                    (298, (299, (300, (301, (302, (303, (1, 'fp')))))))))))))),
             (12, ','))),
           (4, ''))),
         (0, ''))
        self.check_bad_tree(tree, "print >>fp,")

    def test_a_comma_comma_c(self):
        """Illegal input: a,,c"""
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
        """Illegal input: a $= b"""
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


test_support.run_unittest(RoundtripLegalSyntaxTestCase)
test_support.run_unittest(IllegalSyntaxTestCase)
