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
    print s
    st1 = f(s)
    t = st1.totuple()
    st2 = parser.sequence2ast(t)


print "Expressions:"

roundtrip(expr, "foo(1)")
roundtrip(expr, "[1, 2, 3]")
roundtrip(expr, "[x**3 for x in range(20)]")
roundtrip(expr, "[x**3 for x in range(20) if x % 3]")
roundtrip(expr, "foo(*args)")
roundtrip(expr, "foo(*args, **kw)")
roundtrip(expr, "foo(**kw)")
roundtrip(expr, "foo(key=value)")
roundtrip(expr, "foo(key=value, *args)")
roundtrip(expr, "foo(key=value, *args, **kw)")
roundtrip(expr, "foo(key=value, **kw)")
roundtrip(expr, "foo(a, b, c, *args)")
roundtrip(expr, "foo(a, b, c, *args, **kw)")
roundtrip(expr, "foo(a, b, c, **kw)")
roundtrip(expr, "foo + bar")

print
print "Statements:"
roundtrip(suite, "print")
roundtrip(suite, "print 1")
roundtrip(suite, "print 1,")
roundtrip(suite, "print >>fp")
roundtrip(suite, "print >>fp, 1")
roundtrip(suite, "print >>fp, 1,")

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
