import operator
import unittest

from test import test_support


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
        self.assertRaises(TypeError, operator.attrgetter, 1, 2)

        class C(object):
            def __getattr(self, name):
                raise SyntaxError
        self.failUnlessRaises(AttributeError, operator.attrgetter('foo'), C())

    def test_itemgetter(self):
        a = 'ABCDE'
        f = operator.itemgetter(2)
        self.assertEqual(f(a), 'C')
        f = operator.itemgetter(10)
        self.assertRaises(IndexError, f, a)

        class C(object):
            def __getitem(self, name):
                raise SyntaxError
        self.failUnlessRaises(TypeError, operator.itemgetter(42), C())

        f = operator.itemgetter('name')
        self.assertRaises(TypeError, f, a)
        self.assertRaises(TypeError, operator.itemgetter)
        self.assertRaises(TypeError, operator.itemgetter, 1, 2)

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

def test_main():
    test_support.run_unittest(OperatorTestCase)


if __name__ == "__main__":
    test_main()
