import os.path
import parser
import pprint
import sys

from parser import expr, suite, sequence2ast
from test_support import verbose

#
#  First, we test that we can generate trees from valid source fragments,
#  and that these valid trees are indeed allowed by the tree-loading side
#  of the parser module.
#

def roundtrip(f, s):
    st1 = f(s)
    t = st1.totuple()
    st2 = parser.sequence2ast(t)

def roundtrip_fromfile(filename):
    roundtrip(suite, open(filename).read())

def test_expr(s):
    print "expr:", s
    roundtrip(expr, s)

def test_suite(s):
    print "suite:", s
    roundtrip(suite, s)


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
        sequence2ast(tree)
    except parser.ParserError:
        print "caught expected exception for invalid tree"
        pass
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
