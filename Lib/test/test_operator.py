import operator
import unittest

from test import test_support

class Seq1:
    def __init__(self, lst):
        self.lst = lst
    def __len__(self):
        return len(self.lst)
    def __getitem__(self, i):
        return self.lst[i]
    def __add__(self, other):
        return self.lst + other.lst
    def __mul__(self, other):
        return self.lst * other
    def __rmul__(self, other):
        return other * self.lst

class Seq2(object):
    def __init__(self, lst):
        self.lst = lst
    def __len__(self):
        return len(self.lst)
    def __getitem__(self, i):
        return self.lst[i]
    def __add__(self, other):
        return self.lst + other.lst
    def __mul__(self, other):
        return self.lst * other
    def __rmul__(self, other):
        return other * self.lst


class OperatorTestCase(unittest.TestCase):
    def test_lt(self):
        self.failUnlessRaises(TypeError, operator.lt)
        self.failUnlessRaises(TypeError, operator.lt, 1j, 2j)
        self.failIf(operator.lt(1, 0))
        self.failIf(operator.lt(1, 0.0))
        self.failIf(operator.lt(1, 1))
        self.failIf(operator.lt(1, 1.0))
        self.failUnless(operator.lt(1, 2))
        self.failUnless(operator.lt(1, 2.0))

    def test_le(self):
        self.failUnlessRaises(TypeError, operator.le)
        self.failUnlessRaises(TypeError, operator.le, 1j, 2j)
        self.failIf(operator.le(1, 0))
        self.failIf(operator.le(1, 0.0))
        self.failUnless(operator.le(1, 1))
        self.failUnless(operator.le(1, 1.0))
        self.failUnless(operator.le(1, 2))
        self.failUnless(operator.le(1, 2.0))

    def test_eq(self):
        class C(object):
            def __eq__(self, other):
                raise SyntaxError
        self.failUnlessRaises(TypeError, operator.eq)
        self.failUnlessRaises(SyntaxError, operator.eq, C(), C())
        self.failIf(operator.eq(1, 0))
        self.failIf(operator.eq(1, 0.0))
        self.failUnless(operator.eq(1, 1))
        self.failUnless(operator.eq(1, 1.0))
        self.failIf(operator.eq(1, 2))
        self.failIf(operator.eq(1, 2.0))

    def test_ne(self):
        class C(object):
            def __ne__(self, other):
                raise SyntaxError
        self.failUnlessRaises(TypeError, operator.ne)
        self.failUnlessRaises(SyntaxError, operator.ne, C(), C())
        self.failUnless(operator.ne(1, 0))
        self.failUnless(operator.ne(1, 0.0))
        self.failIf(operator.ne(1, 1))
        self.failIf(operator.ne(1, 1.0))
        self.failUnless(operator.ne(1, 2))
        self.failUnless(operator.ne(1, 2.0))

    def test_ge(self):
        self.failUnlessRaises(TypeError, operator.ge)
        self.failUnlessRaises(TypeError, operator.ge, 1j, 2j)
        self.failUnless(operator.ge(1, 0))
        self.failUnless(operator.ge(1, 0.0))
        self.failUnless(operator.ge(1, 1))
        self.failUnless(operator.ge(1, 1.0))
        self.failIf(operator.ge(1, 2))
        self.failIf(operator.ge(1, 2.0))

    def test_gt(self):
        self.failUnlessRaises(TypeError, operator.gt)
        self.failUnlessRaises(TypeError, operator.gt, 1j, 2j)
        self.failUnless(operator.gt(1, 0))
        self.failUnless(operator.gt(1, 0.0))
        self.failIf(operator.gt(1, 1))
        self.failIf(operator.gt(1, 1.0))
        self.failIf(operator.gt(1, 2))
        self.failIf(operator.gt(1, 2.0))

    def test_abs(self):
        self.failUnlessRaises(TypeError, operator.abs)
        self.failUnlessRaises(TypeError, operator.abs, None)
        self.failUnless(operator.abs(-1) == 1)
        self.failUnless(operator.abs(1) == 1)

    def test_add(self):
        self.failUnlessRaises(TypeError, operator.add)
        self.failUnlessRaises(TypeError, operator.add, None, None)
        self.failUnless(operator.add(3, 4) == 7)

    def test_bitwise_and(self):
        self.failUnlessRaises(TypeError, operator.and_)
        self.failUnlessRaises(TypeError, operator.and_, None, None)
        self.failUnless(operator.and_(0xf, 0xa) == 0xa)

    def test_concat(self):
        self.failUnlessRaises(TypeError, operator.concat)
        self.failUnlessRaises(TypeError, operator.concat, None, None)
        self.failUnless(operator.concat('py', 'thon') == 'python')
        self.failUnless(operator.concat([1, 2], [3, 4]) == [1, 2, 3, 4])
        self.failUnless(operator.concat(Seq1([5, 6]), Seq1([7])) == [5, 6, 7])
        self.failUnless(operator.concat(Seq2([5, 6]), Seq2([7])) == [5, 6, 7])
        self.failUnlessRaises(TypeError, operator.concat, 13, 29)

    def test_countOf(self):
        self.failUnlessRaises(TypeError, operator.countOf)
        self.failUnlessRaises(TypeError, operator.countOf, None, None)
        self.failUnless(operator.countOf([1, 2, 1, 3, 1, 4], 3) == 1)
        self.failUnless(operator.countOf([1, 2, 1, 3, 1, 4], 5) == 0)

    def test_delitem(self):
        a = [4, 3, 2, 1]
        self.failUnlessRaises(TypeError, operator.delitem, a)
        self.failUnlessRaises(TypeError, operator.delitem, a, None)
        self.failUnless(operator.delitem(a, 1) is None)
        self.assert_(a == [4, 2, 1])

    def test_delslice(self):
        a = range(10)
        self.failUnlessRaises(TypeError, operator.delslice, a)
        self.failUnlessRaises(TypeError, operator.delslice, a, None, None)
        self.failUnless(operator.delslice(a, 2, 8) is None)
        self.assert_(a == [0, 1, 8, 9])
        operator.delslice(a, 0, test_support.MAX_Py_ssize_t)
        self.assert_(a == [])

    def test_div(self):
        self.failUnlessRaises(TypeError, operator.div, 5)
        self.failUnlessRaises(TypeError, operator.div, None, None)
        self.failUnless(operator.floordiv(5, 2) == 2)

    def test_floordiv(self):
        self.failUnlessRaises(TypeError, operator.floordiv, 5)
        self.failUnlessRaises(TypeError, operator.floordiv, None, None)
        self.failUnless(operator.floordiv(5, 2) == 2)

    def test_truediv(self):
        self.failUnlessRaises(TypeError, operator.truediv, 5)
        self.failUnlessRaises(TypeError, operator.truediv, None, None)
        self.failUnless(operator.truediv(5, 2) == 2.5)

    def test_getitem(self):
        a = range(10)
        self.failUnlessRaises(TypeError, operator.getitem)
        self.failUnlessRaises(TypeError, operator.getitem, a, None)
        self.failUnless(operator.getitem(a, 2) == 2)

    def test_getslice(self):
        a = range(10)
        self.failUnlessRaises(TypeError, operator.getslice)
        self.failUnlessRaises(TypeError, operator.getslice, a, None, None)
        self.failUnless(operator.getslice(a, 4, 6) == [4, 5])
        b = operator.getslice(a, 0, test_support.MAX_Py_ssize_t)
        self.assert_(b == a)

    def test_indexOf(self):
        self.failUnlessRaises(TypeError, operator.indexOf)
        self.failUnlessRaises(TypeError, operator.indexOf, None, None)
        self.failUnless(operator.indexOf([4, 3, 2, 1], 3) == 1)
        self.assertRaises(ValueError, operator.indexOf, [4, 3, 2, 1], 0)

    def test_invert(self):
        self.failUnlessRaises(TypeError, operator.invert)
        self.failUnlessRaises(TypeError, operator.invert, None)
        self.failUnless(operator.inv(4) == -5)

    def test_isCallable(self):
        self.failUnlessRaises(TypeError, operator.isCallable)
        class C:
            pass
        def check(self, o, v):
            self.assert_(operator.isCallable(o) == callable(o) == v)
        check(self, 4, 0)
        check(self, operator.isCallable, 1)
        check(self, C, 1)
        check(self, C(), 0)

    def test_isMappingType(self):
        self.failUnlessRaises(TypeError, operator.isMappingType)
        self.failIf(operator.isMappingType(1))
        self.failIf(operator.isMappingType(operator.isMappingType))
        self.failUnless(operator.isMappingType(operator.__dict__))
        self.failUnless(operator.isMappingType({}))

    def test_isNumberType(self):
        self.failUnlessRaises(TypeError, operator.isNumberType)
        self.failUnless(operator.isNumberType(8))
        self.failUnless(operator.isNumberType(8j))
        self.failUnless(operator.isNumberType(8L))
        self.failUnless(operator.isNumberType(8.3))
        self.failIf(operator.isNumberType(dir()))

    def test_isSequenceType(self):
        self.failUnlessRaises(TypeError, operator.isSequenceType)
        self.failUnless(operator.isSequenceType(dir()))
        self.failUnless(operator.isSequenceType(()))
        self.failUnless(operator.isSequenceType(xrange(10)))
        self.failUnless(operator.isSequenceType('yeahbuddy'))
        self.failIf(operator.isSequenceType(3))
        class Dict(dict): pass
        self.failIf(operator.isSequenceType(Dict()))

    def test_lshift(self):
        self.failUnlessRaises(TypeError, operator.lshift)
        self.failUnlessRaises(TypeError, operator.lshift, None, 42)
        self.failUnless(operator.lshift(5, 1) == 10)
        self.failUnless(operator.lshift(5, 0) == 5)
        self.assertRaises(ValueError, operator.lshift, 2, -1)

    def test_mod(self):
        self.failUnlessRaises(TypeError, operator.mod)
        self.failUnlessRaises(TypeError, operator.mod, None, 42)
        self.failUnless(operator.mod(5, 2) == 1)

    def test_mul(self):
        self.failUnlessRaises(TypeError, operator.mul)
        self.failUnlessRaises(TypeError, operator.mul, None, None)
        self.failUnless(operator.mul(5, 2) == 10)

    def test_neg(self):
        self.failUnlessRaises(TypeError, operator.neg)
        self.failUnlessRaises(TypeError, operator.neg, None)
        self.failUnless(operator.neg(5) == -5)
        self.failUnless(operator.neg(-5) == 5)
        self.failUnless(operator.neg(0) == 0)
        self.failUnless(operator.neg(-0) == 0)

    def test_bitwise_or(self):
        self.failUnlessRaises(TypeError, operator.or_)
        self.failUnlessRaises(TypeError, operator.or_, None, None)
        self.failUnless(operator.or_(0xa, 0x5) == 0xf)

    def test_pos(self):
        self.failUnlessRaises(TypeError, operator.pos)
        self.failUnlessRaises(TypeError, operator.pos, None)
        self.failUnless(operator.pos(5) == 5)
        self.failUnless(operator.pos(-5) == -5)
        self.failUnless(operator.pos(0) == 0)
        self.failUnless(operator.pos(-0) == 0)

    def test_pow(self):
        self.failUnlessRaises(TypeError, operator.pow)
        self.failUnlessRaises(TypeError, operator.pow, None, None)
        self.failUnless(operator.pow(3,5) == 3**5)
        self.failUnless(operator.__pow__(3,5) == 3**5)
        self.assertRaises(TypeError, operator.pow, 1)
        self.assertRaises(TypeError, operator.pow, 1, 2, 3)

    def test_repeat(self):
        a = range(3)
        self.failUnlessRaises(TypeError, operator.repeat)
        self.failUnlessRaises(TypeError, operator.repeat, a, None)
        self.failUnless(operator.repeat(a, 2) == a+a)
        self.failUnless(operator.repeat(a, 1) == a)
        self.failUnless(operator.repeat(a, 0) == [])
        a = (1, 2, 3)
        self.failUnless(operator.repeat(a, 2) == a+a)
        self.failUnless(operator.repeat(a, 1) == a)
        self.failUnless(operator.repeat(a, 0) == ())
        a = '123'
        self.failUnless(operator.repeat(a, 2) == a+a)
        self.failUnless(operator.repeat(a, 1) == a)
        self.failUnless(operator.repeat(a, 0) == '')
        a = Seq1([4, 5, 6])
        self.failUnless(operator.repeat(a, 2) == [4, 5, 6, 4, 5, 6])
        self.failUnless(operator.repeat(a, 1) == [4, 5, 6])
        self.failUnless(operator.repeat(a, 0) == [])
        a = Seq2([4, 5, 6])
        self.failUnless(operator.repeat(a, 2) == [4, 5, 6, 4, 5, 6])
        self.failUnless(operator.repeat(a, 1) == [4, 5, 6])
        self.failUnless(operator.repeat(a, 0) == [])
        self.failUnlessRaises(TypeError, operator.repeat, 6, 7)

    def test_rshift(self):
        self.failUnlessRaises(TypeError, operator.rshift)
        self.failUnlessRaises(TypeError, operator.rshift, None, 42)
        self.failUnless(operator.rshift(5, 1) == 2)
        self.failUnless(operator.rshift(5, 0) == 5)
        self.assertRaises(ValueError, operator.rshift, 2, -1)

    def test_contains(self):
        self.failUnlessRaises(TypeError, operator.contains)
        self.failUnlessRaises(TypeError, operator.contains, None, None)
        self.failUnless(operator.contains(range(4), 2))
        self.failIf(operator.contains(range(4), 5))
        self.failUnless(operator.sequenceIncludes(range(4), 2))
        self.failIf(operator.sequenceIncludes(range(4), 5))

    def test_setitem(self):
        a = range(3)
        self.failUnlessRaises(TypeError, operator.setitem, a)
        self.failUnlessRaises(TypeError, operator.setitem, a, None, None)
        self.failUnless(operator.setitem(a, 0, 2) is None)
        self.assert_(a == [2, 1, 2])
        self.assertRaises(IndexError, operator.setitem, a, 4, 2)

    def test_setslice(self):
        a = range(4)
        self.failUnlessRaises(TypeError, operator.setslice, a)
        self.failUnlessRaises(TypeError, operator.setslice, a, None, None, None)
        self.failUnless(operator.setslice(a, 1, 3, [2, 1]) is None)
        self.assert_(a == [0, 2, 1, 3])
        operator.setslice(a, 0, test_support.MAX_Py_ssize_t, [])
        self.assert_(a == [])

    def test_sub(self):
        self.failUnlessRaises(TypeError, operator.sub)
        self.failUnlessRaises(TypeError, operator.sub, None, None)
        self.failUnless(operator.sub(5, 2) == 3)

    def test_truth(self):
        class C(object):
            def __nonzero__(self):
                raise SyntaxError
        self.failUnlessRaises(TypeError, operator.truth)
        self.failUnlessRaises(SyntaxError, operator.truth, C())
        self.failUnless(operator.truth(5))
        self.failUnless(operator.truth([0]))
        self.failIf(operator.truth(0))
        self.failIf(operator.truth([]))

    def test_bitwise_xor(self):
        self.failUnlessRaises(TypeError, operator.xor)
        self.failUnlessRaises(TypeError, operator.xor, None, None)
        self.failUnless(operator.xor(0xb, 0xc) == 0x7)

    def test_is(self):
        a = b = 'xyzpdq'
        c = a[:3] + b[3:]
        self.failUnlessRaises(TypeError, operator.is_)
        self.failUnless(operator.is_(a, b))
        self.failIf(operator.is_(a,c))

    def test_is_not(self):
        a = b = 'xyzpdq'
        c = a[:3] + b[3:]
        self.failUnlessRaises(TypeError, operator.is_not)
        self.failIf(operator.is_not(a, b))
        self.failUnless(operator.is_not(a,c))

    def test_attrgetter(self):
        class A:
            pass
        a = A()
        a.name = 'arthur'
        f = operator.attrgetter('name')
        self.assertEqual(f(a), 'arthur')
        f = operator.attrgetter('rank')
        self.assertRaises(AttributeError, f, a)
        f = operator.attrgetter(2)
        self.assertRaises(TypeError, f, a)
        self.assertRaises(TypeError, operator.attrgetter)

        # multiple gets
        record = A()
        record.x = 'X'
        record.y = 'Y'
        record.z = 'Z'
        self.assertEqual(operator.attrgetter('x','z','y')(record), ('X', 'Z', 'Y'))
        self.assertRaises(TypeError, operator.attrgetter('x', (), 'y'), record)

        class C(object):
            def __getattr__(self, name):
                raise SyntaxError
        self.failUnlessRaises(SyntaxError, operator.attrgetter('foo'), C())

        # recursive gets
        a = A()
        a.name = 'arthur'
        a.child = A()
        a.child.name = 'thomas'
        f = operator.attrgetter('child.name')
        self.assertEqual(f(a), 'thomas')
        self.assertRaises(AttributeError, f, a.child)
        f = operator.attrgetter('name', 'child.name')
        self.assertEqual(f(a), ('arthur', 'thomas'))
        f = operator.attrgetter('name', 'child.name', 'child.child.name')
        self.assertRaises(AttributeError, f, a)

        a.child.child = A()
        a.child.child.name = 'johnson'
        f = operator.attrgetter('child.child.name')
        self.assertEqual(f(a), 'johnson')
        f = operator.attrgetter('name', 'child.name', 'child.child.name')
        self.assertEqual(f(a), ('arthur', 'thomas', 'johnson'))

    def test_itemgetter(self):
        a = 'ABCDE'
        f = operator.itemgetter(2)
        self.assertEqual(f(a), 'C')
        f = operator.itemgetter(10)
        self.assertRaises(IndexError, f, a)

        class C(object):
            def __getitem__(self, name):
                raise SyntaxError
        self.failUnlessRaises(SyntaxError, operator.itemgetter(42), C())

        f = operator.itemgetter('name')
        self.assertRaises(TypeError, f, a)
        self.assertRaises(TypeError, operator.itemgetter)

        d = dict(key='val')
        f = operator.itemgetter('key')
        self.assertEqual(f(d), 'val')
        f = operator.itemgetter('nonkey')
        self.assertRaises(KeyError, f, d)

        # example used in the docs
        inventory = [('apple', 3), ('banana', 2), ('pear', 5), ('orange', 1)]
        getcount = operator.itemgetter(1)
        self.assertEqual(map(getcount, inventory), [3, 2, 5, 1])
        self.assertEqual(sorted(inventory, key=getcount),
            [('orange', 1), ('banana', 2), ('apple', 3), ('pear', 5)])

        # multiple gets
        data = map(str, range(20))
        self.assertEqual(operator.itemgetter(2,10,5)(data), ('2', '10', '5'))
        self.assertRaises(TypeError, operator.itemgetter(2, 'x', 5), data)

    def test_inplace(self):
        class C(object):
            def __iadd__     (self, other): return "iadd"
            def __iand__     (self, other): return "iand"
            def __idiv__     (self, other): return "idiv"
            def __ifloordiv__(self, other): return "ifloordiv"
            def __ilshift__  (self, other): return "ilshift"
            def __imod__     (self, other): return "imod"
            def __imul__     (self, other): return "imul"
            def __ior__      (self, other): return "ior"
            def __ipow__     (self, other): return "ipow"
            def __irshift__  (self, other): return "irshift"
            def __isub__     (self, other): return "isub"
            def __itruediv__ (self, other): return "itruediv"
            def __ixor__     (self, other): return "ixor"
            def __getitem__(self, other): return 5  # so that C is a sequence
        c = C()
        self.assertEqual(operator.iadd     (c, 5), "iadd")
        self.assertEqual(operator.iand     (c, 5), "iand")
        self.assertEqual(operator.idiv     (c, 5), "idiv")
        self.assertEqual(operator.ifloordiv(c, 5), "ifloordiv")
        self.assertEqual(operator.ilshift  (c, 5), "ilshift")
        self.assertEqual(operator.imod     (c, 5), "imod")
        self.assertEqual(operator.imul     (c, 5), "imul")
        self.assertEqual(operator.ior      (c, 5), "ior")
        self.assertEqual(operator.ipow     (c, 5), "ipow")
        self.assertEqual(operator.irshift  (c, 5), "irshift")
        self.assertEqual(operator.isub     (c, 5), "isub")
        self.assertEqual(operator.itruediv (c, 5), "itruediv")
        self.assertEqual(operator.ixor     (c, 5), "ixor")
        self.assertEqual(operator.iconcat  (c, c), "iadd")
        self.assertEqual(operator.irepeat  (c, 5), "imul")
        self.assertEqual(operator.__iadd__     (c, 5), "iadd")
        self.assertEqual(operator.__iand__     (c, 5), "iand")
        self.assertEqual(operator.__idiv__     (c, 5), "idiv")
        self.assertEqual(operator.__ifloordiv__(c, 5), "ifloordiv")
        self.assertEqual(operator.__ilshift__  (c, 5), "ilshift")
        self.assertEqual(operator.__imod__     (c, 5), "imod")
        self.assertEqual(operator.__imul__     (c, 5), "imul")
        self.assertEqual(operator.__ior__      (c, 5), "ior")
        self.assertEqual(operator.__ipow__     (c, 5), "ipow")
        self.assertEqual(operator.__irshift__  (c, 5), "irshift")
        self.assertEqual(operator.__isub__     (c, 5), "isub")
        self.assertEqual(operator.__itruediv__ (c, 5), "itruediv")
        self.assertEqual(operator.__ixor__     (c, 5), "ixor")
        self.assertEqual(operator.__iconcat__  (c, c), "iadd")
        self.assertEqual(operator.__irepeat__  (c, 5), "imul")

def test_main(verbose=None):
    import sys
    test_classes = (
        OperatorTestCase,
    )

    test_support.run_unittest(*test_classes)

    # verify reference counting
    if verbose and hasattr(sys, "gettotalrefcount"):
        import gc
        counts = [None] * 5
        for i in xrange(len(counts)):
            test_support.run_unittest(*test_classes)
            gc.collect()
            counts[i] = sys.gettotalrefcount()
        print counts

if __name__ == "__main__":
    test_main(verbose=True)
