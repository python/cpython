import operator
import unittest

from test import test_support


class OperatorTestCase(unittest.TestCase):
    def test_lt(self):
        self.failIf(operator.lt(1, 0))
        self.failIf(operator.lt(1, 0.0))
        self.failIf(operator.lt(1, 1))
        self.failIf(operator.lt(1, 1.0))
        self.failUnless(operator.lt(1, 2))
        self.failUnless(operator.lt(1, 2.0))

    def test_le(self):
        self.failIf(operator.le(1, 0))
        self.failIf(operator.le(1, 0.0))
        self.failUnless(operator.le(1, 1))
        self.failUnless(operator.le(1, 1.0))
        self.failUnless(operator.le(1, 2))
        self.failUnless(operator.le(1, 2.0))

    def test_eq(self):
        self.failIf(operator.eq(1, 0))
        self.failIf(operator.eq(1, 0.0))
        self.failUnless(operator.eq(1, 1))
        self.failUnless(operator.eq(1, 1.0))
        self.failIf(operator.eq(1, 2))
        self.failIf(operator.eq(1, 2.0))

    def test_ne(self):
        self.failUnless(operator.ne(1, 0))
        self.failUnless(operator.ne(1, 0.0))
        self.failIf(operator.ne(1, 1))
        self.failIf(operator.ne(1, 1.0))
        self.failUnless(operator.ne(1, 2))
        self.failUnless(operator.ne(1, 2.0))

    def test_ge(self):
        self.failUnless(operator.ge(1, 0))
        self.failUnless(operator.ge(1, 0.0))
        self.failUnless(operator.ge(1, 1))
        self.failUnless(operator.ge(1, 1.0))
        self.failIf(operator.ge(1, 2))
        self.failIf(operator.ge(1, 2.0))

    def test_gt(self):
        self.failUnless(operator.gt(1, 0))
        self.failUnless(operator.gt(1, 0.0))
        self.failIf(operator.gt(1, 1))
        self.failIf(operator.gt(1, 1.0))
        self.failIf(operator.gt(1, 2))
        self.failIf(operator.gt(1, 2.0))

    def test_abs(self):
        self.failUnless(operator.abs(-1) == 1)
        self.failUnless(operator.abs(1) == 1)

    def test_add(self):
        self.failUnless(operator.add(3, 4) == 7)

    def test_bitwise_and(self):
        self.failUnless(operator.and_(0xf, 0xa) == 0xa)

    def test_concat(self):
        self.failUnless(operator.concat('py', 'thon') == 'python')
        self.failUnless(operator.concat([1, 2], [3, 4]) == [1, 2, 3, 4])

    def test_countOf(self):
        self.failUnless(operator.countOf([1, 2, 1, 3, 1, 4], 3) == 1)
        self.failUnless(operator.countOf([1, 2, 1, 3, 1, 4], 5) == 0)

    def test_delitem(self):
        a = [4, 3, 2, 1]
        self.failUnless(operator.delitem(a, 1) is None)
        self.assert_(a == [4, 2, 1])

    def test_delslice(self):
        a = range(10)
        self.failUnless(operator.delslice(a, 2, 8) is None)
        self.assert_(a == [0, 1, 8, 9])

    def test_div(self):
        self.failUnless(operator.floordiv(5, 2) == 2)

    def test_floordiv(self):
        self.failUnless(operator.floordiv(5, 2) == 2)

    def test_truediv(self):
        self.failUnless(operator.truediv(5, 2) == 2.5)

    def test_getitem(self):
        a = range(10)
        self.failUnless(operator.getitem(a, 2) == 2)

    def test_getslice(self):
        a = range(10)
        self.failUnless(operator.getslice(a, 4, 6) == [4, 5])

    def test_indexOf(self):
        self.failUnless(operator.indexOf([4, 3, 2, 1], 3) == 1)
        self.assertRaises(ValueError, operator.indexOf, [4, 3, 2, 1], 0)

    def test_invert(self):
        self.failUnless(operator.inv(4) == -5)

    def test_isCallable(self):
        class C:
            pass
        def check(self, o, v):
            self.assert_(operator.isCallable(o) == callable(o) == v)
        check(self, 4, 0)
        check(self, operator.isCallable, 1)
        check(self, C, 1)
        check(self, C(), 0)

    def test_isMappingType(self):
        self.failIf(operator.isMappingType(1))
        self.failIf(operator.isMappingType(operator.isMappingType))
        self.failUnless(operator.isMappingType(operator.__dict__))
        self.failUnless(operator.isMappingType({}))

    def test_isNumberType(self):
        self.failUnless(operator.isNumberType(8))
        self.failUnless(operator.isNumberType(8j))
        self.failUnless(operator.isNumberType(8L))
        self.failUnless(operator.isNumberType(8.3))
        self.failIf(operator.isNumberType(dir()))

    def test_isSequenceType(self):
        self.failUnless(operator.isSequenceType(dir()))
        self.failUnless(operator.isSequenceType(()))
        self.failUnless(operator.isSequenceType(xrange(10)))
        self.failUnless(operator.isSequenceType('yeahbuddy'))
        self.failIf(operator.isSequenceType(3))

    def test_lshift(self):
        self.failUnless(operator.lshift(5, 1) == 10)
        self.failUnless(operator.lshift(5, 0) == 5)
        self.assertRaises(ValueError, operator.lshift, 2, -1)

    def test_mod(self):
        self.failUnless(operator.mod(5, 2) == 1)

    def test_mul(self):
        self.failUnless(operator.mul(5, 2) == 10)

    def test_neg(self):
        self.failUnless(operator.neg(5) == -5)
        self.failUnless(operator.neg(-5) == 5)
        self.failUnless(operator.neg(0) == 0)
        self.failUnless(operator.neg(-0) == 0)

    def test_bitwise_or(self):
        self.failUnless(operator.or_(0xa, 0x5) == 0xf)

    def test_pos(self):
        self.failUnless(operator.pos(5) == 5)
        self.failUnless(operator.pos(-5) == -5)
        self.failUnless(operator.pos(0) == 0)
        self.failUnless(operator.pos(-0) == 0)

    def test_pow(self):
        self.failUnless(operator.pow(3,5) == 3**5)
        self.failUnless(operator.__pow__(3,5) == 3**5)
        self.assertRaises(TypeError, operator.pow, 1)
        self.assertRaises(TypeError, operator.pow, 1, 2, 3)

    def test_repeat(self):
        a = range(3)
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
        self.failUnless(operator.rshift(5, 1) == 2)
        self.failUnless(operator.rshift(5, 0) == 5)
        self.assertRaises(ValueError, operator.rshift, 2, -1)

    def test_contains(self):
        self.failUnless(operator.contains(range(4), 2))
        self.failIf(operator.contains(range(4), 5))
        self.failUnless(operator.sequenceIncludes(range(4), 2))
        self.failIf(operator.sequenceIncludes(range(4), 5))

    def test_setitem(self):
        a = range(3)
        self.failUnless(operator.setitem(a, 0, 2) is None)
        self.assert_(a == [2, 1, 2])
        self.assertRaises(IndexError, operator.setitem, a, 4, 2)

    def test_setslice(self):
        a = range(4)
        self.failUnless(operator.setslice(a, 1, 3, [2, 1]) is None)
        self.assert_(a == [0, 2, 1, 3])

    def test_sub(self):
        self.failUnless(operator.sub(5, 2) == 3)

    def test_truth(self):
        self.failUnless(operator.truth(5))
        self.failUnless(operator.truth([0]))
        self.failIf(operator.truth(0))
        self.failIf(operator.truth([]))

    def test_bitwise_xor(self):
        self.failUnless(operator.xor(0xb, 0xc) == 0x7)

    def test_is(self):
        a = b = 'xyzpdq'
        c = a[:3] + b[3:]
        self.failUnless(operator.is_(a, b))
        self.failIf(operator.is_(a,c))

    def test_is_not(self):
        a = b = 'xyzpdq'
        c = a[:3] + b[3:]
        self.failIf(operator.is_not(a, b))
        self.failUnless(operator.is_not(a,c))

def test_main():
    test_support.run_unittest(OperatorTestCase)


if __name__ == "__main__":
    test_main()
