# Copyright (c) 2010 Russell Dias
# Licensed under the MIT licence.
# http://www.inversezen.com
#
# This is an implementation of the RSA public key
# encryption written in Python by Russell Dias

__author__ = 'Russell Dias // inversezen.com'
# Py3k port done by Senthil (senthil@uthcode.com)
__date__ = '05/12/2010'
__version__ = '0.0.1'

import random
from math import log

def gcd(u, v):
    """ The Greatest Common Divisor, returns
        the largest positive integer that divides
        u with v without a remainder.
    """
    while v:
        u, v = u, u % v
    return u

def eec(u, v):
    """ The Extended Eculidean Algorithm
        For u and v this algorithm finds (u1, u2, u3)
        such that uu1 + vu2 = u3 = gcd(u, v)

        We also use auxiliary vectors (v1, v2, v3) and
        (tmp1, tmp2, tmp3)
    """
    (u1, u2, u3) = (1, 0, u)
    (v1, v2, v3) = (0, 1, v)
    while (v3 != 0):
        quotient = u3 // v3
        tmp1 = u1 - quotient * v1
        tmp2 = u2 - quotient * v2
        tmp3 = u3 - quotient * v3
        (u1, u2, u3) = (v1, v2, v3)
        (v1, v2, v3) = (tmp1, tmp2, tmp3)
    return u3, u1, u2

def stringEncode(string):
    """ Brandon Sterne's algorithm to convert
        string to long
    """
    message = 0
    messageCount = len(string) - 1

    for letter in string:
        message += (256**messageCount) * ord(letter)
        messageCount -= 1
    return message

def stringDecode(number):
    """ Convert long back to string
    """

    letters = []
    text = ''
    integer = int(log(number, 256))

    while(integer >= 0):
        letter = number // (256**integer)
        letters.append(chr(letter))
        number -= letter * (256**integer)
        integer -= 1
    for char in letters:
        text += char

    return text

def split_to_odd(n):
    """ Return values 2 ^ k, such that 2^k*q = n;
        or an odd integer to test for primiality
        Let n be an odd prime. Then n-1 is even,
        where k is a positive integer.
    """
    k = 0
    while (n > 0) and (n % 2 == 0):
        k += 1
        n >>= 1
    return (k, n)

def prime(a, q, k, n):
    if pow(a, q, n) == 1:
        return True
    elif (n - 1) in [pow(a, q*(2**j), n) for j in range(k)]:
        return True
    else:
        return False

def miller_rabin(n, trials):
    """
        There is still a small chance that n will return a
        false positive. To reduce risk, it is recommended to use
        more trials.
    """
    # 2^k * q = n - 1; q is an odd int
    (k, q) = split_to_odd(n - 1)

    for trial in range(trials):
        a = random.randint(2, n-1)
        if not prime(a, q, k, n):
            return False
    return True

def get_prime(k):
    """ Generate prime of size k bits, with 50 tests
        to ensure accuracy.
    """
    prime = 0
    while (prime == 0):
        prime = random.randrange(pow(2,k//2-1) + 1, pow(2, k//2), 2)
        if not miller_rabin(prime, 50):
            prime = 0
    return prime

def modular_inverse(a, m):
    """ To calculate the decryption exponent such that
        (d * e) mod phi(N) = 1 OR g == 1 in our implementation.
        Where m is Phi(n) (PHI = (p-1) * (q-1) )

        s % m or d (decryption exponent) is the multiplicative inverse of
        the encryption exponent e.
    """
    g, s, t = eec(a, m)
    if g == 1:
        return s % m
    else:
        return None

def key_gen(bits):
    """ The public encryption exponent e,
        can be an artibrary prime number.

        Obviously, the higher the number,
        the more secure the key pairs are.
    """
    e = 17
    p = get_prime(bits)
    q = get_prime(bits)
    d = modular_inverse(e, (p-1)*(q-1))
    return p*q,d,e

def write_to_file(e, d, n):
    """ Write our public and private keys to file
    """
    public = open("publicKey", "w")
    public.write(str(e))
    public.write("\n")
    public.write(str(n))
    public.close()

    private = open("privateKey", "w")
    private.write(str(d))
    private.write("\n")
    private.write(str(n))
    private.close()


if __name__ == '__main__':
    bits = input("Enter the size of your key pairs, in bits: ")

    n, d, e = key_gen(int(bits))

    #Write keys to file
    write_to_file(e, d, n)

    print("Your keys pairs have been saved to file")

    m = input("Enter the message you would like to encrypt: ")

    m = stringEncode(m)
    encrypted = pow(m, e, n)

    print("Your encrypted message is: %s" % encrypted)
    decrypted = pow(encrypted, d, n)
    message = stringDecode(decrypted)
    print("You message decrypted is: %s" % message)
