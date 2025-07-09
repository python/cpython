import functools
import hashlib
import importlib
import unittest
from test.support.import_helper import import_module

try:
    import _hashlib
except ImportError:
    _hashlib = None

try:
    import _hmac
except ImportError:
    _hmac = None


def requires_hashlib():
    return unittest.skipIf(_hashlib is None, "requires _hashlib")


def requires_builtin_hmac():
    return unittest.skipIf(_hmac is None, "requires _hmac")


def _missing_hash(digestname, implementation=None, *, exc=None):
    parts = ["missing", implementation, f"hash algorithm: {digestname!r}"]
    msg = " ".join(filter(None, parts))
    raise unittest.SkipTest(msg) from exc


def _openssl_availabillity(digestname, *, usedforsecurity):
    try:
        _hashlib.new(digestname, usedforsecurity=usedforsecurity)
    except AttributeError:
        assert _hashlib is None
        _missing_hash(digestname, "OpenSSL")
    except ValueError as exc:
        _missing_hash(digestname, "OpenSSL", exc=exc)


def _decorate_func_or_class(func_or_class, decorator_func):
    if not isinstance(func_or_class, type):
        return decorator_func(func_or_class)

    decorated_class = func_or_class
    setUpClass = decorated_class.__dict__.get('setUpClass')
    if setUpClass is None:
        def setUpClass(cls):
            super(decorated_class, cls).setUpClass()
        setUpClass.__qualname__ = decorated_class.__qualname__ + '.setUpClass'
        setUpClass.__module__ = decorated_class.__module__
    else:
        setUpClass = setUpClass.__func__
    setUpClass = classmethod(decorator_func(setUpClass))
    decorated_class.setUpClass = setUpClass
    return decorated_class


def requires_hashdigest(digestname, openssl=None, usedforsecurity=True):
    """Decorator raising SkipTest if a hashing algorithm is not available.

    The hashing algorithm may be missing, blocked by a strict crypto policy,
    or Python may be configured with `--with-builtin-hashlib-hashes=no`.

    If 'openssl' is True, then the decorator checks that OpenSSL provides
    the algorithm. Otherwise the check falls back to (optional) built-in
    HACL* implementations.

    The usedforsecurity flag is passed to the constructor but has no effect
    on HACL* implementations.

    Examples of exceptions being suppressed:
    ValueError: [digital envelope routines: EVP_DigestInit_ex] disabled for FIPS
    ValueError: unsupported hash type md4
    """
    if openssl and _hashlib is not None:
        def test_availability():
            _hashlib.new(digestname, usedforsecurity=usedforsecurity)
    else:
        def test_availability():
            hashlib.new(digestname, usedforsecurity=usedforsecurity)

    def decorator_func(func):
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            try:
                test_availability()
            except ValueError as exc:
                _missing_hash(digestname, exc=exc)
            return func(*args, **kwargs)
        return wrapper

    def decorator(func_or_class):
        return _decorate_func_or_class(func_or_class, decorator_func)
    return decorator


def requires_openssl_hashdigest(digestname, *, usedforsecurity=True):
    """Decorator raising SkipTest if an OpenSSL hashing algorithm is missing.

    The hashing algorithm may be missing or blocked by a strict crypto policy.
    """
    def decorator_func(func):
        @requires_hashlib()  # avoid checking at each call
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            _openssl_availabillity(digestname, usedforsecurity=usedforsecurity)
            return func(*args, **kwargs)
        return wrapper

    def decorator(func_or_class):
        return _decorate_func_or_class(func_or_class, decorator_func)
    return decorator


def find_openssl_hashdigest_constructor(digestname, *, usedforsecurity=True):
    """Find the OpenSSL hash function constructor by its name."""
    assert isinstance(digestname, str), digestname
    _openssl_availabillity(digestname, usedforsecurity=usedforsecurity)
    # This returns a function of the form _hashlib.openssl_<name> and
    # not a lambda function as it is rejected by _hashlib.hmac_new().
    return getattr(_hashlib, f"openssl_{digestname}")


def requires_builtin_hashdigest(
    module_name, digestname, *, usedforsecurity=True
):
    """Decorator raising SkipTest if a HACL* hashing algorithm is missing.

    - The *module_name* is the C extension module name based on HACL*.
    - The *digestname* is one of its member, e.g., 'md5'.
    """
    def decorator_func(func):
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            module = import_module(module_name)
            try:
                getattr(module, digestname)
            except AttributeError:
                fullname = f'{module_name}.{digestname}'
                _missing_hash(fullname, implementation="HACL")
            return func(*args, **kwargs)
        return wrapper

    def decorator(func_or_class):
        return _decorate_func_or_class(func_or_class, decorator_func)
    return decorator


def find_builtin_hashdigest_constructor(
    module_name, digestname, *, usedforsecurity=True
):
    """Find the HACL* hash function constructor.

    - The *module_name* is the C extension module name based on HACL*.
    - The *digestname* is one of its member, e.g., 'md5'.
    """
    module = import_module(module_name)
    try:
        constructor = getattr(module, digestname)
        constructor(b'', usedforsecurity=usedforsecurity)
    except (AttributeError, TypeError, ValueError):
        _missing_hash(f'{module_name}.{digestname}', implementation="HACL")
    return constructor


class HashFunctionsTrait:
    """Mixin trait class containing hash functions.

    This class is assumed to have all unitest.TestCase methods but should
    not directly inherit from it to prevent the test suite being run on it.

    Subclasses should implement the hash functions by returning an object
    that can be recognized as a valid digestmod parameter for both hashlib
    and HMAC. In particular, it cannot be a lambda function as it will not
    be recognized by hashlib (it will still be accepted by the pure Python
    implementation of HMAC).
    """

    ALGORITHMS = [
        'md5', 'sha1',
        'sha224', 'sha256', 'sha384', 'sha512',
        'sha3_224', 'sha3_256', 'sha3_384', 'sha3_512',
    ]

    # Default 'usedforsecurity' to use when looking up a hash function.
    usedforsecurity = True

    def _find_constructor(self, name):
        # By default, a missing algorithm skips the test that uses it.
        self.assertIn(name, self.ALGORITHMS)
        self.skipTest(f"missing hash function: {name}")

    @property
    def md5(self):
        return self._find_constructor("md5")

    @property
    def sha1(self):
        return self._find_constructor("sha1")

    @property
    def sha224(self):
        return self._find_constructor("sha224")

    @property
    def sha256(self):
        return self._find_constructor("sha256")

    @property
    def sha384(self):
        return self._find_constructor("sha384")

    @property
    def sha512(self):
        return self._find_constructor("sha512")

    @property
    def sha3_224(self):
        return self._find_constructor("sha3_224")

    @property
    def sha3_256(self):
        return self._find_constructor("sha3_256")

    @property
    def sha3_384(self):
        return self._find_constructor("sha3_384")

    @property
    def sha3_512(self):
        return self._find_constructor("sha3_512")


class NamedHashFunctionsTrait(HashFunctionsTrait):
    """Trait containing named hash functions.

    Hash functions are available if and only if they are available in hashlib.
    """

    def _find_constructor(self, name):
        self.assertIn(name, self.ALGORITHMS)
        return name


class OpenSSLHashFunctionsTrait(HashFunctionsTrait):
    """Trait containing OpenSSL hash functions.

    Hash functions are available if and only if they are available in _hashlib.
    """

    def _find_constructor(self, name):
        self.assertIn(name, self.ALGORITHMS)
        return find_openssl_hashdigest_constructor(
            name, usedforsecurity=self.usedforsecurity
        )


class BuiltinHashFunctionsTrait(HashFunctionsTrait):
    """Trait containing HACL* hash functions.

    Hash functions are available if and only if they are available in C.
    In particular, HACL* HMAC-MD5 may be available even though HACL* md5
    is not since the former is unconditionally built.
    """

    def _find_constructor_in(self, module, name):
        self.assertIn(name, self.ALGORITHMS)
        return find_builtin_hashdigest_constructor(module, name)

    @property
    def md5(self):
        return self._find_constructor_in("_md5", "md5")

    @property
    def sha1(self):
        return self._find_constructor_in("_sha1", "sha1")

    @property
    def sha224(self):
        return self._find_constructor_in("_sha2", "sha224")

    @property
    def sha256(self):
        return self._find_constructor_in("_sha2", "sha256")

    @property
    def sha384(self):
        return self._find_constructor_in("_sha2", "sha384")

    @property
    def sha512(self):
        return self._find_constructor_in("_sha2", "sha512")

    @property
    def sha3_224(self):
        return self._find_constructor_in("_sha3", "sha3_224")

    @property
    def sha3_256(self):
        return self._find_constructor_in("_sha3","sha3_256")

    @property
    def sha3_384(self):
        return self._find_constructor_in("_sha3","sha3_384")

    @property
    def sha3_512(self):
        return self._find_constructor_in("_sha3","sha3_512")


def find_gil_minsize(modules_names, default=2048):
    """Get the largest GIL_MINSIZE value for the given cryptographic modules.

    The valid module names are the following:

    - _hashlib
    - _md5, _sha1, _sha2, _sha3, _blake2
    - _hmac
    """
    sizes = []
    for module_name in modules_names:
        try:
            module = importlib.import_module(module_name)
        except ImportError:
            continue
        sizes.append(getattr(module, '_GIL_MINSIZE', default))
    return max(sizes, default=default)
