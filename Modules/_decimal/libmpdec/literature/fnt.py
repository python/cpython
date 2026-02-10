#
# Copyright (c) 2008-2020 Stefan Krah. All rights reserved.
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


######################################################################
#  This file lists and checks some of the constants and limits used  #
#  in libmpdec's Number Theoretic Transform. At the end of the file  #
#  there is an example function for the plain DFT transform.         #
######################################################################


#
# Number theoretic transforms are done in subfields of F(p). P[i]
# are the primes, D[i] = P[i] - 1 are highly composite and w[i]
# are the respective primitive roots of F(p).
#
# The strategy is to convolute two coefficients modulo all three
# primes, then use the Chinese Remainder Theorem on the three
# result arrays to recover the result in the usual base RADIX
# form.
#

# ======================================================================
#                           Primitive roots
# ======================================================================

#
# Verify primitive roots:
#
# For a prime field, r is a primitive root if and only if for all prime
# factors f of p-1, r**((p-1)/f) =/= 1  (mod p).
#
def prod(F, E):
    """Check that the factorization of P-1 is correct. F is the list of
       factors of P-1, E lists the number of occurrences of each factor."""
    x = 1
    for y, z in zip(F, E):
        x *= y**z
    return x

def is_primitive_root(r, p, factors, exponents):
    """Check if r is a primitive root of F(p)."""
    if p != prod(factors, exponents) + 1:
        return False
    for f in factors:
        q, control = divmod(p-1, f)
        if control != 0:
            return False
        if pow(r, q, p) == 1:
            return False
    return True


# =================================================================
#             Constants and limits for the 64-bit version
# =================================================================

RADIX = 10**19

# Primes P1, P2 and P3:
P = [2**64-2**32+1, 2**64-2**34+1, 2**64-2**40+1]

# P-1, highly composite. The transform length d is variable and
# must divide D = P-1. Since all D are divisible by 3 * 2**32,
# transform lengths can be 2**n or 3 * 2**n (where n <= 32).
D = [2**32 * 3    * (5 * 17 * 257 * 65537),
     2**34 * 3**2 * (7 * 11 * 31 * 151 * 331),
     2**40 * 3**2 * (5 * 7 * 13 * 17 * 241)]

# Prime factors of P-1 and their exponents:
F = [(2,3,5,17,257,65537), (2,3,7,11,31,151,331), (2,3,5,7,13,17,241)]
E = [(32,1,1,1,1,1), (34,2,1,1,1,1,1), (40,2,1,1,1,1,1)]

# Maximum transform length for 2**n. Above that only 3 * 2**31
# or 3 * 2**32 are possible.
MPD_MAXTRANSFORM_2N = 2**32


# Limits in the terminology of Pollard's paper:
m2 = (MPD_MAXTRANSFORM_2N * 3) // 2 # Maximum length of the smaller array.
M1 = M2 = RADIX-1                   # Maximum value per single word.
L = m2 * M1 * M2
P[0] * P[1] * P[2] > 2 * L


# Primitive roots of F(P1), F(P2) and F(P3):
w = [7, 10, 19]

# The primitive roots are correct:
for i in range(3):
    if not is_primitive_root(w[i], P[i], F[i], E[i]):
        print("FAIL")


# =================================================================
#             Constants and limits for the 32-bit version
# =================================================================

RADIX = 10**9

# Primes P1, P2 and P3:
P = [2113929217, 2013265921, 1811939329]

# P-1, highly composite. All D = P-1 are divisible by 3 * 2**25,
# allowing for transform lengths up to 3 * 2**25 words.
D = [2**25 * 3**2 * 7,
     2**27 * 3    * 5,
     2**26 * 3**3]

# Prime factors of P-1 and their exponents:
F = [(2,3,7), (2,3,5), (2,3)]
E = [(25,2,1), (27,1,1), (26,3)]

# Maximum transform length for 2**n. Above that only 3 * 2**24 or
# 3 * 2**25 are possible.
MPD_MAXTRANSFORM_2N = 2**25


# Limits in the terminology of Pollard's paper:
m2 = (MPD_MAXTRANSFORM_2N * 3) // 2 # Maximum length of the smaller array.
M1 = M2 = RADIX-1                   # Maximum value per single word.
L = m2 * M1 * M2
P[0] * P[1] * P[2] > 2 * L


# Primitive roots of F(P1), F(P2) and F(P3):
w = [5, 31, 13]

# The primitive roots are correct:
for i in range(3):
    if not is_primitive_root(w[i], P[i], F[i], E[i]):
        print("FAIL")


# ======================================================================
#                 Example transform using a single prime
# ======================================================================

def ntt(lst, dir):
    """Perform a transform on the elements of lst. len(lst) must
       be 2**n or 3 * 2**n, where n <= 25. This is the slow DFT."""
    p = 2113929217             # prime
    d = len(lst)               # transform length
    d_prime = pow(d, (p-2), p) # inverse of d
    xi = (p-1)//d
    w = 5                         # primitive root of F(p)
    r = pow(w, xi, p)             # primitive root of the subfield
    r_prime = pow(w, (p-1-xi), p) # inverse of r
    if dir == 1:      # forward transform
        a = lst       # input array
        A = [0] * d   # transformed values
        for i in range(d):
            s = 0
            for j in range(d):
                s += a[j] * pow(r, i*j, p)
            A[i] = s % p
        return A
    elif dir == -1: # backward transform
        A = lst     # input array
        a = [0] * d # transformed values
        for j in range(d):
            s = 0
            for i in range(d):
                s += A[i] * pow(r_prime, i*j, p)
            a[j] = (d_prime * s) % p
        return a

def ntt_convolute(a, b):
    """convolute arrays a and b."""
    assert(len(a) == len(b))
    x = ntt(a, 1)
    y = ntt(b, 1)
    for i in range(len(a)):
        y[i] = y[i] * x[i]
    r = ntt(y, -1)
    return r


# Example: Two arrays representing 21 and 81 in little-endian:
a = [1, 2, 0, 0]
b = [1, 8, 0, 0]

assert(ntt_convolute(a, b) == [1,        10,        16,        0])
assert(21 * 81             == (1*10**0 + 10*10**1 + 16*10**2 + 0*10**3))
