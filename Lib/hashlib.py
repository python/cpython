#.  Copyright (C) 2005-2010   Gregory P. Smith (greg@krypto.org)
#  Licensed to PSF under a Contributor Agreement.
#

__doc__ = r"""hashlib module - A common interface to many hash functions.

new(name, data=b'', **kwargs) - returns a new hash object implementing the
                                given hash function; initializing the hash
                                using the given binary data.

Named constructor functions are also available, these are faster
than using new(name):

md5(), sha1(), sha224(), sha256(), sha384(), sha512(), blake2b(), blake2s(),
sha3_224(), sha3_256(), sha3_384(), sha3_512(), shake_128(), and shake_256().

More algorithms may be available on your platform but the above are guaranteed
to exist.  See the algorithms_guaranteed and algorithms_available attributes
to find out what algorithm names can be passed to new().

NOTE: If you want the adler32 or crc32 hash functions they are available in
the zlib module.

Choose your hash function wisely.  Some have known collision weaknesses,
while others may be slower depending on the CPU architecture.

Hash objects have these methods:
 - update(data): Update the hash object with the bytes in data. Repeated calls
                 are equivalent to a single call with the concatenation of all
                 the arguments.
 - digest():     Return the digest of the bytes passed to the update() method
                 so far as a bytes object.
 - hexdigest():  Like digest() except the digest is returned as a string
                 of double length, containing only hexadecimal digits.
 - copy():       Return a copy (clone) of the hash object. This can be used to
                 efficiently compute the digests of data that share a common
                 initial substring.

Assuming that Python has been built with SHA-2 support, the SHA-256 digest
of the byte string b'Nobody inspects the spammish repetition' is computed
as follows:

    >>> import hashlib
    >>> m = hashlib.sha256()
    >>> m.update(b"Nobody inspects")
    >>> m.update(b" the spammish repetition")
    >>> m.digest()  # doctest: +ELLIPSIS
    b'\x03\x1e\xdd}Ae\x15\x93\xc5\xfe\\\x00o\xa5u+7...'

More condensed:

    >>> hashlib.sha256(b"Nobody inspects the spammish repetition").hexdigest()
    '031edd7d41651593c5fe5c006fa5752b37fddff7bc4e843aa6af0c950f4b9406'
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
    if not isinstance(name, str):
        # Since this function is only used by new(), we use the same
        # exception as _hashlib.new() when 'name' is of incorrect type.
        err = f"new() argument 'name' must be str, not {type(name).__name__}"
        raise TypeError(err)
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
            import _sha2
            cache['SHA224'] = cache['sha224'] = _sha2.sha224
            cache['SHA256'] = cache['sha256'] = _sha2.sha256
        elif name in {'SHA512', 'sha512', 'SHA384', 'sha384'}:
            import _sha2
            cache['SHA384'] = cache['sha384'] = _sha2.sha384
            cache['SHA512'] = cache['sha512'] = _sha2.sha512
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

    # Keep the message in sync with hashlib.h::HASHLIB_UNSUPPORTED_ALGORITHM.
    raise ValueError(f'unsupported hash algorithm {name}')


def __get_openssl_constructor(name):
    # This function is only used until the module has been initialized.
    assert isinstance(name, str), "invalid call to __get_openssl_constructor()"
    if name in __block_openssl_constructor:
        # Prefer our builtin blake2 implementation.
        return __get_builtin_constructor(name)
    try:
        # Fetch the OpenSSL hash function if it exists,
        # independently of the context security policy.
        f = getattr(_hashlib, 'openssl_' + name)
        # Check if the context security policy blocks the digest or not
        # by allowing the C module to raise a ValueError. The function
        # will be defined but the hash will not be available at runtime.
        #
        # We use "usedforsecurity=False" to prevent falling back to the
        # built-in function in case the security policy does not allow it.
        #
        # Note that this only affects the explicit named constructors,
        # and not the algorithms exposed through hashlib.new() which
        # can still be resolved to a built-in function even if the
        # current security policy does not allow it.
        #
        # See https://github.com/python/cpython/issues/84872.
        f(usedforsecurity=False)
        # Use the C function directly (very fast)
        return f
    except (AttributeError, ValueError):
        return __get_builtin_constructor(name)


def __py_new(name, *args, **kwargs):
    """new(name, data=b'', **kwargs) - Return a new hashing object using the
    named algorithm; optionally initialized with data (which must be
    a bytes-like object).
    """
    return __get_builtin_constructor(name)(*args, **kwargs)


def __hash_new(name, *args, **kwargs):
    """new(name, data=b'') - Return a new hashing object using the named algorithm;
    optionally initialized with data (which must be a bytes-like object).
    """
    if name in __block_openssl_constructor:
        # __block_openssl_constructor is expected to contain strings only
        assert isinstance(name, str), f"unexpected name: {name}"
        # Prefer our builtin blake2 implementation.
        return __get_builtin_constructor(name)(*args, **kwargs)
    try:
        return _hashlib.new(name, *args, **kwargs)
    except ValueError:
        # If the _hashlib module (OpenSSL) doesn't support the named
        # hash, try using our builtin implementations.
        # This allows for SHA224/256 and SHA384/512 support even though
        # the OpenSSL library prior to 0.9.8 doesn't provide them.
        return __get_builtin_constructor(name)(*args, **kwargs)


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
    __all__ += ('scrypt',)
except ImportError:
    pass


def file_digest(fileobj, digest, /, *, _bufsize=2**18):
    """Hash the contents of a file-like object. Returns a digest object.

    *fileobj* must be a file-like object opened for reading in binary mode.
    It accepts file objects from open(), io.BytesIO(), and SocketIO objects.
    The function may bypass Python's I/O and use the file descriptor *fileno*
    directly.

    *digest* must either be a hash algorithm name as a *str*, a hash
    constructor, or a callable that returns a hash object.
    """
    # On Linux we could use AF_ALG sockets and sendfile() to achieve zero-copy
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
        if size is None:
            raise BlockingIOError("I/O operation would block.")
        if size == 0:
            break  # EOF
        digestobj.update(view[:size])

    return digestobj


__logging = None
for __func_name in __always_supported:
    # try them all, some may not work due to the OpenSSL
    # version not supporting that algorithm.
    try:
        globals()[__func_name] = __get_hash(__func_name)
    except ValueError as __exc:
        import logging as __logging
        __logging.error('hash algorithm %s will not be supported at runtime '
                        '[reason: %s]', __func_name, __exc)
        # The following code can be simplified in Python 3.19
        # once "string" is removed from the signature.
        __code = f'''\
def {__func_name}(data=__UNSET, *, usedforsecurity=True, string=__UNSET):
    if data is __UNSET and string is not __UNSET:
        import warnings
        warnings.warn(
            "the 'string' keyword parameter is deprecated since "
            "Python 3.15 and slated for removal in Python 3.19; "
            "use the 'data' keyword parameter or pass the data "
            "to hash as a positional argument instead",
            DeprecationWarning, stacklevel=2)
    if data is not __UNSET and string is not __UNSET:
        raise TypeError("'data' and 'string' are mutually exclusive "
                        "and support for 'string' keyword parameter "
                        "is slated for removal in a future version.")
    raise ValueError("unsupported hash algorithm {__func_name}")
'''
        exec(__code, {"__UNSET": object()}, __locals := {})
        globals()[__func_name] = __locals[__func_name]
        del __exc, __code, __locals

# Cleanup locals()
del __always_supported, __func_name, __get_hash
del __py_new, __hash_new, __get_openssl_constructor
del __logging
