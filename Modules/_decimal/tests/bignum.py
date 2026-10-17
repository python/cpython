#
# These tests require gmpy and test the limits of the 32-bit build. The
# limits of the 64-bit build are so large that they cannot be tested
# on accessible hardware.
#

import sys
from decimal import *
from gmpy import mpz


_PyHASH_MODULUS = sys.hash_info.modulus
# hash values to use for positive and negative infinities, and nans
_PyHASH_INF = sys.hash_info.inf
_PyHASH_NAN = sys.hash_info.nan

# _PyHASH_10INV is the inverse of 10 modulo the prime _PyHASH_MODULUS
_PyHASH_10INV = pow(10, _PyHASH_MODULUS - 2, _PyHASH_MODULUS)

def xhash(coeff, exp):
    sign = 1
    if coeff < 0:
        sign = -1
        coeff = -coeff
    if exp >= 0:
        exp_hash = pow(10, exp, _PyHASH_MODULUS)
    else:
        exp_hash = pow(_PyHASH_10INV, -exp, _PyHASH_MODULUS)
    hash_ = coeff * exp_hash % _PyHASH_MODULUS
    ans = hash_ if sign == 1 else -hash_
    return -2 if ans == -1 else ans


x = mpz(10) ** 425000000 - 1
coeff = int(x)

d = Decimal('9' * 425000000 + 'e-849999999')

h1 = xhash(coeff, -849999999)
h2 = hash(d)

assert h2 == h1
