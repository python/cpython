#
# Copyright (c) 2008-2012 Stefan Krah. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#


# Generate test cases for deccheck.py.


#
# Grammar from http://speleotrove.com/decimal/daconvs.html
#
# sign           ::=  '+' | '-'
# digit          ::=  '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' |
#                     '8' | '9'
# indicator      ::=  'e' | 'E'
# digits         ::=  digit [digit]...
# decimal-part   ::=  digits '.' [digits] | ['.'] digits
# exponent-part  ::=  indicator [sign] digits
# infinity       ::=  'Infinity' | 'Inf'
# nan            ::=  'NaN' [digits] | 'sNaN' [digits]
# numeric-value  ::=  decimal-part [exponent-part] | infinity
# numeric-string ::=  [sign] numeric-value | [sign] nan
#


from random import randrange, sample
from fractions import Fraction
from randfloat import un_randfloat, bin_randfloat, tern_randfloat


def sign():
    if randrange(2):
        if randrange(2): return '+'
        return ''
    return '-'

def indicator():
    return "eE"[randrange(2)]

def digits(maxprec):
    if maxprec == 0: return ''
    return str(randrange(10**maxprec))

def dot():
    if randrange(2): return '.'
    return ''

def decimal_part(maxprec):
    if randrange(100) > 60: # integers
        return digits(maxprec)
    if randrange(2):
        intlen = randrange(1, maxprec+1)
        fraclen = maxprec-intlen
        intpart = digits(intlen)
        fracpart = digits(fraclen)
        return ''.join((intpart, '.', fracpart))
    else:
        return ''.join((dot(), digits(maxprec)))

def expdigits(maxexp):
    return str(randrange(maxexp))

def exponent_part(maxexp):
    return ''.join((indicator(), sign(), expdigits(maxexp)))

def infinity():
    if randrange(2): return 'Infinity'
    return 'Inf'

def nan():
    d = ''
    if randrange(2):
        d = digits(randrange(99))
    if randrange(2):
        return ''.join(('NaN', d))
    else:
        return ''.join(('sNaN', d))

def numeric_value(maxprec, maxexp):
    if randrange(100) > 90:
        return infinity()
    exp_part = ''
    if randrange(100) > 60:
        exp_part = exponent_part(maxexp)
    return ''.join((decimal_part(maxprec), exp_part))

def numeric_string(maxprec, maxexp):
    if randrange(100) > 95:
        return ''.join((sign(), nan()))
    else:
        return ''.join((sign(), numeric_value(maxprec, maxexp)))

def randdec(maxprec, maxexp):
    return numeric_string(maxprec, maxexp)

def rand_adjexp(maxprec, maxadjexp):
    d = digits(maxprec)
    maxexp = maxadjexp-len(d)+1
    if maxexp == 0: maxexp = 1
    exp = str(randrange(maxexp-2*(abs(maxexp)), maxexp))
    return ''.join((sign(), d, 'E', exp))


def ndigits(n):
    if n < 1: return 0
    return randrange(10**(n-1), 10**n)

def randtuple(maxprec, maxexp):
    n = randrange(100)
    sign = randrange(2)
    coeff = ndigits(maxprec)
    if n >= 95:
        coeff = ()
        exp = 'F'
    elif n >= 85:
        coeff = tuple(map(int, str(ndigits(maxprec))))
        exp = "nN"[randrange(2)]
    else:
        coeff = tuple(map(int, str(ndigits(maxprec))))
        exp = randrange(-maxexp, maxexp)
    return (sign, coeff, exp)

def from_triple(sign, coeff, exp):
    return ''.join((str(sign*coeff), indicator(), str(exp)))


# Close to 10**n
def un_close_to_pow10(prec, maxexp, itr=None):
    if itr is None:
        lst = range(prec+30)
    else:
        lst = sample(range(prec+30), itr)
    nines = [10**n - 1 for n in lst]
    pow10 = [10**n for n in lst]
    for coeff in nines:
        yield coeff
        yield -coeff
        yield from_triple(1, coeff, randrange(2*maxexp))
        yield from_triple(-1, coeff, randrange(2*maxexp))
    for coeff in pow10:
        yield coeff
        yield -coeff

# Close to 10**n
def bin_close_to_pow10(prec, maxexp, itr=None):
    if itr is None:
        lst = range(prec+30)
    else:
        lst = sample(range(prec+30), itr)
    nines = [10**n - 1 for n in lst]
    pow10 = [10**n for n in lst]
    for coeff in nines:
        yield coeff, 1
        yield -coeff, -1
        yield 1, coeff
        yield -1, -coeff
        yield from_triple(1, coeff, randrange(2*maxexp)), 1
        yield from_triple(-1, coeff, randrange(2*maxexp)), -1
        yield 1, from_triple(1, coeff, -randrange(2*maxexp))
        yield -1, from_triple(-1, coeff, -randrange(2*maxexp))
    for coeff in pow10:
        yield coeff, -1
        yield -coeff, 1
        yield 1, -coeff
        yield -coeff, 1

# Close to 1:
def close_to_one_greater(prec, emax, emin):
    rprec = 10**prec
    return ''.join(("1.", '0'*randrange(prec),
                   str(randrange(rprec))))

def close_to_one_less(prec, emax, emin):
    rprec = 10**prec
    return ''.join(("0.9", '9'*randrange(prec),
                   str(randrange(rprec))))

# Close to 0:
def close_to_zero_greater(prec, emax, emin):
    rprec = 10**prec
    return ''.join(("0.", '0'*randrange(prec),
                   str(randrange(rprec))))

def close_to_zero_less(prec, emax, emin):
    rprec = 10**prec
    return ''.join(("-0.", '0'*randrange(prec),
                   str(randrange(rprec))))

# Close to emax:
def close_to_emax_less(prec, emax, emin):
    rprec = 10**prec
    return ''.join(("9.", '9'*randrange(prec),
                   str(randrange(rprec)), "E", str(emax)))

def close_to_emax_greater(prec, emax, emin):
    rprec = 10**prec
    return ''.join(("1.", '0'*randrange(prec),
                   str(randrange(rprec)), "E", str(emax+1)))

# Close to emin:
def close_to_emin_greater(prec, emax, emin):
    rprec = 10**prec
    return ''.join(("1.", '0'*randrange(prec),
                   str(randrange(rprec)), "E", str(emin)))

def close_to_emin_less(prec, emax, emin):
    rprec = 10**prec
    return ''.join(("9.", '9'*randrange(prec),
                   str(randrange(rprec)), "E", str(emin-1)))

# Close to etiny:
def close_to_etiny_greater(prec, emax, emin):
    rprec = 10**prec
    etiny = emin - (prec - 1)
    return ''.join(("1.", '0'*randrange(prec),
                   str(randrange(rprec)), "E", str(etiny)))

def close_to_etiny_less(prec, emax, emin):
    rprec = 10**prec
    etiny = emin - (prec - 1)
    return ''.join(("9.", '9'*randrange(prec),
                   str(randrange(rprec)), "E", str(etiny-1)))


def close_to_min_etiny_greater(prec, max_prec, min_emin):
    rprec = 10**prec
    etiny = min_emin - (max_prec - 1)
    return ''.join(("1.", '0'*randrange(prec),
                   str(randrange(rprec)), "E", str(etiny)))

def close_to_min_etiny_less(prec, max_prec, min_emin):
    rprec = 10**prec
    etiny = min_emin - (max_prec - 1)
    return ''.join(("9.", '9'*randrange(prec),
                   str(randrange(rprec)), "E", str(etiny-1)))


close_funcs = [
  close_to_one_greater, close_to_one_less, close_to_zero_greater,
  close_to_zero_less, close_to_emax_less, close_to_emax_greater,
  close_to_emin_greater, close_to_emin_less, close_to_etiny_greater,
  close_to_etiny_less, close_to_min_etiny_greater, close_to_min_etiny_less
]


def un_close_numbers(prec, emax, emin, itr=None):
    if itr is None:
        itr = 1000
    for _ in range(itr):
        for func in close_funcs:
            yield func(prec, emax, emin)

def bin_close_numbers(prec, emax, emin, itr=None):
    if itr is None:
        itr = 1000
    for _ in range(itr):
        for func1 in close_funcs:
            for func2 in close_funcs:
                yield func1(prec, emax, emin), func2(prec, emax, emin)
        for func in close_funcs:
            yield randdec(prec, emax), func(prec, emax, emin)
            yield func(prec, emax, emin), randdec(prec, emax)

def tern_close_numbers(prec, emax, emin, itr):
    if itr is None:
        itr = 1000
    for _ in range(itr):
        for func1 in close_funcs:
            for func2 in close_funcs:
                for func3 in close_funcs:
                    yield (func1(prec, emax, emin), func2(prec, emax, emin),
                           func3(prec, emax, emin))
        for func in close_funcs:
            yield (randdec(prec, emax), func(prec, emax, emin),
                   func(prec, emax, emin))
            yield (func(prec, emax, emin), randdec(prec, emax),
                   func(prec, emax, emin))
            yield (func(prec, emax, emin), func(prec, emax, emin),
                   randdec(prec, emax))
        for func in close_funcs:
            yield (randdec(prec, emax), randdec(prec, emax),
                   func(prec, emax, emin))
            yield (randdec(prec, emax), func(prec, emax, emin),
                   randdec(prec, emax))
            yield (func(prec, emax, emin), randdec(prec, emax),
                   randdec(prec, emax))


# If itr == None, test all digit lengths up to prec + 30
def un_incr_digits(prec, maxexp, itr):
    if itr is None:
        lst = range(prec+30)
    else:
        lst = sample(range(prec+30), itr)
    for m in lst:
        yield from_triple(1, ndigits(m), 0)
        yield from_triple(-1, ndigits(m), 0)
        yield from_triple(1, ndigits(m), randrange(maxexp))
        yield from_triple(-1, ndigits(m), randrange(maxexp))

# If itr == None, test all digit lengths up to prec + 30
# Also output decimals im tuple form.
def un_incr_digits_tuple(prec, maxexp, itr):
    if itr is None:
        lst = range(prec+30)
    else:
        lst = sample(range(prec+30), itr)
    for m in lst:
        yield from_triple(1, ndigits(m), 0)
        yield from_triple(-1, ndigits(m), 0)
        yield from_triple(1, ndigits(m), randrange(maxexp))
        yield from_triple(-1, ndigits(m), randrange(maxexp))
        # test from tuple
        yield (0, tuple(map(int, str(ndigits(m)))), 0)
        yield (1, tuple(map(int, str(ndigits(m)))), 0)
        yield (0, tuple(map(int, str(ndigits(m)))), randrange(maxexp))
        yield (1, tuple(map(int, str(ndigits(m)))), randrange(maxexp))

# If itr == None, test all combinations of digit lengths up to prec + 30
def bin_incr_digits(prec, maxexp, itr):
    if itr is None:
        lst1 = range(prec+30)
        lst2 = range(prec+30)
    else:
        lst1 = sample(range(prec+30), itr)
        lst2 = sample(range(prec+30), itr)
    for m in lst1:
        x = from_triple(1, ndigits(m), 0)
        yield x, x
        x = from_triple(-1, ndigits(m), 0)
        yield x, x
        x = from_triple(1, ndigits(m), randrange(maxexp))
        yield x, x
        x = from_triple(-1, ndigits(m), randrange(maxexp))
        yield x, x
    for m in lst1:
        for n in lst2:
            x = from_triple(1, ndigits(m), 0)
            y = from_triple(1, ndigits(n), 0)
            yield x, y
            x = from_triple(-1, ndigits(m), 0)
            y = from_triple(1, ndigits(n), 0)
            yield x, y
            x = from_triple(1, ndigits(m), 0)
            y = from_triple(-1, ndigits(n), 0)
            yield x, y
            x = from_triple(-1, ndigits(m), 0)
            y = from_triple(-1, ndigits(n), 0)
            yield x, y
            x = from_triple(1, ndigits(m), randrange(maxexp))
            y = from_triple(1, ndigits(n), randrange(maxexp))
            yield x, y
            x = from_triple(-1, ndigits(m), randrange(maxexp))
            y = from_triple(1, ndigits(n), randrange(maxexp))
            yield x, y
            x = from_triple(1, ndigits(m), randrange(maxexp))
            y = from_triple(-1, ndigits(n), randrange(maxexp))
            yield x, y
            x = from_triple(-1, ndigits(m), randrange(maxexp))
            y = from_triple(-1, ndigits(n), randrange(maxexp))
            yield x, y


def randsign():
    return (1, -1)[randrange(2)]

# If itr == None, test all combinations of digit lengths up to prec + 30
def tern_incr_digits(prec, maxexp, itr):
    if itr is None:
        lst1 = range(prec+30)
        lst2 = range(prec+30)
        lst3 = range(prec+30)
    else:
        lst1 = sample(range(prec+30), itr)
        lst2 = sample(range(prec+30), itr)
        lst3 = sample(range(prec+30), itr)
    for m in lst1:
        for n in lst2:
            for p in lst3:
                x = from_triple(randsign(), ndigits(m), 0)
                y = from_triple(randsign(), ndigits(n), 0)
                z = from_triple(randsign(), ndigits(p), 0)
                yield x, y, z


# Tests for the 'logical' functions
def bindigits(prec):
    z = 0
    for i in range(prec):
        z += randrange(2) * 10**i
    return z

def logical_un_incr_digits(prec, itr):
    if itr is None:
        lst = range(prec+30)
    else:
        lst = sample(range(prec+30), itr)
    for m in lst:
        yield from_triple(1, bindigits(m), 0)

def logical_bin_incr_digits(prec, itr):
    if itr is None:
        lst1 = range(prec+30)
        lst2 = range(prec+30)
    else:
        lst1 = sample(range(prec+30), itr)
        lst2 = sample(range(prec+30), itr)
    for m in lst1:
        x = from_triple(1, bindigits(m), 0)
        yield x, x
    for m in lst1:
        for n in lst2:
            x = from_triple(1, bindigits(m), 0)
            y = from_triple(1, bindigits(n), 0)
            yield x, y


def randint():
    p = randrange(1, 100)
    return ndigits(p) * (1,-1)[randrange(2)]

def randfloat():
    p = randrange(1, 100)
    s = numeric_value(p, 383)
    try:
        f = float(numeric_value(p, 383))
    except ValueError:
        f = 0.0
    return f

def randcomplex():
    real = randfloat()
    if randrange(100) > 30:
        imag = 0.0
    else:
        imag = randfloat()
    return complex(real, imag)

def randfraction():
    num = randint()
    denom = randint()
    if denom == 0:
        denom = 1
    return Fraction(num, denom)

number_funcs = [randint, randfloat, randcomplex, randfraction]

def un_random_mixed_op(itr=None):
    if itr is None:
        itr = 1000
    for _ in range(itr):
        for func in number_funcs:
            yield func()
    # Test garbage input
    for x in (['x'], ('y',), {'z'}, {1:'z'}):
        yield x

def bin_random_mixed_op(prec, emax, emin, itr=None):
    if itr is None:
        itr = 1000
    for _ in range(itr):
        for func in number_funcs:
            yield randdec(prec, emax), func()
            yield func(), randdec(prec, emax)
        for number in number_funcs:
            for dec in close_funcs:
                yield dec(prec, emax, emin), number()
    # Test garbage input
    for x in (['x'], ('y',), {'z'}, {1:'z'}):
        for y in (['x'], ('y',), {'z'}, {1:'z'}):
            yield x, y

def tern_random_mixed_op(prec, emax, emin, itr):
    if itr is None:
        itr = 1000
    for _ in range(itr):
        for func in number_funcs:
            yield randdec(prec, emax), randdec(prec, emax), func()
            yield randdec(prec, emax), func(), func()
            yield func(), func(), func()
    # Test garbage input
    for x in (['x'], ('y',), {'z'}, {1:'z'}):
        for y in (['x'], ('y',), {'z'}, {1:'z'}):
            for z in (['x'], ('y',), {'z'}, {1:'z'}):
                yield x, y, z

def all_unary(prec, exp_range, itr):
    for a in un_close_to_pow10(prec, exp_range, itr):
        yield (a,)
    for a in un_close_numbers(prec, exp_range, -exp_range, itr):
        yield (a,)
    for a in un_incr_digits_tuple(prec, exp_range, itr):
        yield (a,)
    for a in un_randfloat():
        yield (a,)
    for a in un_random_mixed_op(itr):
        yield (a,)
    for a in logical_un_incr_digits(prec, itr):
        yield (a,)
    for _ in range(100):
        yield (randdec(prec, exp_range),)
    for _ in range(100):
        yield (randtuple(prec, exp_range),)

def unary_optarg(prec, exp_range, itr):
    for _ in range(100):
        yield randdec(prec, exp_range), None
        yield randdec(prec, exp_range), None, None

def all_binary(prec, exp_range, itr):
    for a, b in bin_close_to_pow10(prec, exp_range, itr):
        yield a, b
    for a, b in bin_close_numbers(prec, exp_range, -exp_range, itr):
        yield a, b
    for a, b in bin_incr_digits(prec, exp_range, itr):
        yield a, b
    for a, b in bin_randfloat():
        yield a, b
    for a, b in bin_random_mixed_op(prec, exp_range, -exp_range, itr):
        yield a, b
    for a, b in logical_bin_incr_digits(prec, itr):
        yield a, b
    for _ in range(100):
        yield randdec(prec, exp_range), randdec(prec, exp_range)

def binary_optarg(prec, exp_range, itr):
    for _ in range(100):
        yield randdec(prec, exp_range), randdec(prec, exp_range), None
        yield randdec(prec, exp_range), randdec(prec, exp_range), None, None

def all_ternary(prec, exp_range, itr):
    for a, b, c in tern_close_numbers(prec, exp_range, -exp_range, itr):
        yield a, b, c
    for a, b, c in tern_incr_digits(prec, exp_range, itr):
        yield a, b, c
    for a, b, c in tern_randfloat():
        yield a, b, c
    for a, b, c in tern_random_mixed_op(prec, exp_range, -exp_range, itr):
        yield a, b, c
    for _ in range(100):
        a = randdec(prec, 2*exp_range)
        b = randdec(prec, 2*exp_range)
        c = randdec(prec, 2*exp_range)
        yield a, b, c

def ternary_optarg(prec, exp_range, itr):
    for _ in range(100):
        a = randdec(prec, 2*exp_range)
        b = randdec(prec, 2*exp_range)
        c = randdec(prec, 2*exp_range)
        yield a, b, c, None
        yield a, b, c, None, None
