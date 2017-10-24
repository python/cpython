"""Wrapper to the POSIX crypt library call and associated functionality."""

import _crypt
import string as _string
from random import SystemRandom as _SystemRandom
from collections import namedtuple as _namedtuple


_saltchars = _string.ascii_letters + _string.digits + './'
_sr = _SystemRandom()


class _Method(_namedtuple('_Method', 'name ident salt_chars total_size')):

    """Class representing a salt method per the Modular Crypt Format or the
    legacy 2-character crypt method."""

    def __repr__(self):
        return '<crypt.METHOD_{}>'.format(self.name)


def mksalt(method=None, *, log_rounds=12):
    """Generate a salt for the specified method.

    If not specified, the strongest available method will be used.

    """
    if method is None:
        method = methods[0]
    if not method.ident:
        s = ''
    elif method.ident[0] == '2':
        s = f'${method.ident}${log_rounds:02d}$'
    else:
        s = f'${method.ident}$'
    s += ''.join(_sr.choice(_saltchars) for char in range(method.salt_chars))
    return s


def crypt(word, salt=None):
    """Return a string representing the one-way hash of a password, with a salt
    prepended.

    If ``salt`` is not specified or is ``None``, the strongest
    available method will be selected and a salt generated.  Otherwise,
    ``salt`` may be one of the ``crypt.METHOD_*`` values, or a string as
    returned by ``crypt.mksalt()``.

    """
    if salt is None or isinstance(salt, _Method):
        salt = mksalt(salt)
    return _crypt.crypt(word, salt)


#  available salting/crypto methods
methods = []

def _add_method(name, *args):
    method = _Method(name, *args)
    globals()['METHOD_' + name] = method
    salt = mksalt(method, log_rounds=4)
    result = crypt('', salt)
    if result and len(result) == method.total_size:
        methods.append(method)
        return True
    return False

_add_method('SHA512', '6', 16, 106)
_add_method('SHA256', '5', 16, 63)

# Choose the strongest supported version of Blowfish hashing.
# Early versions have flaws.  Version 'a' fixes flaws of
# the initial implementation, 'b' fixes flaws of 'a'.
# 'y' is the same as 'b', for compatibility
# with openwall crypt_blowfish.
for _v in 'b', 'y', 'a', '':
    if _add_method('BLOWFISH', '2' + _v, 22, 59 + len(_v)):
        break

_add_method('MD5', '1', 8, 34)
_add_method('CRYPT', None, 2, 13)

del _v, _add_method
