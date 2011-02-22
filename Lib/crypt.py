'''Wrapper to the POSIX crypt library call and associated functionality.
'''

import _crypt

saltchars = 'abcdefghijklmnopqrstuvwxyz'
saltchars += saltchars.upper()
saltchars += '0123456789./'


class _MethodClass:
    '''Class representing a salt method per the Modular Crypt Format or the
    legacy 2-character crypt method.'''
    def __init__(self, name, ident, salt_chars, total_size):
        self.name = name
        self.ident = ident
        self.salt_chars = salt_chars
        self.total_size = total_size

    def __repr__(self):
        return '<crypt.METHOD_%s>' % self.name


#  available salting/crypto methods
METHOD_CRYPT = _MethodClass('CRYPT', None, 2, 13)
METHOD_MD5 = _MethodClass('MD5', '1', 8, 34)
METHOD_SHA256 = _MethodClass('SHA256', '5', 16, 63)
METHOD_SHA512 = _MethodClass('SHA512', '6', 16, 106)


def methods():
    '''Return a list of methods that are available in the platform ``crypt()``
    library, sorted from strongest to weakest.  This is guaranteed to always
    return at least ``[METHOD_CRYPT]``'''
    method_list = [ METHOD_SHA512, METHOD_SHA256, METHOD_MD5 ]
    ret = [ method for method in method_list
            if len(crypt('', method)) == method.total_size ]
    ret.append(METHOD_CRYPT)
    return ret


def mksalt(method = None):
    '''Generate a salt for the specified method.  If not specified, the
    strongest available method will be used.'''
    import random

    if method == None: method = methods()[0]
    s = '$%s$' % method.ident if method.ident else ''
    s += ''.join([ random.choice(saltchars) for x in range(method.salt_chars) ])
    return(s)


def crypt(word, salt = None):
    '''Return a string representing the one-way hash of a password, preturbed
    by a salt.  If ``salt`` is not specified or is ``None``, the strongest
    available method will be selected and a salt generated.  Otherwise,
    ``salt`` may be one of the ``crypt.METHOD_*`` values, or a string as
    returned by ``crypt.mksalt()``.'''
    if salt == None: salt = mksalt()
    elif isinstance(salt, _MethodClass): salt = mksalt(salt)
    return(_crypt.crypt(word, salt))
