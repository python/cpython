import functools
import hashlib
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
                msg = f"missing hash algorithm: {digestname!r}"
                raise unittest.SkipTest(msg) from exc
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
        @requires_hashlib()
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            try:
                _hashlib.new(digestname, usedforsecurity=usedforsecurity)
            except ValueError:
                msg = f"missing OpenSSL hash algorithm: {digestname!r}"
                raise unittest.SkipTest(msg)
            return func(*args, **kwargs)
        return wrapper

    def decorator(func_or_class):
        return _decorate_func_or_class(func_or_class, decorator_func)
    return decorator
