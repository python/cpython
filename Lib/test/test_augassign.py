# Augmented assignment test.

x = 2
x += 1
x *= 2
x **= 2
x -= 8
x //= 2
x //= 1
x %= 12
x &= 2
x |= 5
x ^= 1

print x

x = [2]
x[0] += 1
x[0] *= 2
x[0] **= 2
x[0] -= 8
x[0] //= 2
x[0] //= 2
x[0] %= 12
x[0] &= 2
x[0] |= 5
x[0] ^= 1

print x

x = {0: 2}
x[0] += 1
x[0] *= 2
x[0] **= 2
x[0] -= 8
x[0] //= 2
x[0] //= 1
x[0] %= 12
x[0] &= 2
x[0] |= 5
x[0] ^= 1

print x[0]

x = [1,2]
x += [3,4]
x *= 2

print x

x = [1, 2, 3]
y = x
x[1:2] *= 2
y[1:2] += [1]

print x
print x is y

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

print isinstance(x, aug_test)
print y is not x
print x.val

x = aug_test2(2)
y = x
x += 10

print y is x
print x.val

x = aug_test3(3)
y = x
x += 10

print isinstance(x, aug_test3)
print y is not x
print x.val

class testall:

    def __add__(self, val):
        print "__add__ called"
    def __radd__(self, val):
        print "__radd__ called"
    def __iadd__(self, val):
        print "__iadd__ called"
        return self

    def __sub__(self, val):
        print "__sub__ called"
    def __rsub__(self, val):
        print "__rsub__ called"
    def __isub__(self, val):
        print "__isub__ called"
        return self

    def __mul__(self, val):
        print "__mul__ called"
    def __rmul__(self, val):
        print "__rmul__ called"
    def __imul__(self, val):
        print "__imul__ called"
        return self

    def __div__(self, val):
        print "__div__ called"
    def __rdiv__(self, val):
        print "__rdiv__ called"
    def __idiv__(self, val):
        print "__idiv__ called"
        return self

    def __floordiv__(self, val):
        print "__floordiv__ called"
        return self
    def __ifloordiv__(self, val):
        print "__ifloordiv__ called"
        return self
    def __rfloordiv__(self, val):
        print "__rfloordiv__ called"
        return self

    def __truediv__(self, val):
        print "__truediv__ called"
        return self
    def __itruediv__(self, val):
        print "__itruediv__ called"
        return self

    def __mod__(self, val):
        print "__mod__ called"
    def __rmod__(self, val):
        print "__rmod__ called"
    def __imod__(self, val):
        print "__imod__ called"
        return self

    def __pow__(self, val):
        print "__pow__ called"
    def __rpow__(self, val):
        print "__rpow__ called"
    def __ipow__(self, val):
        print "__ipow__ called"
        return self

    def __or__(self, val):
        print "__or__ called"
    def __ror__(self, val):
        print "__ror__ called"
    def __ior__(self, val):
        print "__ior__ called"
        return self

    def __and__(self, val):
        print "__and__ called"
    def __rand__(self, val):
        print "__rand__ called"
    def __iand__(self, val):
        print "__iand__ called"
        return self

    def __xor__(self, val):
        print "__xor__ called"
    def __rxor__(self, val):
        print "__rxor__ called"
    def __ixor__(self, val):
        print "__ixor__ called"
        return self

    def __rshift__(self, val):
        print "__rshift__ called"
    def __rrshift__(self, val):
        print "__rrshift__ called"
    def __irshift__(self, val):
        print "__irshift__ called"
        return self

    def __lshift__(self, val):
        print "__lshift__ called"
    def __rlshift__(self, val):
        print "__rlshift__ called"
    def __ilshift__(self, val):
        print "__ilshift__ called"
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
