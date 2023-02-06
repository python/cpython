#.  Copyright (C) 2005-2010   Gregory P. Smith (greg@krypto.org)
#  Licensed to PSF under a Contributor Agreement.
#

__doc__ = """hashlib module - A common interface to many hash functions.

new(name, data=b'', **kwargs) - returns a new hash object implementing the
                                given hash function; initializing the hash
                                using the given binary data.

Named constructor functions are also available, these are faster
than using new(name):

md5(), sha1(), sha224(), sha256(), sha384(), sha512(), blake2b(), blake2s(),
sha3_224, sha3_256, sha3_384, sha3_512, shake_128, and shake_256.

More algorithms may be available on your platform but the above are guaranteed
to exist.  See the algorithms_guaranteed and algorithms_available attributes
to find out what algorithm names can be passed to new().

NOTE: If you want the adler32 or crc32 hash functions they are available in
the zlib module.

Choose your hash function wisely.  Some have known collision weaknesses.
sha384 and sha512 will be slow on 32 bit platforms.

Hash objects have these methods:
 - update(data): Update the hash object with the bytes in data. Repeated calls
                 are equivalent to a single call with the concatenation of all
                 the arguments.
 - digest():     Return the digest of the bytes passed to the update() method
                 so far as a bytes object.
 - hexdigest():  Like digest() except the digest is returned as a string
                 of double length, containing only hexadecimal digits.
 - copy():       Return a copy (clone) of the hash object. This can be used to
                 efficiently compute the digests of datas that share a common
                 initial substring.

For example, to obtain the digest of the byte string 'Nobody inspects the
spammish repetition':

    >>> import hashlib
    >>> m = hashlib.md5()
    >>> m.update(b"Nobody inspects")
    >>> m.update(b" the spammish repetition")
    >>> m.digest()
    b'\\xbbd\\x9c\\x83\\xdd\\x1e\\xa5\\xc9\\xd9\\xde\\xc9\\xa1\\x8d\\xf0\\xff\\xe9'

More condensed:

    >>> hashlib.sha224(b"Nobody inspects the spammish repetition").hexdigest()
    'a4337bc45a8fc544c03f52dc550cd6e1e87021bc896588bd79e901e2'

"""

# This tuple and __get_builtin_constructor() must be modified if a new
# always available algorithm is added.
__always_supported = ('md5', 'sha1', 'sha224', 'sha256', 'sha384', 'sha512',
                      'blake2b', 'blake2s',
                      'sha3_224', 'sha3_256', 'sha3_384', 'sha3_512',
                      'shake_128', 'shake_256')


algorithms_guaranteed = set(__always_supported)
algorithms_available = set(__always_supported)

__all__ = __always_supported + ('new', 'algorithms_guaranteed',
                                'algorithms_available', 'file_digest')


__builtin_constructor_cache = {}

# Prefer our blake2 implementation
# OpenSSL 1.1.0 comes with a limited implementation of blake2b/s. The OpenSSL
# implementations neither support keyed blake2 (blake2 MAC) nor advanced
# features like salt, personalization, or tree hashing. OpenSSL hash-only
# variants are available as 'blake2b512' and 'blake2s256', though.
__block_openssl_constructor = {
    'blake2b', 'blake2s',
}

def __get_builtin_constructor(name):
    cache = __builtin_constructor_cache
    constructor = cache.get(name)
    if constructor is not None:
        return constructor
    try:
        if name in {'SHA1', 'sha1'}:
            import _sha1
            cache['SHA1'] = cache['sha1'] = _sha1.sha1
        elif name in {'MD5', 'md5'}:
            import _md5
            cache['MD5'] = cache['md5'] = _md5.md5
        elif name in {'SHA256', 'sha256', 'SHA224', 'sha224'}:
            import _sha256
            cache['SHA224'] = cache['sha224'] = _sha256.sha224
            # cache['SHA256'] = cache['sha256'] = _sha256.sha256
            cache['SHA256'] = cache['sha256'] = linux_sha256
        elif name in {'SHA512', 'sha512', 'SHA384', 'sha384'}:
            import _sha512
            cache['SHA384'] = cache['sha384'] = _sha512.sha384
            cache['SHA512'] = cache['sha512'] = _sha512.sha512
        elif name in {'blake2b', 'blake2s'}:
            import _blake2
            cache['blake2b'] = _blake2.blake2b
            cache['blake2s'] = _blake2.blake2s
        elif name in {'sha3_224', 'sha3_256', 'sha3_384', 'sha3_512'}:
            import _sha3
            cache['sha3_224'] = _sha3.sha3_224
            cache['sha3_256'] = _sha3.sha3_256
            cache['sha3_384'] = _sha3.sha3_384
            cache['sha3_512'] = _sha3.sha3_512
        elif name in {'shake_128', 'shake_256'}:
            import _sha3
            cache['shake_128'] = _sha3.shake_128
            cache['shake_256'] = _sha3.shake_256
    except ImportError:
        pass  # no extension module, this hash is unsupported.

    constructor = cache.get(name)
    if constructor is not None:
        return constructor

    raise ValueError('unsupported hash type ' + name)


def __get_openssl_constructor(name):
    if name in __block_openssl_constructor:
        # Prefer our builtin blake2 implementation.
        return __get_builtin_constructor(name)
    try:
        # MD5, SHA1, and SHA2 are in all supported OpenSSL versions
        # SHA3/shake are available in OpenSSL 1.1.1+
        f = getattr(_hashlib, 'openssl_' + name)
        # Allow the C module to raise ValueError.  The function will be
        # defined but the hash not actually available.  Don't fall back to
        # builtin if the current security policy blocks a digest, bpo#40695.
        f(usedforsecurity=False)
        # Use the C function directly (very fast)
        return f
    except (AttributeError, ValueError):
        return __get_builtin_constructor(name)


def __py_new(name, data=b'', **kwargs):
    """new(name, data=b'', **kwargs) - Return a new hashing object using the
    named algorithm; optionally initialized with data (which must be
    a bytes-like object).
    """
    return __get_builtin_constructor(name)(data, **kwargs)


def __hash_new(name, data=b'', **kwargs):
    """new(name, data=b'') - Return a new hashing object using the named algorithm;
    optionally initialized with data (which must be a bytes-like object).
    """
    if name in __block_openssl_constructor:
        # Prefer our builtin blake2 implementation.
        return __get_builtin_constructor(name)(data, **kwargs)
    try:
        return _hashlib.new(name, data, **kwargs)
    except ValueError:
        # If the _hashlib module (OpenSSL) doesn't support the named
        # hash, try using our builtin implementations.
        # This allows for SHA224/256 and SHA384/512 support even though
        # the OpenSSL library prior to 0.9.8 doesn't provide them.
        return __get_builtin_constructor(name)(data)


try:
    import _hashlib
    new = __hash_new
    __get_hash = __get_openssl_constructor
    algorithms_available = algorithms_available.union(
            _hashlib.openssl_md_meth_names)
except ImportError:
    _hashlib = None
    new = __py_new
    __get_hash = __get_builtin_constructor

try:
    # OpenSSL's PKCS5_PBKDF2_HMAC requires OpenSSL 1.0+ with HMAC and SHA
    from _hashlib import pbkdf2_hmac
    __all__ += ('pbkdf2_hmac',)
except ImportError:
    pass


try:
    # OpenSSL's scrypt requires OpenSSL 1.1+
    from _hashlib import scrypt
except ImportError:
    pass


import socket as _socket
import binascii as _binascii
import warnings as _warnings

class _LinuxKCAPI:
    """Linux Kernel Crypto API (AF_ALG socket)
    """

    # TODO: retrieve info from AF_NETLINK, NETLINK_CRYPTO
    _digests = {
        'md5': (16, 64),
        'sha1': (20, 64),
        'sha224': (28, 64),
        'sha256': (32, 64),
        'sha384': (48, 128),
        'sha512': (64, 128),
    }
    __slots__ = ("_digest", "_kcapi_sock")

    def __init__(self, digest):
        digest = digest.lower()  # SHA256 -> sha256
        if digest not in self._digests:
            raise ValueError(f"Kernel Crypto API does not support {digest}.")
        self._digest = digest

    def __getstate__(self):
        raise TypeError(f"cannot pickle {self.__class__.__name__!r} object")

    def __del__(self):
        if getattr(self, "_kcapi_sock", None):
            self._kcapi_sock.close()
            self._kcapi_sock = None

    def _get_kcapi_sock(self, digest, mac_key=None):
        """Create KCAPI client socket

        Algorithm and MAC key are configured on the server socket. accept()
        creates a new client socket that consumes data. accept() on a client
        socket creates an independent copy.

        https://www.kernel.org/doc/html/v5.17/crypto/userspace-if.html
        """
        with _socket.socket(_socket.AF_ALG, _socket.SOCK_SEQPACKET, 0) as cfg:
            algo = digest if mac_key is None else f"hmac({digest})"
            binding = ("hash", algo)
            try:
                cfg.bind(binding)
            except FileNotFoundError:
                raise ValueError(
                    f"Kernel Crypto API does not support {algo}."
                )
            if mac_key is not None:
                # Linux Kernel 5.17 docs are incorrect. AF_ALG setsockopt()
                # requires a non-connected 'server' socket.
                # if (sock->state == SS_CONNECTED) return ENOPROTOOPT;
                cfg.setsockopt(_socket.SOL_ALG, _socket.ALG_SET_KEY, mac_key)
            return cfg.accept()[0]

    def _digest_by_digestmod(self, digestmod):
        """Get digest object and name
        """
        if isinstance(digestmod, str):
            return None, digestmod
        elif callable(digestmod):
            if _hashlib is not None:
                # _hashopenssl.c constructor?
                try:
                    return None, _hashlib._constructors[digestmod]
                except KeyError:
                    pass
            # callable
            digestobj = digestmod()
            return digestobj, digestobj.name
        else:
            # digest module with new() function
            digestobj = digestmod.new()
            return digestobj, digestobj.name

    def __repr__(self):
        return f"<Linux KCAPI {self.name} at 0x{id(self):x}>"

    @property
    def digest_size(self):
        return self._digests[self._digest][0]

    @property
    def block_size(self):
        return self._digests[self._digest][1]

    def update(self, data):
        self._kcapi_sock.sendall(data, _socket.MSG_MORE)

    def copy(self):
        new = self.__new__(type(self))
        new._digest = self._digest
        new._kcapi_sock = self._kcapi_sock.accept()[0]
        return new

    def digest(self):
        copysock = self._kcapi_sock.accept()[0]
        with copysock:
            copysock.send(b'')
            return copysock.recv(64)

    def hexdigest(self):
        return _binascii.hexlify(self.digest()).decode("ascii")


class _LinuxKCAPIHash(_LinuxKCAPI):
    def __init__(self, name, data=None, usedforsecurity=True):
        super().__init__(name)
        # ignores usedforsecurity
        self._kcapi_sock = self._get_kcapi_sock(name)
        if data is not None:
            self.update(data)

    @property
    def name(self):
        return self._digest


class _LinuxKCAPIHMAC(_LinuxKCAPI):
    def __init__(self, key, msg=None, digestmod=''):
        if not isinstance(key, (bytes, bytearray)):
            raise TypeError("key: expected bytes or bytearray, but got %r" % type(key).__name__)

        if not digestmod:
            raise TypeError("Missing required parameter 'digestmod'.")
        digestobj, name = self._digest_by_digestmod(digestmod)
        super().__init__(name)
        if digestobj:
            if not hasattr(digestobj, "block_size"):
                _warnings.warn(
                    "No block_size attribute on given digest object.",
                    RuntimeWarning, 2
                )
            elif digestobj.block_size != self.block_size:
                _warnings.warn(
                    f"digest object block_size {digestobj.block_size} does "
                    f"not match KCAPI block_size {self.block_size}.",
                    RuntimeWarning, 2
                )

        self._kcapi_sock = self._get_kcapi_sock(name, key)
        if msg is not None:
            self.update(msg)

    @property
    def name(self):
        return f"hmac-{self._digest}"


def linux_sha256(data=None, usedforsecurity=True):
    return _LinuxKCAPIHash("sha256", data, usedforsecurity=usedforsecurity)


def file_digest(fileobj, digest, /, *, _bufsize=2**18):
    """Hash the contents of a file-like object. Returns a digest object.

    *fileobj* must be a file-like object opened for reading in binary mode.
    It accepts file objects from open(), io.BytesIO(), and SocketIO objects.
    The function may bypass Python's I/O and use the file descriptor *fileno*
    directly.

    *digest* must either be a hash algorithm name as a *str*, a hash
    constructor, or a callable that returns a hash object.
    """
    # On Linux we could use AF_ALG sockets and sendfile() to archive zero-copy
    # hashing with hardware acceleration.
    if isinstance(digest, str):
        digestobj = new(digest)
    else:
        digestobj = digest()

    if hasattr(fileobj, "getbuffer"):
        # io.BytesIO object, use zero-copy buffer
        digestobj.update(fileobj.getbuffer())
        return digestobj

    # Only binary files implement readinto().
    if not (
        hasattr(fileobj, "readinto")
        and hasattr(fileobj, "readable")
        and fileobj.readable()
    ):
        raise ValueError(
            f"'{fileobj!r}' is not a file-like object in binary reading mode."
        )

    # binary file, socket.SocketIO object
    # Note: socket I/O uses different syscalls than file I/O.
    buf = bytearray(_bufsize)  # Reusable buffer to reduce allocations.
    view = memoryview(buf)
    while True:
        size = fileobj.readinto(buf)
        if size == 0:
            break  # EOF
        digestobj.update(view[:size])

    return digestobj


for __func_name in __always_supported:
    # try them all, some may not work due to the OpenSSL
    # version not supporting that algorithm.
    try:
        globals()[__func_name] = __get_hash(__func_name)
    except ValueError:
        import logging
        logging.exception('code for hash %s was not found.', __func_name)


# Cleanup locals()
del __always_supported, __func_name, __get_hash
del __py_new, __hash_new, __get_openssl_constructor
