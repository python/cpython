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
    elif method.ident == '_':
        s = method.ident
    elif method.ident[0] == '2':
        s = f'${method.ident}${log_rounds:02d}$'
    elif method.ident == '3':
        return f'${method.ident}$$'
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

def _probe(method):
    result = crypt('', method)
    if result and len(result) == method.total_size:
        methods.append(method)
        return True
    return False

METHOD_SHA512 = _Method('SHA512', '6', 16, 106)
_probe(METHOD_SHA512)
METHOD_SHA256 = _Method('SHA256', '5', 16, 63)
_probe(METHOD_SHA256)

for _v in 'b', 'y', 'a', '':
    METHOD_BLF = _Method('BLF', '2' + _v, 22, 59 + len(_v))
    if _probe(METHOD_BLF):
        break

METHOD_MD5 = _Method('MD5', '1', 8, 34)
_probe(METHOD_MD5)
METHOD_DES = _Method('DES', '_', 8, 20)
_probe(METHOD_DES)
METHOD_NTH = _Method('NTH', '3', 0, 36)
_probe(METHOD_NTH)
METHOD_CRYPT = _Method('CRYPT', None, 2, 13)
_probe(METHOD_CRYPT)

del _v, _probe
