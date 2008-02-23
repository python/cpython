import copy
import warnings
import unittest
from test.test_support import run_unittest, TestFailed

# Fake a number that implements numeric methods through __coerce__
class CoerceNumber:
    def __init__(self, arg):
        self.arg = arg

    def __repr__(self):
        return '<CoerceNumber %s>' % repr(self.arg)

    def __coerce__(self, other):
        if isinstance(other, CoerceNumber):
            return self.arg, other.arg
        else:
            return (self.arg, other)

# New-style class version of CoerceNumber
class CoerceTo(object):
    def __init__(self, arg):
        self.arg = arg
    def __coerce__(self, other):
        if isinstance(other, CoerceTo):
            return self.arg, other.arg
        else:
            return self.arg, other


# Fake a number that implements numeric ops through methods.
class MethodNumber:
    def __init__(self,arg):
        self.arg = arg

    def __repr__(self):
        return '<MethodNumber %s>' % repr(self.arg)

    def __add__(self,other):
        return self.arg + other

    def __radd__(self,other):
        return other + self.arg

    def __sub__(self,other):
        return self.arg - other

    def __rsub__(self,other):
        return other - self.arg

    def __mul__(self,other):
        return self.arg * other

    def __rmul__(self,other):
        return other * self.arg

    def __div__(self,other):
        return self.arg / other

    def __rdiv__(self,other):
        return other / self.arg

    def __truediv__(self,other):
        return self.arg / other

    def __rtruediv__(self,other):
        return other / self.arg

    def __floordiv__(self,other):
        return self.arg // other

    def __rfloordiv__(self,other):
        return other // self.arg

    def __pow__(self,other):
        return self.arg ** other

    def __rpow__(self,other):
        return other ** self.arg

    def __mod__(self,other):
        return self.arg % other

    def __rmod__(self,other):
        return other % self.arg

    def __cmp__(self, other):
        return cmp(self.arg, other)


candidates = [2, 2L, 4.0, 2+0j, [1], (2,), None,
              MethodNumber(2), CoerceNumber(2)]

infix_binops = [ '+', '-', '*', '**', '%', '//', '/' ]

TE = TypeError
# b = both normal and augmented give same result list
# s = single result lists for normal and augmented
# e = equals other results
# result lists: ['+', '-', '*', '**', '%', '//', ('classic /', 'new /')]
#                                                ^^^^^^^^^^^^^^^^^^^^^^
#                                               2-tuple if results differ
#                                                 else only one value
infix_results = {
    # 2
    (0,0): ('b', [4, 0, 4, 4, 0, 1, (1, 1.0)]),
    (0,1): ('e', (0,0)),
    (0,2): ('b', [6.0, -2.0, 8.0, 16.0, 2.0, 0.0, 0.5]),
    (0,3): ('b', [4+0j, 0+0j, 4+0j, 4+0j, 0+0j, 1+0j, 1+0j]),
    (0,4): ('b', [TE, TE, [1, 1], TE, TE, TE, TE]),
    (0,5): ('b', [TE, TE, (2, 2), TE, TE, TE, TE]),
    (0,6): ('b', [TE, TE, TE, TE, TE, TE, TE]),
    (0,7): ('e', (0,0)),
    (0,8): ('e', (0,0)),

    # 2L
    (1,0): ('e', (0,0)),
    (1,1): ('e', (0,1)),
    (1,2): ('e', (0,2)),
    (1,3): ('e', (0,3)),
    (1,4): ('e', (0,4)),
    (1,5): ('e', (0,5)),
    (1,6): ('e', (0,6)),
    (1,7): ('e', (0,7)),
    (1,8): ('e', (0,8)),

    # 4.0
    (2,0): ('b', [6.0, 2.0, 8.0, 16.0, 0.0, 2.0, 2.0]),
    (2,1): ('e', (2,0)),
    (2,2): ('b', [8.0, 0.0, 16.0, 256.0, 0.0, 1.0, 1.0]),
    (2,3): ('b', [6+0j, 2+0j, 8+0j, 16+0j, 0+0j, 2+0j, 2+0j]),
    (2,4): ('b', [TE, TE, TE, TE, TE, TE, TE]),
    (2,5): ('e', (2,4)),
    (2,6): ('e', (2,4)),
    (2,7): ('e', (2,0)),
    (2,8): ('e', (2,0)),

    # (2+0j)
    (3,0): ('b', [4+0j, 0+0j, 4+0j, 4+0j, 0+0j, 1+0j, 1+0j]),
    (3,1): ('e', (3,0)),
    (3,2): ('b', [6+0j, -2+0j, 8+0j, 16+0j, 2+0j, 0+0j, 0.5+0j]),
    (3,3): ('b', [4+0j, 0+0j, 4+0j, 4+0j, 0+0j, 1+0j, 1+0j]),
    (3,4): ('b', [TE, TE, TE, TE, TE, TE, TE]),
    (3,5): ('e', (3,4)),
    (3,6): ('e', (3,4)),
    (3,7): ('e', (3,0)),
    (3,8): ('e', (3,0)),

    # [1]
    (4,0): ('b', [TE, TE, [1, 1], TE, TE, TE, TE]),
    (4,1): ('e', (4,0)),
    (4,2): ('b', [TE, TE, TE, TE, TE, TE, TE]),
    (4,3): ('b', [TE, TE, TE, TE, TE, TE, TE]),
    (4,4): ('b', [[1, 1], TE, TE, TE, TE, TE, TE]),
    (4,5): ('s', [TE, TE, TE, TE, TE, TE, TE], [[1, 2], TE, TE, TE, TE, TE, TE]),
    (4,6): ('b', [TE, TE, TE, TE, TE, TE, TE]),
    (4,7): ('e', (4,0)),
    (4,8): ('e', (4,0)),

    # (2,)
    (5,0): ('b', [TE, TE, (2, 2), TE, TE, TE, TE]),
    (5,1): ('e', (5,0)),
    (5,2): ('b', [TE, TE, TE, TE, TE, TE, TE]),
    (5,3): ('e', (5,2)),
    (5,4): ('e', (5,2)),
    (5,5): ('b', [(2, 2), TE, TE, TE, TE, TE, TE]),
    (5,6): ('b', [TE, TE, TE, TE, TE, TE, TE]),
    (5,7): ('e', (5,0)),
    (5,8): ('e', (5,0)),

    # None
    (6,0): ('b', [TE, TE, TE, TE, TE, TE, TE]),
    (6,1): ('e', (6,0)),
    (6,2): ('e', (6,0)),
    (6,3): ('e', (6,0)),
    (6,4): ('e', (6,0)),
    (6,5): ('e', (6,0)),
    (6,6): ('e', (6,0)),
    (6,7): ('e', (6,0)),
    (6,8): ('e', (6,0)),

    # MethodNumber(2)
    (7,0): ('e', (0,0)),
    (7,1): ('e', (0,1)),
    (7,2): ('e', (0,2)),
    (7,3): ('e', (0,3)),
    (7,4): ('e', (0,4)),
    (7,5): ('e', (0,5)),
    (7,6): ('e', (0,6)),
    (7,7): ('e', (0,7)),
    (7,8): ('e', (0,8)),

    # CoerceNumber(2)
    (8,0): ('e', (0,0)),
    (8,1): ('e', (0,1)),
    (8,2): ('e', (0,2)),
    (8,3): ('e', (0,3)),
    (8,4): ('e', (0,4)),
    (8,5): ('e', (0,5)),
    (8,6): ('e', (0,6)),
    (8,7): ('e', (0,7)),
    (8,8): ('e', (0,8)),
}

def process_infix_results():
    for key in sorted(infix_results):
        val = infix_results[key]
        if val[0] == 'e':
            infix_results[key] = infix_results[val[1]]
        else:
            if val[0] == 's':
                res = (val[1], val[2])
            elif val[0] == 'b':
                res = (val[1], val[1])
            for i in range(1):
                if isinstance(res[i][6], tuple):
                    if 1/2 == 0:
                        # testing with classic (floor) division
                        res[i][6] = res[i][6][0]
                    else:
                        # testing with -Qnew
                        res[i][6] = res[i][6][1]
            infix_results[key] = res



process_infix_results()
# now infix_results has two lists of results for every pairing.

prefix_binops = [ 'divmod' ]
prefix_results = [
    [(1,0), (1L,0L), (0.0,2.0), ((1+0j),0j), TE, TE, TE, TE, (1,0)],
    [(1L,0L), (1L,0L), (0.0,2.0), ((1+0j),0j), TE, TE, TE, TE, (1L,0L)],
    [(2.0,0.0), (2.0,0.0), (1.0,0.0), ((2+0j),0j), TE, TE, TE, TE, (2.0,0.0)],
    [((1+0j),0j), ((1+0j),0j), (0j,(2+0j)), ((1+0j),0j), TE, TE, TE, TE, ((1+0j),0j)],
    [TE, TE, TE, TE, TE, TE, TE, TE, TE],
    [TE, TE, TE, TE, TE, TE, TE, TE, TE],
    [TE, TE, TE, TE, TE, TE, TE, TE, TE],
    [TE, TE, TE, TE, TE, TE, TE, TE, TE],
    [(1,0), (1L,0L), (0.0,2.0), ((1+0j),0j), TE, TE, TE, TE, (1,0)]
]

def format_float(value):
    if abs(value) < 0.01:
        return '0.0'
    else:
        return '%.1f' % value

# avoid testing platform fp quirks
def format_result(value):
    if isinstance(value, complex):
        return '(%s + %sj)' % (format_float(value.real),
                               format_float(value.imag))
    elif isinstance(value, float):
        return format_float(value)
    return str(value)

class CoercionTest(unittest.TestCase):
    def test_infix_binops(self):
        for ia, a in enumerate(candidates):
            for ib, b in enumerate(candidates):
                results = infix_results[(ia, ib)]
                for op, res, ires in zip(infix_binops, results[0], results[1]):
                    if res is TE:
                        self.assertRaises(TypeError, eval,
                                          'a %s b' % op, {'a': a, 'b': b})
                    else:
                        self.assertEquals(format_result(res),
                                          format_result(eval('a %s b' % op)),
                                          '%s %s %s == %s failed' % (a, op, b, res))
                    try:
                        z = copy.copy(a)
                    except copy.Error:
                        z = a # assume it has no inplace ops
                    if ires is TE:
                        try:
                            exec 'z %s= b' % op
                        except TypeError:
                            pass
                        else:
                            self.fail("TypeError not raised")
                    else:
                        exec('z %s= b' % op)
                        self.assertEquals(ires, z)

    def test_prefix_binops(self):
        for ia, a in enumerate(candidates):
            for ib, b in enumerate(candidates):
                for op in prefix_binops:
                    res = prefix_results[ia][ib]
                    if res is TE:
                        self.assertRaises(TypeError, eval,
                                          '%s(a, b)' % op, {'a': a, 'b': b})
                    else:
                        self.assertEquals(format_result(res),
                                          format_result(eval('%s(a, b)' % op)),
                                          '%s(%s, %s) == %s failed' % (op, a, b, res))

    def test_cmptypes(self):
        # Built-in tp_compare slots expect their arguments to have the
        # same type, but a user-defined __coerce__ doesn't have to obey.
        # SF #980352
        evil_coercer = CoerceTo(42)
        # Make sure these don't crash any more
        self.assertNotEquals(cmp(u'fish', evil_coercer), 0)
        self.assertNotEquals(cmp(slice(1), evil_coercer), 0)
        # ...but that this still works
        class WackyComparer(object):
            def __cmp__(slf, other):
                self.assert_(other == 42, 'expected evil_coercer, got %r' % other)
                return 0
        self.assertEquals(cmp(WackyComparer(), evil_coercer), 0)
        # ...and classic classes too, since that code path is a little different
        class ClassicWackyComparer:
            def __cmp__(slf, other):
                self.assert_(other == 42, 'expected evil_coercer, got %r' % other)
                return 0
        self.assertEquals(cmp(ClassicWackyComparer(), evil_coercer), 0)

    def test_infinite_rec_classic_classes(self):
        # if __coerce__() returns its arguments reversed it causes an infinite
        # recursion for classic classes.
        class Tester:
            def __coerce__(self, other):
                return other, self

        exc = TestFailed("__coerce__() returning its arguments reverse "
                                "should raise RuntimeError")
        try:
            Tester() + 1
        except (RuntimeError, TypeError):
            return
        except:
            raise exc
        else:
            raise exc

def test_main():
    warnings.filterwarnings("ignore",
                            r'complex divmod\(\), // and % are deprecated',
                            DeprecationWarning,
                            r'test.test_coercion$')
    run_unittest(CoercionTest)

if __name__ == "__main__":
    test_main()
