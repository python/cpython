"""HMAC (Keyed-Hashing for Message Authentication) module.

Implements the HMAC algorithm as described by RFC 2104.
"""

import warnings as _warnings
try:
    import _hashlib as _hashopenssl
except ImportError:
    _hashopenssl = None
    _functype = None
    from _operator import _compare_digest as compare_digest
else:
    compare_digest = _hashopenssl.compare_digest
    _functype = type(_hashopenssl.openssl_sha256)  # builtin type

import hashlib as _hashlib

# Constants for padding
INNER_PAD = 0x36
OUTER_PAD = 0x5C

# The size of the digests returned by HMAC depends on the underlying
# hashing module used. Use digest_size from the instance of HMAC instead.
digest_size = None


class HMAC:
    """RFC 2104 HMAC class. Complies with RFC 4231 and PEP 247."""

    blocksize = 64  # 512-bit HMAC; can be changed in subclasses.

    __slots__ = ("_hmac", "_inner", "_outer", "block_size", "digest_size")

    def __init__(self, key: bytes, msg: bytes = None, digestmod: str = '') -> None:
        """Create a new HMAC object.

        key: bytes or buffer, key for the keyed hash object.
        msg: bytes or buffer, Initial input for the hash or None.
        digestmod: A hash name suitable for hashlib.new(). *OR*
                   A hashlib constructor returning a new hash object. *OR*
                   A module supporting PEP 247.
        """
        if not isinstance(key, (bytes, bytearray)):
            raise TypeError(f"Expected bytes or bytearray for key, got {type(key).__name__}")
        if not digestmod:
            raise TypeError("Missing required argument 'digestmod'.")

        if _hashopenssl and isinstance(digestmod, (str, _functype)):
            try:
                self._init_hmac(key, msg, digestmod)
            except _hashopenssl.UnsupportedDigestmodError:
                self._init_old(key, msg, digestmod)
        else:
            self._init_old(key, msg, digestmod)
    
    def _init_hmac(self, key: bytes, msg: bytes, digestmod: str) -> None:
        """Initialize HMAC using openssl."""
        self._hmac = _hashopenssl.hmac_new(key, msg, digestmod=digestmod)
        self.digest_size = self._hmac.digest_size
        self.block_size = self._hmac.block_size

    def _init_old(self, key: bytes, msg: bytes, digestmod: str) -> None:
        """Initialize HMAC for old methods."""
        digest_cons = self._get_digest_cons(digestmod)
        self._outer = digest_cons()
        self._inner = digest_cons()
        self.digest_size = self._inner.digest_size
        self._handle_blocksize(self._inner.block_size)

        # Prepare the key
        key = self._prepare_key(key, self.block_size)
        self._outer.update(key.translate(trans_5C))
        self._inner.update(key.translate(trans_36))
        if msg is not None:
            self.update(msg)

    def _get_digest_cons(self, digestmod: str):
        """Returns a suitable digest constructor."""
        if callable(digestmod):
            return digestmod
        elif isinstance(digestmod, str):
            return lambda d=b'': _hashlib.new(digestmod, d)
        else:
            return lambda d=b'': digestmod.new(d)

    def _handle_blocksize(self, blocksize: int) -> None:
        """Handle blocksize warnings."""
        if blocksize < 16:
            _warnings.warn(f'block_size of {blocksize} seems too small; using default of {self.blocksize}.', RuntimeWarning, 2)
            blocksize = self.blocksize
        self.block_size = blocksize

    def _prepare_key(self, key: bytes, blocksize: int) -> bytes:
        """Prepare the key for the HMAC calculation."""
        if len(key) > blocksize:
            key = _hashlib.new(self._inner.name, key).digest()
        return key.ljust(blocksize, b'\0')

    @property
    def name(self) -> str:
        """Return the name of the hashing algorithm."""
        if self._hmac:
            return self._hmac.name
        else:
            return f"hmac-{self._inner.name}"

    def update(self, msg: bytes) -> None:
        """Feed data into this hashing object."""
        inst = self._hmac or self._inner
        inst.update(msg)

    def copy(self) -> 'HMAC':
        """Return a separate copy of this hashing object.

        An update to this copy won't affect the original object.
        """
        other = self.__class__.__new__(self.__class__)
        other.digest_size = self.digest_size
        if self._hmac:
            other._hmac = self._hmac.copy()
            other._inner = other._outer = None
        else:
            other._hmac = None
            other._inner = self._inner.copy()
            other._outer = self._outer.copy()
        return other

    def _current(self):
        """Return a hash object for the current state.

        To be used only internally with digest() and hexdigest().
        """
        if self._hmac:
            return self._hmac
        else:
            h = self._outer.copy()
            h.update(self._inner.digest())
            return h

    def digest(self) -> bytes:
        """Return the hash value of this hashing object.

        This returns the hmac value as bytes. The object is
        not altered in any way by this function; you can continue
        updating the object after calling this function.
        """
        h = self._current()
        return h.digest()

    def hexdigest(self) -> str:
        """Like digest(), but returns a string of hexadecimal digits instead."""
        h = self._current()
        return h.hexdigest()


def new(key: bytes, msg: bytes = None, digestmod: str = '') -> HMAC:
    """Create a new HMAC object and return it.

    key: bytes or buffer, The starting key for the hash.
    msg: bytes or buffer, Initial input for the hash, or None.
    digestmod: A hash name suitable for hashlib.new(). *OR*
               A hashlib constructor returning a new hash object. *OR*
               A module supporting PEP 247.

    You can now feed arbitrary bytes into the object using its update()
    method, and can ask for the hash value at any time by calling its digest()
    or hexdigest() methods.
    """
    return HMAC(key, msg, digestmod)


def digest(key: bytes, msg: bytes, digest: str) -> bytes:
    """Fast inline implementation of HMAC.

    key: bytes or buffer, The key for the keyed hash object.
    msg: bytes or buffer, Input message.
    digest: A hash name suitable for hashlib.new() for best performance. *OR*
            A hashlib constructor returning a new hash object. *OR*
            A module supporting PEP 247.
    """
    if _hashopenssl is not None and isinstance(digest, (str, _functype)):
        try:
            return _hashopenssl.hmac_digest(key, msg, digest)
        except _hashopenssl.UnsupportedDigestmodError:
            pass

    digest_cons = _get_digest_cons(digest)
    inner = digest_cons()
    outer = digest_cons()
    blocksize = getattr(inner, 'block_size', 64)
    if len(key) > blocksize:
        key = digest_cons(key).digest()
    key = key + b'\x00' * (blocksize - len(key))
    inner.update(key.translate(trans_36))
    outer.update(key.translate(trans_5C))
    inner.update(msg)
    outer.update(inner.digest())
    return outer.digest()

