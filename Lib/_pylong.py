"""Python implementations of some algorithms for use by longobject.c.
The goal is to provide asymptotically faster algorithms that can be
used for operations on integers with many digits.  In those cases, the
performance overhead of the Python implementation is not significant
since the asymptotic behavior is what dominates runtime. Functions
provided by this module should be considered private and not part of any
public API.

Note: for ease of maintainability, please prefer clear code and avoid
"micro-optimizations".  This module will only be imported and used for
integers with a huge number of digits.  Saving a few microseconds with
tricky or non-obvious code is not worth it.  For people looking for
maximum performance, they should use something like gmpy2."""

import re
import decimal
try:
    import _decimal
except ImportError:
    _decimal = None


def int_to_decimal(n):
    """Asymptotically fast conversion of an 'int' to Decimal."""

    # Function due to Tim Peters.  See GH issue #90716 for details.
    # https://github.com/python/cpython/issues/90716
    #
    # The implementation in longobject.c of base conversion algorithms
    # between power-of-2 and non-power-of-2 bases are quadratic time.
    # This function implements a divide-and-conquer algorithm that is
    # faster for large numbers.  Builds an equal decimal.Decimal in a
    # "clever" recursive way.  If we want a string representation, we
    # apply str to _that_.

    D = decimal.Decimal
    D2 = D(2)

    BITLIM = 128

    mem = {}

    def w2pow(w):
        """Return D(2)**w and store the result. Also possibly save some
        intermediate results. In context, these are likely to be reused
        across various levels of the conversion to Decimal."""
        if (result := mem.get(w)) is None:
            if w <= BITLIM:
                result = D2**w
            elif w - 1 in mem:
                result = (t := mem[w - 1]) + t
            else:
                w2 = w >> 1
                # If w happens to be odd, w-w2 is one larger then w2
                # now. Recurse on the smaller first (w2), so that it's
                # in the cache and the larger (w-w2) can be handled by
                # the cheaper `w-1 in mem` branch instead.
                result = w2pow(w2) * w2pow(w - w2)
            mem[w] = result
        return result

    def inner(n, w):
        if w <= BITLIM:
            return D(n)
        w2 = w >> 1
        hi = n >> w2
        lo = n - (hi << w2)
        return inner(lo, w2) + inner(hi, w - w2) * w2pow(w2)

    with decimal.localcontext() as ctx:
        ctx.prec = decimal.MAX_PREC
        ctx.Emax = decimal.MAX_EMAX
        ctx.Emin = decimal.MIN_EMIN
        ctx.traps[decimal.Inexact] = 1

        if n < 0:
            negate = True
            n = -n
        else:
            negate = False
        result = inner(n, n.bit_length())
        if negate:
            result = -result
    return result


def int_to_decimal_string(n):
    """Asymptotically fast conversion of an 'int' to a decimal string."""
    w = n.bit_length()
    if w > 450_000 and _decimal is not None:
        # It is only usable with the C decimal implementation.
        # _pydecimal.py calls str() on very large integers, which in its
        # turn calls int_to_decimal_string(), causing very deep recursion.
        return str(int_to_decimal(n))

    # Fallback algorithm for the case when the C decimal module isn't
    # available.  This algorithm is asymptotically worse than the algorithm
    # using the decimal module, but better than the quadratic time
    # implementation in longobject.c.
    def inner(n, w):
        if w <= 1000:
            return str(n)
        w2 = w >> 1
        d = pow10_cache.get(w2)
        if d is None:
            d = pow10_cache[w2] = 5**w2 << w2 # 10**i = (5*2)**i = 5**i * 2**i
        hi, lo = divmod(n, d)
        return inner(hi, w - w2) + inner(lo, w2).zfill(w2)

    # The estimation of the number of decimal digits.
    # There is no harm in small error.  If we guess too large, there may
    # be leading 0's that need to be stripped.  If we guess too small, we
    # may need to call str() recursively for the remaining highest digits,
    # which can still potentially be a large integer. This is manifested
    # only if the number has way more than 10**15 digits, that exceeds
    # the 52-bit physical address limit in both Intel64 and AMD64.
    w = int(w * 0.3010299956639812 + 1)  # log10(2)
    pow10_cache = {}
    if n < 0:
        n = -n
        sign = '-'
    else:
        sign = ''
    s = inner(n, w)
    if s[0] == '0' and n:
        # If our guess of w is too large, there may be leading 0's that
        # need to be stripped.
        s = s.lstrip('0')
    return sign + s


def _str_to_int_inner(s):
    """Asymptotically fast conversion of a 'str' to an 'int'."""

    # Function due to Bjorn Martinsson.  See GH issue #90716 for details.
    # https://github.com/python/cpython/issues/90716
    #
    # The implementation in longobject.c of base conversion algorithms
    # between power-of-2 and non-power-of-2 bases are quadratic time.
    # This function implements a divide-and-conquer algorithm making use
    # of Python's built in big int multiplication. Since Python uses the
    # Karatsuba algorithm for multiplication, the time complexity
    # of this function is O(len(s)**1.58).

    DIGLIM = 2048

    mem = {}

    def w5pow(w):
        """Return 5**w and store the result.
        Also possibly save some intermediate results. In context, these
        are likely to be reused across various levels of the conversion
        to 'int'.
        """
        if (result := mem.get(w)) is None:
            if w <= DIGLIM:
                result = 5**w
            elif w - 1 in mem:
                result = mem[w - 1] * 5
            else:
                w2 = w >> 1
                # If w happens to be odd, w-w2 is one larger then w2
                # now. Recurse on the smaller first (w2), so that it's
                # in the cache and the larger (w-w2) can be handled by
                # the cheaper `w-1 in mem` branch instead.
                result = w5pow(w2) * w5pow(w - w2)
            mem[w] = result
        return result

    def inner(a, b):
        if b - a <= DIGLIM:
            return int(s[a:b])
        mid = (a + b + 1) >> 1
        return inner(mid, b) + ((inner(a, mid) * w5pow(b - mid)) << (b - mid))

    return inner(0, len(s))


def int_from_string(s):
    """Asymptotically fast version of PyLong_FromString(), conversion
    of a string of decimal digits into an 'int'."""
    # PyLong_FromString() has already removed leading +/-, checked for invalid
    # use of underscore characters, checked that string consists of only digits
    # and underscores, and stripped leading whitespace.  The input can still
    # contain underscores and have trailing whitespace.
    s = s.rstrip().replace('_', '')
    return _str_to_int_inner(s)


def str_to_int(s):
    """Asymptotically fast version of decimal string to 'int' conversion."""
    # FIXME: this doesn't support the full syntax that int() supports.
    m = re.match(r'\s*([+-]?)([0-9_]+)\s*', s)
    if not m:
        raise ValueError('invalid literal for int() with base 10')
    v = int_from_string(m.group(2))
    if m.group(1) == '-':
        v = -v
    return v


# Fast integer division, based on code from Mark Dickinson, fast_div.py
# GH-47701. Additional refinements and optimizations by Bjorn Martinsson.  The
# algorithm is due to Burnikel and Ziegler, in their paper "Fast Recursive
# Division".

_DIV_LIMIT = 4000


def _div2n1n(a, b, n):
    """Divide a 2n-bit nonnegative integer a by an n-bit positive integer
    b, using a recursive divide-and-conquer algorithm.

    Inputs:
      n is a positive integer
      b is a positive integer with exactly n bits
      a is a nonnegative integer such that a < 2**n * b

    Output:
      (q, r) such that a = b*q+r and 0 <= r < b.

    """
    if a.bit_length() - n <= _DIV_LIMIT:
        return divmod(a, b)
    pad = n & 1
    if pad:
        a <<= 1
        b <<= 1
        n += 1
    half_n = n >> 1
    mask = (1 << half_n) - 1
    b1, b2 = b >> half_n, b & mask
    q1, r = _div3n2n(a >> n, (a >> half_n) & mask, b, b1, b2, half_n)
    q2, r = _div3n2n(r, a & mask, b, b1, b2, half_n)
    if pad:
        r >>= 1
    return q1 << half_n | q2, r


def _div3n2n(a12, a3, b, b1, b2, n):
    """Helper function for _div2n1n; not intended to be called directly."""
    if a12 >> n == b1:
        q, r = (1 << n) - 1, a12 - (b1 << n) + b1
    else:
        q, r = _div2n1n(a12, b1, n)
    r = (r << n | a3) - q * b2
    while r < 0:
        q -= 1
        r += b
    return q, r


def _int2digits(a, n):
    """Decompose non-negative int a into base 2**n

    Input:
      a is a non-negative integer

    Output:
      List of the digits of a in base 2**n in little-endian order,
      meaning the most significant digit is last. The most
      significant digit is guaranteed to be non-zero.
      If a is 0 then the output is an empty list.

    """
    a_digits = [0] * ((a.bit_length() + n - 1) // n)

    def inner(x, L, R):
        if L + 1 == R:
            a_digits[L] = x
            return
        mid = (L + R) >> 1
        shift = (mid - L) * n
        upper = x >> shift
        lower = x ^ (upper << shift)
        inner(lower, L, mid)
        inner(upper, mid, R)

    if a:
        inner(a, 0, len(a_digits))
    return a_digits


def _digits2int(digits, n):
    """Combine base-2**n digits into an int. This function is the
    inverse of `_int2digits`. For more details, see _int2digits.
    """

    def inner(L, R):
        if L + 1 == R:
            return digits[L]
        mid = (L + R) >> 1
        shift = (mid - L) * n
        return (inner(mid, R) << shift) + inner(L, mid)

    return inner(0, len(digits)) if digits else 0


def _divmod_pos(a, b):
    """Divide a non-negative integer a by a positive integer b, giving
    quotient and remainder."""
    # Use grade-school algorithm in base 2**n, n = nbits(b)
    n = b.bit_length()
    a_digits = _int2digits(a, n)

    r = 0
    q_digits = []
    for a_digit in reversed(a_digits):
        q_digit, r = _div2n1n((r << n) + a_digit, b, n)
        q_digits.append(q_digit)
    q_digits.reverse()
    q = _digits2int(q_digits, n)
    return q, r


def int_divmod(a, b):
    """Asymptotically fast replacement for divmod, for 'int'.
    Its time complexity is O(n**1.58), where n = #bits(a) + #bits(b).
    """
    if b == 0:
        raise ZeroDivisionError
    elif b < 0:
        q, r = int_divmod(-a, -b)
        return q, -r
    elif a < 0:
        q, r = int_divmod(~a, b)
        return ~q, b + ~r
    else:
        return _divmod_pos(a, b)
