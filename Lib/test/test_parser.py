import os.path
import parser
import pprint
import sys

from test_support import TestFailed

#
#  First, we test that we can generate trees from valid source fragments,
#  and that these valid trees are indeed allowed by the tree-loading side
#  of the parser module.
#

def roundtrip(f, s):
    st1 = f(s)
    t = st1.totuple()
    try:
        st2 = parser.sequence2ast(t)
    except parser.ParserError:
        raise TestFailed, s

def roundtrip_fromfile(filename):
    roundtrip(parser.suite, open(filename).read())

def test_expr(s):
    print "expr:", s
    roundtrip(parser.expr, s)

def test_suite(s):
    print "suite:", s
    roundtrip(parser.suite, s)


print "Expressions:"

test_expr("foo(1)")
test_expr("[1, 2, 3]")
test_expr("[x**3 for x in range(20)]")
test_expr("[x**3 for x in range(20) if x % 3]")
test_expr("foo(*args)")
test_expr("foo(*args, **kw)")
test_expr("foo(**kw)")
test_expr("foo(key=value)")
test_expr("foo(key=value, *args)")
test_expr("foo(key=value, *args, **kw)")
test_expr("foo(key=value, **kw)")
test_expr("foo(a, b, c, *args)")
test_expr("foo(a, b, c, *args, **kw)")
test_expr("foo(a, b, c, **kw)")
test_expr("foo + bar")
test_expr("lambda: 0")
test_expr("lambda x: 0")
test_expr("lambda *y: 0")
test_expr("lambda *y, **z: 0")
test_expr("lambda **z: 0")
test_expr("lambda x, y: 0")
test_expr("lambda foo=bar: 0")
test_expr("lambda foo=bar, spaz=nifty+spit: 0")
test_expr("lambda foo=bar, **z: 0")
test_expr("lambda foo=bar, blaz=blat+2, **z: 0")
test_expr("lambda foo=bar, blaz=blat+2, *y, **z: 0")
test_expr("lambda x, *y, **z: 0")

print
print "Statements:"
test_suite("print")
test_suite("print 1")
test_suite("print 1,")
test_suite("print >>fp")
test_suite("print >>fp, 1")
test_suite("print >>fp, 1,")

# expr_stmt
test_suite("a")
test_suite("a = b")
test_suite("a = b = c = d = e")
test_suite("a += b")
test_suite("a -= b")
test_suite("a *= b")
test_suite("a /= b")
test_suite("a %= b")
test_suite("a &= b")
test_suite("a |= b")
test_suite("a ^= b")
test_suite("a <<= b")
test_suite("a >>= b")
test_suite("a **= b")

test_suite("def f(): pass")
test_suite("def f(*args): pass")
test_suite("def f(*args, **kw): pass")
test_suite("def f(**kw): pass")
test_suite("def f(foo=bar): pass")
test_suite("def f(foo=bar, *args): pass")
test_suite("def f(foo=bar, *args, **kw): pass")
test_suite("def f(foo=bar, **kw): pass")

test_suite("def f(a, b): pass")
test_suite("def f(a, b, *args): pass")
test_suite("def f(a, b, *args, **kw): pass")
test_suite("def f(a, b, **kw): pass")
test_suite("def f(a, b, foo=bar): pass")
test_suite("def f(a, b, foo=bar, *args): pass")
test_suite("def f(a, b, foo=bar, *args, **kw): pass")
test_suite("def f(a, b, foo=bar, **kw): pass")

test_suite("from sys.path import *")
test_suite("from sys.path import dirname")
test_suite("from sys.path import dirname as my_dirname")
test_suite("from sys.path import dirname, basename")
test_suite("from sys.path import dirname as my_dirname, basename")
test_suite("from sys.path import dirname, basename as my_basename")

test_suite("import sys")
test_suite("import sys as system")
test_suite("import sys, math")
test_suite("import sys as system, math")
test_suite("import sys, math as my_math")

#d = os.path.dirname(os.__file__)
#roundtrip_fromfile(os.path.join(d, "os.py"))
#roundtrip_fromfile(os.path.join(d, "test", "test_parser.py"))

#
#  Second, we take *invalid* trees and make sure we get ParserError
#  rejections for them.
#

print
print "Invalid parse trees:"

def check_bad_tree(tree, label):
    print
    print label
    try:
        parser.sequence2ast(tree)
    except parser.ParserError:
        print "caught expected exception for invalid tree"
    else:
        print "test failed: did not properly detect invalid tree:"
        pprint.pprint(tree)


# not even remotely valid:
check_bad_tree((1, 2, 3), "<junk>")

# print >>fp,
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

check_bad_tree(tree, "print >>fp,")

# a,,c
tree = \
(258,
 (311,
  (290,
   (291,
    (292,
     (293,
      (295,
       (296, (297, (298, (299, (300, (301, (302, (303, (1, 'a')))))))))))))),
  (12, ','),
  (12, ','),
  (290,
   (291,
    (292,
     (293,
      (295,
       (296, (297, (298, (299, (300, (301, (302, (303, (1, 'c'))))))))))))))),
 (4, ''),
 (0, ''))

check_bad_tree(tree, "a,,c")

# a $= b
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
             (299, (300, (301, (302, (303, (304, (1, 'a'))))))))))))))),
     (268, (37, '$=')),
     (312,
      (291,
       (292,
        (293,
         (294,
          (296,
           (297,
            (298,
             (299, (300, (301, (302, (303, (304, (1, 'b'))))))))))))))))),
   (4, ''))),
 (0, ''))

check_bad_tree(tree, "a $= b")
