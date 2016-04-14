"""Generate cryptographically strong pseudo-random numbers suitable for
managing secrets such as account authentication, tokens, and similar.
See PEP 506 for more information.

https://www.python.org/dev/peps/pep-0506/


Random numbers
==============

The ``secrets`` module provides the following pseudo-random functions, based
on SystemRandom, which in turn uses the most secure source of randomness your
operating system provides.


    choice(sequence)
        Choose a random element from a non-empty sequence.

    randbelow(n)
        Return a random int in the range [0, n).

    randbits(k)
        Generates an int with k random bits.

    SystemRandom
        Class for generating random numbers using sources provided by
        the operating system. See the ``random`` module for documentation.


Token functions
===============

The ``secrets`` module provides a number of functions for generating secure
tokens, suitable for applications such as password resets, hard-to-guess
URLs, and similar. All the ``token_*`` functions take an optional single
argument specifying the number of bytes of randomness to use. If that is
not given, or is ``None``, a reasonable default is used. That default is
subject to change at any time, including during maintenance releases.


    token_bytes(nbytes=None)
        Return a random byte-string containing ``nbytes`` number of bytes.

        >>> secrets.token_bytes(16)  #doctest:+SKIP
        b'\\xebr\\x17D*t\\xae\\xd4\\xe3S\\xb6\\xe2\\xebP1\\x8b'


    token_hex(nbytes=None)
        Return a random text-string, in hexadecimal. The string has ``nbytes``
        random bytes, each byte converted to two hex digits.

        >>> secrets.token_hex(16)  #doctest:+SKIP
        'f9bf78b9a18ce6d46a0cd2b0b86df9da'

    token_urlsafe(nbytes=None)
        Return a random URL-safe text-string, containing ``nbytes`` random
        bytes. On average, each byte results in approximately 1.3 characters
        in the final result.

        >>> secrets.token_urlsafe(16)  #doctest:+SKIP
        'Drmhze6EPcv0fN_81Bj-nA'


(The examples above assume Python 3. In Python 2, byte-strings will display
using regular quotes ``''`` with no prefix, and text-strings will have a
``u`` prefix.)


Other functions
===============

    compare_digest(a, b)
        Return True if strings a and b are equal, otherwise False.
        Performs the equality comparison in such a way as to reduce the
        risk of timing attacks.

        See http://codahale.com/a-lesson-in-timing-attacks/ for a
        discussion on how timing attacks against ``==`` can reveal
        secrets from your application.


"""

__all__ = ['choice', 'randbelow', 'randbits', 'SystemRandom',
           'token_bytes', 'token_hex', 'token_urlsafe',
           'compare_digest',
           ]


import base64
import binascii
import os

try:
    from hmac import compare_digest
except ImportError:
    # Python version is too old. Fall back to a pure-Python version.

    import operator
    from functools import reduce

    def compare_digest(a, b):
        """Return ``a == b`` using an approach resistant to timing analysis.

        a and b must both be of the same type: either both text strings,
        or both byte strings.

        Note: If a and b are of different lengths, or if an error occurs,
        a timing attack could theoretically reveal information about the
        types and lengths of a and b, but not their values.
        """
        # For a similar approach, see
        # http://codahale.com/a-lesson-in-timing-attacks/
        for T in (bytes, str):
            if isinstance(a, T) and isinstance(b, T):
                break
        else:  # for...else
            raise TypeError("arguments must be both strings or both bytes")
        if len(a) != len(b):
            return False
        # Thanks to Raymond Hettinger for this one-liner.
        return reduce(operator.and_, map(operator.eq, a, b), True)



from random import SystemRandom

_sysrand = SystemRandom()

randbits = _sysrand.getrandbits
choice = _sysrand.choice

def randbelow(exclusive_upper_bound):
    return _sysrand._randbelow(exclusive_upper_bound)

DEFAULT_ENTROPY = 32  # number of bytes to return by default

def token_bytes(nbytes=None):
    if nbytes is None:
        nbytes = DEFAULT_ENTROPY
    return os.urandom(nbytes)

def token_hex(nbytes=None):
    return binascii.hexlify(token_bytes(nbytes)).decode('ascii')

def token_urlsafe(nbytes=None):
    tok = token_bytes(nbytes)
    return base64.urlsafe_b64encode(tok).rstrip(b'=').decode('ascii')

