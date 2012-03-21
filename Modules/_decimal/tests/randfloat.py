# Copyright (c) 2010 Python Software Foundation. All Rights Reserved.
# Adapted from Python's Lib/test/test_strtod.py (by Mark Dickinson)

# More test cases for deccheck.py.

import random

TEST_SIZE = 2


def test_short_halfway_cases():
    # exact halfway cases with a small number of significant digits
    for k in 0, 5, 10, 15, 20:
        # upper = smallest integer >= 2**54/5**k
        upper = -(-2**54//5**k)
        # lower = smallest odd number >= 2**53/5**k
        lower = -(-2**53//5**k)
        if lower % 2 == 0:
            lower += 1
        for i in range(10 * TEST_SIZE):
            # Select a random odd n in [2**53/5**k,
            # 2**54/5**k). Then n * 10**k gives a halfway case
            # with small number of significant digits.
            n, e = random.randrange(lower, upper, 2), k

            # Remove any additional powers of 5.
            while n % 5 == 0:
                n, e = n // 5, e + 1
            assert n % 10 in (1, 3, 7, 9)

            # Try numbers of the form n * 2**p2 * 10**e, p2 >= 0,
            # until n * 2**p2 has more than 20 significant digits.
            digits, exponent = n, e
            while digits < 10**20:
                s = '{}e{}'.format(digits, exponent)
                yield s
                # Same again, but with extra trailing zeros.
                s = '{}e{}'.format(digits * 10**40, exponent - 40)
                yield s
                digits *= 2

            # Try numbers of the form n * 5**p2 * 10**(e - p5), p5
            # >= 0, with n * 5**p5 < 10**20.
            digits, exponent = n, e
            while digits < 10**20:
                s = '{}e{}'.format(digits, exponent)
                yield s
                # Same again, but with extra trailing zeros.
                s = '{}e{}'.format(digits * 10**40, exponent - 40)
                yield s
                digits *= 5
                exponent -= 1

def test_halfway_cases():
    # test halfway cases for the round-half-to-even rule
    for i in range(1000):
        for j in range(TEST_SIZE):
            # bit pattern for a random finite positive (or +0.0) float
            bits = random.randrange(2047*2**52)

            # convert bit pattern to a number of the form m * 2**e
            e, m = divmod(bits, 2**52)
            if e:
                m, e = m + 2**52, e - 1
            e -= 1074

            # add 0.5 ulps
            m, e = 2*m + 1, e - 1

            # convert to a decimal string
            if e >= 0:
                digits = m << e
                exponent = 0
            else:
                # m * 2**e = (m * 5**-e) * 10**e
                digits = m * 5**-e
                exponent = e
            s = '{}e{}'.format(digits, exponent)
            yield s

def test_boundaries():
    # boundaries expressed as triples (n, e, u), where
    # n*10**e is an approximation to the boundary value and
    # u*10**e is 1ulp
    boundaries = [
        (10000000000000000000, -19, 1110),   # a power of 2 boundary (1.0)
        (17976931348623159077, 289, 1995),   # overflow boundary (2.**1024)
        (22250738585072013831, -327, 4941),  # normal/subnormal (2.**-1022)
        (0, -327, 4941),                     # zero
        ]
    for n, e, u in boundaries:
        for j in range(1000):
            for i in range(TEST_SIZE):
                digits = n + random.randrange(-3*u, 3*u)
                exponent = e
                s = '{}e{}'.format(digits, exponent)
                yield s
            n *= 10
            u *= 10
            e -= 1

def test_underflow_boundary():
    # test values close to 2**-1075, the underflow boundary; similar
    # to boundary_tests, except that the random error doesn't scale
    # with n
    for exponent in range(-400, -320):
        base = 10**-exponent // 2**1075
        for j in range(TEST_SIZE):
            digits = base + random.randrange(-1000, 1000)
            s = '{}e{}'.format(digits, exponent)
            yield s

def test_bigcomp():
    for ndigs in 5, 10, 14, 15, 16, 17, 18, 19, 20, 40, 41, 50:
        dig10 = 10**ndigs
        for i in range(100 * TEST_SIZE):
            digits = random.randrange(dig10)
            exponent = random.randrange(-400, 400)
            s = '{}e{}'.format(digits, exponent)
            yield s

def test_parsing():
    # make '0' more likely to be chosen than other digits
    digits = '000000123456789'
    signs = ('+', '-', '')

    # put together random short valid strings
    # \d*[.\d*]?e
    for i in range(1000):
        for j in range(TEST_SIZE):
            s = random.choice(signs)
            intpart_len = random.randrange(5)
            s += ''.join(random.choice(digits) for _ in range(intpart_len))
            if random.choice([True, False]):
                s += '.'
                fracpart_len = random.randrange(5)
                s += ''.join(random.choice(digits)
                             for _ in range(fracpart_len))
            else:
                fracpart_len = 0
            if random.choice([True, False]):
                s += random.choice(['e', 'E'])
                s += random.choice(signs)
                exponent_len = random.randrange(1, 4)
                s += ''.join(random.choice(digits)
                             for _ in range(exponent_len))

            if intpart_len + fracpart_len:
                yield s

test_particular = [
     # squares
    '1.00000000100000000025',
    '1.0000000000000000000000000100000000000000000000000' #...
    '00025',
    '1.0000000000000000000000000000000000000000000010000' #...
    '0000000000000000000000000000000000000000025',
    '1.0000000000000000000000000000000000000000000000000' #...
    '000001000000000000000000000000000000000000000000000' #...
    '000000000025',
    '0.99999999900000000025',
    '0.9999999999999999999999999999999999999999999999999' #...
    '999000000000000000000000000000000000000000000000000' #...
    '000025',
    '0.9999999999999999999999999999999999999999999999999' #...
    '999999999999999999999999999999999999999999999999999' #...
    '999999999999999999999999999999999999999990000000000' #...
    '000000000000000000000000000000000000000000000000000' #...
    '000000000000000000000000000000000000000000000000000' #...
    '0000000000000000000000000000025',

    '1.0000000000000000000000000000000000000000000000000' #...
    '000000000000000000000000000000000000000000000000000' #...
    '100000000000000000000000000000000000000000000000000' #...
    '000000000000000000000000000000000000000000000000001',
    '1.0000000000000000000000000000000000000000000000000' #...
    '000000000000000000000000000000000000000000000000000' #...
    '500000000000000000000000000000000000000000000000000' #...
    '000000000000000000000000000000000000000000000000005',
    '1.0000000000000000000000000000000000000000000000000' #...
    '000000000100000000000000000000000000000000000000000' #...
    '000000000000000000250000000000000002000000000000000' #...
    '000000000000000000000000000000000000000000010000000' #...
    '000000000000000000000000000000000000000000000000000' #...
    '0000000000000000001',
    '1.0000000000000000000000000000000000000000000000000' #...
    '000000000100000000000000000000000000000000000000000' #...
    '000000000000000000249999999999999999999999999999999' #...
    '999999999999979999999999999999999999999999999999999' #...
    '999999999999999999999900000000000000000000000000000' #...
    '000000000000000000000000000000000000000000000000000' #...
    '00000000000000000000000001',

    '0.9999999999999999999999999999999999999999999999999' #...
    '999999999900000000000000000000000000000000000000000' #...
    '000000000000000000249999999999999998000000000000000' #...
    '000000000000000000000000000000000000000000010000000' #...
    '000000000000000000000000000000000000000000000000000' #...
    '0000000000000000001',
    '0.9999999999999999999999999999999999999999999999999' #...
    '999999999900000000000000000000000000000000000000000' #...
    '000000000000000000250000001999999999999999999999999' #...
    '999999999999999999999999999999999990000000000000000' #...
    '000000000000000000000000000000000000000000000000000' #...
    '1',

    # tough cases for ln etc.
    '1.000000000000000000000000000000000000000000000000' #...
    '00000000000000000000000000000000000000000000000000' #...
    '00100000000000000000000000000000000000000000000000' #...
    '00000000000000000000000000000000000000000000000000' #...
    '0001',
    '0.999999999999999999999999999999999999999999999999' #...
    '99999999999999999999999999999999999999999999999999' #...
    '99899999999999999999999999999999999999999999999999' #...
    '99999999999999999999999999999999999999999999999999' #...
    '99999999999999999999999999999999999999999999999999' #...
    '9999'
    ]


TESTCASES = [
      [x for x in test_short_halfway_cases()],
      [x for x in test_halfway_cases()],
      [x for x in test_boundaries()],
      [x for x in test_underflow_boundary()],
      [x for x in test_bigcomp()],
      [x for x in test_parsing()],
      test_particular
]

def un_randfloat():
    for i in range(1000):
        l = random.choice(TESTCASES[:6])
        yield random.choice(l)
    for v in test_particular:
        yield v

def bin_randfloat():
    for i in range(1000):
        l1 = random.choice(TESTCASES)
        l2 = random.choice(TESTCASES)
        yield random.choice(l1), random.choice(l2)

def tern_randfloat():
    for i in range(1000):
        l1 = random.choice(TESTCASES)
        l2 = random.choice(TESTCASES)
        l3 = random.choice(TESTCASES)
        yield random.choice(l1), random.choice(l2), random.choice(l3)
