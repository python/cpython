# Augmented assignment test.

from test.test_support import run_unittest, _check_py3k_warnings
import unittest


class AugAssignTest(unittest.TestCase):
    def testBasic(self):
        x = 2
        x += 1
        x *= 2
        x **= 2
        x -= 8
        x //= 5
        x %= 3
        x &= 2
        x |= 5
        x ^= 1
        x /= 2
        if 1/2 == 0:
            # classic division
            self.assertEquals(x, 3)
        else:
            # new-style division (with -Qnew)
            self.assertEquals(x, 3.0)

    def testInList(self):
        x = [2]
        x[0] += 1
        x[0] *= 2
        x[0] **= 2
        x[0] -= 8
        x[0] //= 5
        x[0] %= 3
        x[0] &= 2
        x[0] |= 5
        x[0] ^= 1
        x[0] /= 2
        if 1/2 == 0:
            self.assertEquals(x[0], 3)
        else:
            self.assertEquals(x[0], 3.0)

    def testInDict(self):
        x = {0: 2}
        x[0] += 1
        x[0] *= 2
        x[0] **= 2
        x[0] -= 8
        x[0] //= 5
        x[0] %= 3
        x[0] &= 2
        x[0] |= 5
        x[0] ^= 1
        x[0] /= 2
        if 1/2 == 0:
            self.assertEquals(x[0], 3)
        else:
            self.assertEquals(x[0], 3.0)

    def testSequences(self):
        x = [1,2]
        x += [3,4]
        x *= 2

        self.assertEquals(x, [1, 2, 3, 4, 1, 2, 3, 4])

        x = [1, 2, 3]
        y = x
        x[1:2] *= 2
        y[1:2] += [1]

        self.assertEquals(x, [1, 2, 1, 2, 3])
        self.assert_(x is y)

    def testCustomMethods1(self):

        class aug_test:
            def __init__(self, value):
                self.val = value
            def __radd__(self, val):
                return self.val + val
            def __add__(self, val):
                return aug_test(self.val + val)

        class aug_test2(aug_test):
            def __iadd__(self, val):
                self.val = self.val + val
                return self

        class aug_test3(aug_test):
            def __iadd__(self, val):
                return aug_test3(self.val + val)

        x = aug_test(1)
        y = x
        x += 10

        self.assert_(isinstance(x, aug_test))
        self.assert_(y is not x)
        self.assertEquals(x.val, 11)

        x = aug_test2(2)
        y = x
        x += 10

        self.assert_(y is x)
        self.assertEquals(x.val, 12)

        x = aug_test3(3)
        y = x
        x += 10

        self.assert_(isinstance(x, aug_test3))
        self.assert_(y is not x)
        self.assertEquals(x.val, 13)


    def testCustomMethods2(test_self):
        output = []

        class testall:
            def __add__(self, val):
                output.append("__add__ called")
            def __radd__(self, val):
                output.append("__radd__ called")
            def __iadd__(self, val):
                output.append("__iadd__ called")
                return self

            def __sub__(self, val):
                output.append("__sub__ called")
            def __rsub__(self, val):
                output.append("__rsub__ called")
            def __isub__(self, val):
                output.append("__isub__ called")
                return self

            def __mul__(self, val):
                output.append("__mul__ called")
            def __rmul__(self, val):
                output.append("__rmul__ called")
            def __imul__(self, val):
                output.append("__imul__ called")
                return self

            def __div__(self, val):
                output.append("__div__ called")
            def __rdiv__(self, val):
                output.append("__rdiv__ called")
            def __idiv__(self, val):
                output.append("__idiv__ called")
                return self

            def __floordiv__(self, val):
                output.append("__floordiv__ called")
                return self
            def __ifloordiv__(self, val):
                output.append("__ifloordiv__ called")
                return self
            def __rfloordiv__(self, val):
                output.append("__rfloordiv__ called")
                return self

            def __truediv__(self, val):
                output.append("__truediv__ called")
                return self
            def __itruediv__(self, val):
                output.append("__itruediv__ called")
                return self

            def __mod__(self, val):
                output.append("__mod__ called")
            def __rmod__(self, val):
                output.append("__rmod__ called")
            def __imod__(self, val):
                output.append("__imod__ called")
                return self

            def __pow__(self, val):
                output.append("__pow__ called")
            def __rpow__(self, val):
                output.append("__rpow__ called")
            def __ipow__(self, val):
                output.append("__ipow__ called")
                return self

            def __or__(self, val):
                output.append("__or__ called")
            def __ror__(self, val):
                output.append("__ror__ called")
            def __ior__(self, val):
                output.append("__ior__ called")
                return self

            def __and__(self, val):
                output.append("__and__ called")
            def __rand__(self, val):
                output.append("__rand__ called")
            def __iand__(self, val):
                output.append("__iand__ called")
                return self

            def __xor__(self, val):
                output.append("__xor__ called")
            def __rxor__(self, val):
                output.append("__rxor__ called")
            def __ixor__(self, val):
                output.append("__ixor__ called")
                return self

            def __rshift__(self, val):
                output.append("__rshift__ called")
            def __rrshift__(self, val):
                output.append("__rrshift__ called")
            def __irshift__(self, val):
                output.append("__irshift__ called")
                return self

            def __lshift__(self, val):
                output.append("__lshift__ called")
            def __rlshift__(self, val):
                output.append("__rlshift__ called")
            def __ilshift__(self, val):
                output.append("__ilshift__ called")
                return self

        x = testall()
        x + 1
        1 + x
        x += 1

        x - 1
        1 - x
        x -= 1

        x * 1
        1 * x
        x *= 1

        if 1/2 == 0:
            x / 1
            1 / x
            x /= 1
        else:
            # True division is in effect, so "/" doesn't map to __div__ etc;
            # but the canned expected-output file requires that those get called.
            x.__div__(1)
            x.__rdiv__(1)
            x.__idiv__(1)

        x // 1
        1 // x
        x //= 1

        x % 1
        1 % x
        x %= 1

        x ** 1
        1 ** x
        x **= 1

        x | 1
        1 | x
        x |= 1

        x & 1
        1 & x
        x &= 1

        x ^ 1
        1 ^ x
        x ^= 1

        x >> 1
        1 >> x
        x >>= 1

        x << 1
        1 << x
        x <<= 1

        test_self.assertEquals(output, '''\
__add__ called
__radd__ called
__iadd__ called
__sub__ called
__rsub__ called
__isub__ called
__mul__ called
__rmul__ called
__imul__ called
__div__ called
__rdiv__ called
__idiv__ called
__floordiv__ called
__rfloordiv__ called
__ifloordiv__ called
__mod__ called
__rmod__ called
__imod__ called
__pow__ called
__rpow__ called
__ipow__ called
__or__ called
__ror__ called
__ior__ called
__and__ called
__rand__ called
__iand__ called
__xor__ called
__rxor__ called
__ixor__ called
__rshift__ called
__rrshift__ called
__irshift__ called
__lshift__ called
__rlshift__ called
__ilshift__ called
'''.splitlines())

def test_main():
    with _check_py3k_warnings(("classic int division", DeprecationWarning)):
        run_unittest(AugAssignTest)

if __name__ == '__main__':
    test_main()
