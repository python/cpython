"""HMAC (Keyed-Hashing for Message Authentication) Python module.

Implements the HMAC algorithm as described by RFC 2104.
"""

import string

def _strxor(s1, s2):
    """Utility method. XOR the two strings s1 and s2 (must have same length).
    """
    return "".join(map(lambda x, y: chr(ord(x) ^ ord(y)), s1, s2))
        
class HMAC:
    """RFC2104 HMAC class.

    This (mostly) supports the API for Cryptographic Hash Functions (PEP 247).
    """ 

    def __init__(self, key, msg = None, digestmod = None):
        """Create a new HMAC object.

        key:       key for the keyed hash object.
        msg:       Initial input for the hash, if provided.
        digestmod: A module supporting PEP 247. Defaults to the md5 module.
        """
        if digestmod == None:
            import md5
            digestmod = md5

        self.outer = digestmod.new()
        self.inner = digestmod.new()

        blocksize = 64
        ipad = "\x36" * blocksize
        opad = "\x5C" * blocksize

        if len(key) > blocksize:
            key = digestmod.new(key).digest()

        key = key + chr(0) * (blocksize - len(key))
        self.outer.update(_strxor(key, opad))
        self.inner.update(_strxor(key, ipad))
        if (msg):
            self.update(msg)

##    def clear(self):
##        raise NotImplementedError, "clear() method not available in HMAC."

    def update(self, msg):
        """Update this hashing object with the string msg.
        """
        self.inner.update(msg)

    def copy(self):
        """Return a separate copy of this hashing object.

        An update to this copy won't affect the original object.
        """
        return HMAC(self)

    def digest(self):
        """Return the hash value of this hashing object.

        This returns a string containing 8-bit data.  The object is
        not altered in any way by this function; you can continue
        updating the object after calling this function.
        """
        h = self.outer.copy()
        h.update(self.inner.digest())
        return h.digest()

    def hexdigest(self):
        """Like digest(), but returns a string of hexadecimal digits instead.
        """
        return "".join([string.zfill(hex(ord(x))[2:], 2)
                        for x in tuple(self.digest())])

def new(key, msg = None, digestmod = None):
    """Create a new hashing object and return it.

    key: The starting key for the hash.
    msg: if available, will immediately be hashed into the object's starting
    state.  

    You can now feed arbitrary strings into the object using its update()
    method, and can ask for the hash value at any time by calling its digest()
    method.
    """
    return HMAC(key, msg, digestmod)

def test():
    def md5test(key, data, digest):
        h = HMAC(key, data)
        assert(h.hexdigest().upper() == digest.upper())

    # Test vectors from the RFC
    md5test(chr(0x0b) * 16,
            "Hi There",
            "9294727A3638BB1C13F48EF8158BFC9D")

    md5test("Jefe",
            "what do ya want for nothing?",
            "750c783e6ab0b503eaa86e310a5db738")

    md5test(chr(0xAA)*16,
            chr(0xDD)*50,
            "56be34521d144c88dbb8c733f0e8b3f6")

if __name__ == "__main__":
    test()
