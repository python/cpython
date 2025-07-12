import contextlib
import functools
import hashlib
import importlib
import inspect
import unittest
import unittest.mock
from collections import namedtuple
from test.support.import_helper import import_module
from types import MappingProxyType

try:
    import _hashlib
except ImportError:
    _hashlib = None

try:
    import _hmac
except ImportError:
    _hmac = None


CANONICAL_DIGEST_NAMES = frozenset((
    'md5', 'sha1',
    'sha224', 'sha256', 'sha384', 'sha512',
    'sha3_224', 'sha3_256', 'sha3_384', 'sha3_512',
    'shake_128', 'shake_256',
    'blake2s', 'blake2b',
))

NON_HMAC_DIGEST_NAMES = frozenset({
    'shake_128', 'shake_256', 'blake2s', 'blake2b'
})

# Mapping from a "canonical" name to a pair (HACL*, _hashlib.*, hashlib.*)
# constructors. If the constructor name is None, then this means that the
# algorithm can only be used by the "agile" new() interfaces.

class HashAPI(namedtuple("HashAPI", "builtin openssl hashlib")):

    def fullname(self, typ):
        match typ:
            case "builtin":
                return self.builtin
            case "openssl":
                return f"_hashlib.{self.openssl}" if self.openssl else None
            case "hashlib":
                return f"hashlib.{self.hashlib}" if self.hashlib else None
            case _:
                raise AssertionError(f"unknown type: {typ}")


_EXPLICIT_CONSTRUCTORS = MappingProxyType({
    "md5": HashAPI("_md5.md5", "openssl_md5", "md5"),
    "sha1": HashAPI("_sha1.sha1", "openssl_sha1", "sha1"),
    "sha224": HashAPI("_sha2.sha224", "openssl_sha224", "sha224"),
    "sha256": HashAPI("_sha2.sha256", "openssl_sha256", "sha256"),
    "sha384": HashAPI("_sha2.sha384", "openssl_sha384", "sha384"),
    "sha512": HashAPI("_sha2.sha512", "openssl_sha512", "sha512"),
    "sha3_224": HashAPI("_sha3.sha3_224", "openssl_sha3_224", "sha3_224"),
    "sha3_256": HashAPI("_sha3.sha3_256", "openssl_sha3_256", "sha3_256"),
    "sha3_384": HashAPI("_sha3.sha3_384", "openssl_sha3_384", "sha3_384"),
    "sha3_512": HashAPI("_sha3.sha3_512", "openssl_sha3_512", "sha3_512"),
    "shake_128": HashAPI("_sha3.shake_128", "openssl_shake_128", "shake_128"),
    "shake_256": HashAPI("_sha3.shake_256", "openssl_shake_256", "shake_256"),
    "blake2s": HashAPI("_blake2.blake2s", None, "blake2s"),
    "blake2b": HashAPI("_blake2.blake2b", None, "blake2b"),
})

assert _EXPLICIT_CONSTRUCTORS.keys() == CANONICAL_DIGEST_NAMES

_EXPLICIT_HMAC_CONSTRUCTORS = {
    name: f'_hmac.compute_{name}' for name in (
        'md5', 'sha1',
        'sha224', 'sha256', 'sha384', 'sha512',
        'sha3_224', 'sha3_256', 'sha3_384', 'sha3_512',
    )
}
_EXPLICIT_HMAC_CONSTRUCTORS['shake_128'] = None
_EXPLICIT_HMAC_CONSTRUCTORS['shake_256'] = None
_EXPLICIT_HMAC_CONSTRUCTORS['blake2s'] = '_hmac.compute_blake2s_32'
_EXPLICIT_HMAC_CONSTRUCTORS['blake2b'] = '_hmac.compute_blake2b_32'
_EXPLICIT_HMAC_CONSTRUCTORS = MappingProxyType(_EXPLICIT_HMAC_CONSTRUCTORS)
assert _EXPLICIT_HMAC_CONSTRUCTORS.keys() == CANONICAL_DIGEST_NAMES


def _ensure_wrapper_signature(wrapper, wrapped):
    """Ensure that a wrapper has the same signature as the wrapped function.

    This is used to guarantee that a TypeError raised due to a bad API call
    is raised consistently (using variadic signatures would hide such errors).
    """
    try:
        wrapped_sig = inspect.signature(wrapped)
    except ValueError:  # built-in signature cannot be found
        return

    wrapper_sig = inspect.signature(wrapper)
    if wrapped_sig != wrapper_sig:
        fullname = f"{wrapped.__module__}.{wrapped.__qualname__}"
        raise AssertionError(
            f"signature for {fullname}() is incorrect:\n"
            f"  expect: {wrapped_sig}\n"
            f"  actual: {wrapper_sig}"
        )


def requires_hashlib():
    return unittest.skipIf(_hashlib is None, "requires _hashlib")


def requires_builtin_hmac():
    return unittest.skipIf(_hmac is None, "requires _hmac")


def _missing_hash(digestname, implementation=None, *, exc=None):
    parts = ["missing", implementation, f"hash algorithm: {digestname!r}"]
    msg = " ".join(filter(None, parts))
    raise unittest.SkipTest(msg) from exc


def _openssl_availabillity(digestname, *, usedforsecurity):
    assert isinstance(digestname, str), digestname
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
    assert isinstance(digestname, str), digestname
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
    assert isinstance(digestname, str), digestname
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
    assert isinstance(digestname, str), digestname
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
    assert isinstance(digestname, str), digestname
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

    DIGEST_NAMES = [
        'md5', 'sha1',
        'sha224', 'sha256', 'sha384', 'sha512',
        'sha3_224', 'sha3_256', 'sha3_384', 'sha3_512',
    ]

    # Default 'usedforsecurity' to use when looking up a hash function.
    usedforsecurity = True

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        assert CANONICAL_DIGEST_NAMES.issuperset(cls.DIGEST_NAMES)

    def is_valid_digest_name(self, digestname):
        self.assertIn(digestname, self.DIGEST_NAMES)

    def _find_constructor(self, digestname):
        # By default, a missing algorithm skips the test that uses it.
        self.is_valid_digest_name(digestname)
        self.skipTest(f"missing hash function: {digestname}")

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

    def _find_constructor(self, digestname):
        self.is_valid_digest_name(digestname)
        return digestname


class OpenSSLHashFunctionsTrait(HashFunctionsTrait):
    """Trait containing OpenSSL hash functions.

    Hash functions are available if and only if they are available in _hashlib.
    """

    def _find_constructor(self, digestname):
        self.is_valid_digest_name(digestname)
        return find_openssl_hashdigest_constructor(
            digestname, usedforsecurity=self.usedforsecurity
        )


class BuiltinHashFunctionsTrait(HashFunctionsTrait):
    """Trait containing HACL* hash functions.

    Hash functions are available if and only if they are available in C.
    In particular, HACL* HMAC-MD5 may be available even though HACL* md5
    is not since the former is unconditionally built.
    """

    def _find_constructor_in(self, module, digestname):
        self.is_valid_digest_name(digestname)
        return find_builtin_hashdigest_constructor(module, digestname)

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


def _block_openssl_hash_new(blocked_name):
    """Block OpenSSL implementation of _hashlib.new()."""
    assert isinstance(blocked_name, str), blocked_name
    if _hashlib is None:
        return contextlib.nullcontext()
    @functools.wraps(wrapped := _hashlib.new)
    def wrapper(name, data=b'', *, usedforsecurity=True, string=None):
        if name == blocked_name:
            raise _hashlib.UnsupportedDigestmodError(blocked_name)
        return wrapped(*args, **kwargs)
    _ensure_wrapper_signature(wrapper, wrapped)
    return unittest.mock.patch('_hashlib.new', wrapper)


def _block_openssl_hmac_new(blocked_name):
    """Block OpenSSL HMAC-HASH implementation."""
    assert isinstance(blocked_name, str), blocked_name
    if _hashlib is None:
        return contextlib.nullcontext()
    @functools.wraps(wrapped := _hashlib.hmac_new)
    def wrapper(key, msg=b'', digestmod=None):
        if digestmod == blocked_name:
            raise _hashlib.UnsupportedDigestmodError(blocked_name)
        return wrapped(key, msg, digestmod)
    _ensure_wrapper_signature(wrapper, wrapped)
    return unittest.mock.patch('_hashlib.hmac_new', wrapper)


def _block_openssl_hmac_digest(blocked_name):
    """Block OpenSSL HMAC-HASH one-shot digest implementation."""
    assert isinstance(blocked_name, str), blocked_name
    if _hashlib is None:
        return contextlib.nullcontext()
    @functools.wraps(wrapped := _hashlib.hmac_digest)
    def wrapper(key, msg, digest):
        if digest == blocked_name:
            raise _hashlib.UnsupportedDigestmodError(blocked_name)
        return wrapped(key, msg, digestmod)
    _ensure_wrapper_signature(wrapper, wrapped)
    return unittest.mock.patch('_hashlib.hmac_digest', wrapper)


@contextlib.contextmanager
def _block_builtin_hash_new(name):
    assert isinstance(name, str), name
    assert name.lower() == name, f"invalid name: {name}"

    builtin_cache = getattr(hashlib, '__builtin_constructor_cache')
    if name in builtin_cache:
        f = builtin_cache.pop(name)
        F = builtin_cache.pop(name.upper(), None)
    else:
        f = F = None
    try:
        yield
    finally:
        if f is not None:
            builtin_cache[name] = f
        if F is not None:
            builtin_cache[name.upper()] = F


def _block_builtin_hmac_new(blocked_name):
    assert isinstance(blocked_name, str), blocked_name
    if _hmac is None:
        return contextlib.nullcontext()
    @functools.wraps(wrapped := _hmac.new)
    def wrapper(key, msg=None, digestmod=None):
        if digestmod == blocked_name:
            raise _hmac.UnknownHashError(blocked_name)
        return wrapped(key, msg, digestmod)
    _ensure_wrapper_signature(wrapper, wrapped)
    return unittest.mock.patch('_hmac.new', wrapper)


def _block_builtin_hmac_digest(blocked_name):
    assert isinstance(blocked_name, str), blocked_name
    if _hmac is None:
        return contextlib.nullcontext()
    @functools.wraps(wrapped := _hmac.compute_digest)
    def wrapper(key, msg, digest):
        if digest == blocked_name:
            raise _hmac.UnknownHashError(blocked_name)
        return wrapped(key, msg, digest)
    _ensure_wrapper_signature(wrapper, wrapped)
    return unittest.mock.patch('_hmac.compute_digest', wrapper)


def _make_hash_constructor_blocker(name, dummy, *, interface):
    assert isinstance(name, str), name
    assert interface in ('builtin', 'openssl', 'hashlib')
    assert name in _EXPLICIT_CONSTRUCTORS, f"invalid hash: {name}"
    fullname = _EXPLICIT_CONSTRUCTORS[name].fullname(interface)
    if fullname is None:
        # function shouldn't exist for this implementation
        return contextlib.nullcontext()
    assert fullname.count('.') == 1, fullname
    module_name, method = fullname.split('.', maxsplit=1)
    try:
        module = importlib.import_module(module_name)
    except ImportError:
        # module is already disabled
        return contextlib.nullcontext()
    wrapped = getattr(module, method)
    wrapper = functools.wraps(wrapped)(dummy)
    _ensure_wrapper_signature(wrapper, wrapped)
    return unittest.mock.patch(fullname, wrapper)


def _block_hashlib_hash_constructor(name):
    """Block explicit public constructors."""
    assert isinstance(name, str), name
    def dummy(data=b'', *, usedforsecurity=True, string=None):
        raise ValueError(f"unsupported hash name: {name}")
    return _make_hash_constructor_blocker(name, dummy, interface='hashlib')


def _block_openssl_hash_constructor(name):
    """Block explicit OpenSSL constructors."""
    assert isinstance(name, str), name
    def dummy(data=b'', *, usedforsecurity=True, string=None):
        raise ValueError(f"unsupported hash name: {name}")
    return _make_hash_constructor_blocker(name, dummy, interface='openssl')


def _block_builtin_hash_constructor(name):
    """Block explicit HACL* constructors."""
    assert isinstance(name, str), name
    def dummy(data=b'', *, usedforsecurity=True, string=b''):
        raise ValueError(f"unsupported hash name: {name}")
    return _make_hash_constructor_blocker(name, dummy, interface='builtin')


def _block_builtin_hmac_constructor(name):
    """Block explicit HACL* HMAC constructors."""
    assert isinstance(name, str), name
    assert name in _EXPLICIT_HMAC_CONSTRUCTORS, f"invalid hash: {name}"
    fullname = _EXPLICIT_HMAC_CONSTRUCTORS[name]
    if fullname is None:
        # function shouldn't exist for this implementation
        return contextlib.nullcontext()
    assert fullname.count('.') == 1, fullname
    module_name, method = fullname.split('.', maxsplit=1)
    assert module_name == '_hmac', module_name
    try:
        module = importlib.import_module(module_name)
    except ImportError:
        # module is already disabled
        return contextlib.nullcontext()
    @functools.wraps(wrapped := getattr(module, method))
    def wrapper(key, obj):
        raise ValueError(f"unsupported hash name: {name}")
    _ensure_wrapper_signature(wrapper, wrapped)
    return unittest.mock.patch(fullname, wrapper)


@contextlib.contextmanager
def block_algorithm(*names, allow_openssl=False, allow_builtin=False):
    """Block a hash algorithm for both hashing and HMAC."""
    with contextlib.ExitStack() as stack:
        for name in names:
            if not (allow_openssl or allow_builtin):
                # If one of the private interface is allowed, then the
                # public interface will fallback to it even though the
                # comment in hashlib.py says otherwise.
                #
                # So we should only block it if the private interfaces
                # are blocked as well.
                stack.enter_context(_block_hashlib_hash_constructor(name))
            if not allow_openssl:
                stack.enter_context(_block_openssl_hash_new(name))
                stack.enter_context(_block_openssl_hmac_new(name))
                stack.enter_context(_block_openssl_hmac_digest(name))
                stack.enter_context(_block_openssl_hash_constructor(name))
            if not allow_builtin:
                stack.enter_context(_block_builtin_hash_new(name))
                stack.enter_context(_block_builtin_hmac_new(name))
                stack.enter_context(_block_builtin_hmac_digest(name))
                stack.enter_context(_block_builtin_hash_constructor(name))
                stack.enter_context(_block_builtin_hmac_constructor(name))
        yield
