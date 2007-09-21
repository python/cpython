'''\
This module implements rational numbers.

The entry point of this module is the function
        rat(numerator, denominator)
If either numerator or denominator is of an integral or rational type,
the result is a rational number, else, the result is the simplest of
the types float and complex which can hold numerator/denominator.
If denominator is omitted, it defaults to 1.
Rational numbers can be used in calculations with any other numeric
type.  The result of the calculation will be rational if possible.

There is also a test function with calling sequence
        test()
The documentation string of the test function contains the expected
output.
'''

# Contributed by Sjoerd Mullender

from types import *

def gcd(a, b):
    '''Calculate the Greatest Common Divisor.'''
    while b:
        a, b = b, a%b
    return a

def rat(num, den = 1):
    # must check complex before float
    if isinstance(num, complex) or isinstance(den, complex):
        # numerator or denominator is complex: return a complex
        return complex(num) / complex(den)
    if isinstance(num, float) or isinstance(den, float):
        # numerator or denominator is float: return a float
        return float(num) / float(den)
    # otherwise return a rational
    return Rat(num, den)

class Rat:
    '''This class implements rational numbers.'''

    def __init__(self, num, den = 1):
        if den == 0:
            raise ZeroDivisionError('rat(x, 0)')

        # normalize

        # must check complex before float
        if (isinstance(num, complex) or
            isinstance(den, complex)):
            # numerator or denominator is complex:
            # normalized form has denominator == 1+0j
            self.__num = complex(num) / complex(den)
            self.__den = complex(1)
            return
        if isinstance(num, float) or isinstance(den, float):
            # numerator or denominator is float:
            # normalized form has denominator == 1.0
            self.__num = float(num) / float(den)
            self.__den = 1.0
            return
        if (isinstance(num, self.__class__) or
            isinstance(den, self.__class__)):
            # numerator or denominator is rational
            new = num / den
            if not isinstance(new, self.__class__):
                self.__num = new
                if isinstance(new, complex):
                    self.__den = complex(1)
                else:
                    self.__den = 1.0
            else:
                self.__num = new.__num
                self.__den = new.__den
        else:
            # make sure numerator and denominator don't
            # have common factors
            # this also makes sure that denominator > 0
            g = gcd(num, den)
            self.__num = num / g
            self.__den = den / g
        # try making numerator and denominator of IntType if they fit
        try:
            numi = int(self.__num)
            deni = int(self.__den)
        except (OverflowError, TypeError):
            pass
        else:
            if self.__num == numi and self.__den == deni:
                self.__num = numi
                self.__den = deni

    def __repr__(self):
        return 'Rat(%s,%s)' % (self.__num, self.__den)

    def __str__(self):
        if self.__den == 1:
            return str(self.__num)
        else:
            return '(%s/%s)' % (str(self.__num), str(self.__den))

    # a + b
    def __add__(a, b):
        try:
            return rat(a.__num * b.__den + b.__num * a.__den,
                       a.__den * b.__den)
        except OverflowError:
            return rat(int(a.__num) * int(b.__den) +
                       int(b.__num) * int(a.__den),
                       int(a.__den) * int(b.__den))

    def __radd__(b, a):
        return Rat(a) + b

    # a - b
    def __sub__(a, b):
        try:
            return rat(a.__num * b.__den - b.__num * a.__den,
                       a.__den * b.__den)
        except OverflowError:
            return rat(int(a.__num) * int(b.__den) -
                       int(b.__num) * int(a.__den),
                       int(a.__den) * int(b.__den))

    def __rsub__(b, a):
        return Rat(a) - b

    # a * b
    def __mul__(a, b):
        try:
            return rat(a.__num * b.__num, a.__den * b.__den)
        except OverflowError:
            return rat(int(a.__num) * int(b.__num),
                       int(a.__den) * int(b.__den))

    def __rmul__(b, a):
        return Rat(a) * b

    # a / b
    def __div__(a, b):
        try:
            return rat(a.__num * b.__den, a.__den * b.__num)
        except OverflowError:
            return rat(int(a.__num) * int(b.__den),
                       int(a.__den) * int(b.__num))

    def __rdiv__(b, a):
        return Rat(a) / b

    # a % b
    def __mod__(a, b):
        div = a / b
        try:
            div = int(div)
        except OverflowError:
            div = int(div)
        return a - b * div

    def __rmod__(b, a):
        return Rat(a) % b

    # a ** b
    def __pow__(a, b):
        if b.__den != 1:
            if isinstance(a.__num, complex):
                a = complex(a)
            else:
                a = float(a)
            if isinstance(b.__num, complex):
                b = complex(b)
            else:
                b = float(b)
            return a ** b
        try:
            return rat(a.__num ** b.__num, a.__den ** b.__num)
        except OverflowError:
            return rat(int(a.__num) ** b.__num,
                       int(a.__den) ** b.__num)

    def __rpow__(b, a):
        return Rat(a) ** b

    # -a
    def __neg__(a):
        try:
            return rat(-a.__num, a.__den)
        except OverflowError:
            # a.__num == sys.maxint
            return rat(-int(a.__num), a.__den)

    # abs(a)
    def __abs__(a):
        return rat(abs(a.__num), a.__den)

    # int(a)
    def __int__(a):
        return int(a.__num / a.__den)

    # long(a)
    def __long__(a):
        return int(a.__num) / int(a.__den)

    # float(a)
    def __float__(a):
        return float(a.__num) / float(a.__den)

    # complex(a)
    def __complex__(a):
        return complex(a.__num) / complex(a.__den)

    # cmp(a,b)
    def __cmp__(a, b):
        diff = Rat(a - b)
        if diff.__num < 0:
            return -1
        elif diff.__num > 0:
            return 1
        else:
            return 0

    def __rcmp__(b, a):
        return cmp(Rat(a), b)

    # a != 0
    def __bool__(a):
        return a.__num != 0

def test():
    '''\
    Test function for rat module.

    The expected output is (module some differences in floating
    precission):
    -1
    -1
    0 0L 0.1 (0.1+0j)
    [Rat(1,2), Rat(-3,10), Rat(1,25), Rat(1,4)]
    [Rat(-3,10), Rat(1,25), Rat(1,4), Rat(1,2)]
    0
    (11/10)
    (11/10)
    1.1
    OK
    2 1.5 (3/2) (1.5+1.5j) (15707963/5000000)
    2 2 2.0 (2+0j)

    4 0 4 1 4 0
    3.5 0.5 3.0 1.33333333333 2.82842712475 1
    (7/2) (1/2) 3 (4/3) 2.82842712475 1
    (3.5+1.5j) (0.5-1.5j) (3+3j) (0.666666666667-0.666666666667j) (1.43248815986+2.43884761145j) 1
    1.5 1 1.5 (1.5+0j)

    3.5 -0.5 3.0 0.75 2.25 -1
    3.0 0.0 2.25 1.0 1.83711730709 0
    3.0 0.0 2.25 1.0 1.83711730709 1
    (3+1.5j) -1.5j (2.25+2.25j) (0.5-0.5j) (1.50768393746+1.04970907623j) -1
    (3/2) 1 1.5 (1.5+0j)

    (7/2) (-1/2) 3 (3/4) (9/4) -1
    3.0 0.0 2.25 1.0 1.83711730709 -1
    3 0 (9/4) 1 1.83711730709 0
    (3+1.5j) -1.5j (2.25+2.25j) (0.5-0.5j) (1.50768393746+1.04970907623j) -1
    (1.5+1.5j) (1.5+1.5j)

    (3.5+1.5j) (-0.5+1.5j) (3+3j) (0.75+0.75j) 4.5j -1
    (3+1.5j) 1.5j (2.25+2.25j) (1+1j) (1.18235814075+2.85446505899j) 1
    (3+1.5j) 1.5j (2.25+2.25j) (1+1j) (1.18235814075+2.85446505899j) 1
    (3+3j) 0j 4.5j (1+0j) (-0.638110484918+0.705394566962j) 0
    '''
    print(rat(-1, 1))
    print(rat(1, -1))
    a = rat(1, 10)
    print(int(a), int(a), float(a), complex(a))
    b = rat(2, 5)
    l = [a+b, a-b, a*b, a/b]
    print(l)
    l.sort()
    print(l)
    print(rat(0, 1))
    print(a+1)
    print(a+1)
    print(a+1.0)
    try:
        print(rat(1, 0))
        raise SystemError('should have been ZeroDivisionError')
    except ZeroDivisionError:
        print('OK')
    print(rat(2), rat(1.5), rat(3, 2), rat(1.5+1.5j), rat(31415926,10000000))
    list = [2, 1.5, rat(3,2), 1.5+1.5j]
    for i in list:
        print(i, end=' ')
        if not isinstance(i, complex):
            print(int(i), float(i), end=' ')
        print(complex(i))
        print()
        for j in list:
            print(i + j, i - j, i * j, i / j, i ** j, end=' ')
            if not (isinstance(i, complex) or
                    isinstance(j, complex)):
                print(cmp(i, j))
            print()


if __name__ == '__main__':
    test()
